#include "drivers/gles3/rasterizer_gles3.h"
#include "core/class_db.h"
#include "core/dictionary.h"
#include "core/map.h"
#include "core/object.h"
#include "core/script_language.h"
#include "gles3_renderer.h"
#include "servers/visual/visual_server_globals.h"
#include "wayland_display.h"
#include "wlr_backend.h"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

extern "C" {
#include <wlr/backend.h>
#include <wlr/backend/interface.h>
#include <wlr/render/wlr_renderer.h>
#include <wlr/util/log.h>
}

void log_debug_dma(const char* fmt, ...);

namespace {

/* File-local global variable for keeping track of object counts when debugging memory usage from godot-haskell-plugin

	 Examples:

	 "type:WlrGLES3Texture" -> 33
	 "parent:Reference"     -> 120
	 "script:SomeScript"    -> 4
*/

Map<String, int> *object_usage_counts = NULL;

void increment_object_usage_label_count(const String &class_name, const String &label_prefix = String()) {
	if (class_name.empty()) {
		return;
	}

	String label = label_prefix + class_name;
	Map<String, int>::Element *element = object_usage_counts->find(label);
	if (element) {
		element->get()++;
	} else {
		object_usage_counts->insert(label, 1);
	}
}

String try_call_object_method_for_string(Object *object, const StringName &method_name) {
	if (!object || !object->has_method(method_name)) {
		return String();
	}

	Variant::CallError call_error;
	Variant result = object->call(method_name, nullptr, 0, call_error);
	if (call_error.error != Variant::CallError::CALL_OK || result.get_type() != Variant::STRING) {
		return String();
	}

	return result;
}

String try_get_string_from_object_property(Object *object, const StringName &property_name) {
	if (!object) {
		return String();
	}

	bool valid = false;
	Variant result = object->get(property_name, &valid);
	if (!valid || result.get_type() != Variant::STRING) {
		return String();
	}

	return result;
}

String try_get_script_class_name_from_script_resource(Object *script_object) {
	String script_name = try_get_string_from_object_property(script_object, StringName("class_name"));
	if (script_name.empty()) {
		script_name = try_get_string_from_object_property(script_object, StringName("script_class_name"));
	}
	if (script_name.empty()) {
		script_name = try_call_object_method_for_string(script_object, StringName("get_class_name"));
	}
	if (script_name.empty()) {
		script_name = try_call_object_method_for_string(script_object, StringName("get_script_class_name"));
	}

	return script_name;
}

String try_get_script_class_name_for_object(Object *object) {
	if (object->get_class() == "NativeScript") {
		return try_get_script_class_name_from_script_resource(object);
	}

	ScriptInstance *script_instance = object->get_script_instance();
	if (!script_instance) {
		return String();
	}

	Ref<Script> script = script_instance->get_script();
	if (!script.is_valid()) {
		return String();
	}

	Object *script_object = script.ptr();
	String script_name = try_get_script_class_name_from_script_resource(script_object);
	if (script_name.empty() && !script->get_path().empty()) {
		script_name = script->get_path().get_file().get_basename();
	}

	return script_name;
}

/* This is the callback we supply to ObjectDB::debug_objects(..) so that we can get more fine-grained information about live objects being detected
  In particular, it adds labels to `object_usage_counts`


  For an example, it might fire on a new `* Object` which, by introspection on it, happens to be a `WlrGLES3Texture`. It will then adjust object_usage_counts something like:

  type:WlrGLES3Texture += 1
  parent:Texture       += 1
  parent:Resource      += 1
  parent:Reference     += 1
  parent:Object        += 1

*/
void record_object_usage_labels_for_debug_object(Object *object) {
	if (!object_usage_counts || !object) {
		return;
	}

	StringName native_class_name = object->get_class();
	if (!ClassDB::class_exists(native_class_name)) {
		native_class_name = object->get_class_name();
	}

	String script_name = try_get_script_class_name_for_object(object);
	if (!script_name.empty() && script_name != String(native_class_name)) {
		increment_object_usage_label_count(script_name, "script:");
	}

	bool exact_class = true;
	for (StringName class_name = native_class_name; class_name != StringName();
			class_name = ClassDB::get_parent_class(class_name)) {
		increment_object_usage_label_count(String(class_name), exact_class ? "type:" : "parent:");
		exact_class = false;
	}
}
}

extern "C" {
bool backend_start(struct wlr_backend *backend) {
	/* This space deliberately left blank */
	return false;
}

void backend_destroy(struct wlr_backend *backend) {
	/* This space deliberately left blank */
}

struct wlr_renderer *backend_get_renderer(struct wlr_backend *_backend) {
	WlrBackend *backend = (WlrBackend *)_backend;
	return backend->get_renderer()->get_wlr_renderer();
}

static const struct wlr_backend_impl backend_impl = {
	.start = backend_start,
	.destroy = backend_destroy,
	.get_renderer = backend_get_renderer,
};

}

void WlrBackend::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_live_object_dictionary"), &WlrBackend::get_live_object_dictionary);
}

struct WlrRenderer *WlrBackend::get_renderer() {
	return renderer;
}

struct wlr_backend *WlrBackend::get_wlr_backend() {
	return (struct wlr_backend *)this;
}

/*
	Samples all currently live Godot Objects known to ObjectDB
	at this instant, including objects that are not in the scene tree.

	Populates a Map<String, int> with label-to-count entries,
	then converts that map into a Dictionary where each key is a
	label and each value is the count for that label.

	Example entries:
	"type:WlrGLES3Texture" -> 33
	"parent:Reference" -> 120
	"script:GodotSimulaServer" -> 1

	It would be cleaner to just make this a global function that doesn't
	require us to pass a WlrBackend, but not sure how to do this in
	godot-haskell and it's natural enough to just stick it here.
*/
Dictionary WlrBackend::get_live_object_dictionary() {
	Map<String, int> counts;
	object_usage_counts = &counts;
	ObjectDB::debug_objects(record_object_usage_labels_for_debug_object);
	object_usage_counts = NULL;

	Dictionary dictionary;
	for (Map<String, int>::Element *element = counts.front(); element; element = element->next()) {
		dictionary[element->key()] = element->get();
	}
	return dictionary;
}

WaylandDisplay *WlrBackend::get_wayland_display() {
	Node *parent = get_parent();
	WaylandDisplay *display = dynamic_cast<WaylandDisplay *>(parent);
	while (parent && !display) {
		parent = parent->get_parent();
		display = dynamic_cast<WaylandDisplay *>(parent);
	}
	return display;
}

void WlrBackend::_notification(int p_what) {
	WaylandDisplay *display = get_wayland_display();
	switch (p_what) {
	case NOTIFICATION_ENTER_TREE:
		log_debug_dma("DEBUG: WlrBackend entered tree, initializing wl_display\n");
		if (display != initialized_display) {
			wlr_renderer_init_wl_display(
				get_renderer()->get_wlr_renderer(),
				display->get_wayland_display());

			if (auto *gles3_renderer = dynamic_cast<WlrGLES3Renderer *>(get_renderer());
					gles3_renderer && gles3_renderer->is_dmabuf_available()) {
				// Only advertise linux-dmabuf when the EGL Wayland bind succeeded.
				linux_dmabuf = wlr_linux_dmabuf_v1_create(
					display->get_wayland_display(),
					get_renderer()->get_wlr_renderer());
				if (linux_dmabuf) {
					log_debug_dma("DEBUG: wlr_linux_dmabuf_v1 protocol global created successfully\n");
				} else {
					log_debug_dma("DEBUG: Failed to create wlr_linux_dmabuf_v1 protocol global\n");
				}
			} else {
				log_debug_dma("DEBUG: Skipping wlr_linux_dmabuf_v1 protocol global; dmabuf EGL binding unavailable\n");
			}

			initialized_display = display;
		}
		break;
	}
}

WlrBackend::WlrBackend() {
	wlr_log_init(WLR_ERROR, NULL);
	initialized_display = NULL;
	linux_dmabuf = NULL;
	auto gles3_rasterizer = dynamic_cast<RasterizerGLES3 *>(VSG::rasterizer);
	if (auto gles3_rasterizer = dynamic_cast<RasterizerGLES3 *>(VSG::rasterizer)) {
		renderer = new WlrGLES3Renderer(gles3_rasterizer);
       	} else {
		print_line("Unsupported rasterizer backend");
		assert(0);
	}
	wlr_backend_init(&backend, &backend_impl);
}

WlrBackend::~WlrBackend() {
	if (linux_dmabuf) {
		wlr_linux_dmabuf_v1_destroy(linux_dmabuf);
		linux_dmabuf = NULL;
	}
	wlr_backend_destroy(&backend);
}

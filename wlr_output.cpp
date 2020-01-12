#include "core/object.h"
#include "scene/main/viewport.h"
#include "wayland_display.h"
#include "wlr_backend.h"
#include "wlr_output.h"
#include <stdlib.h>
#include <iostream>
extern "C" {
#include <wlr/interfaces/wlr_output.h>

static void transform(struct wlr_output *output,
		enum wl_output_transform transform) {
	// TODO: Should we transform the viewport?
}

static bool make_current(struct wlr_output *output, int *buffer_age) {
	/* This space deliberately left blank */
	return false;
}

static bool swap_buffers(struct wlr_output *output, pixman_region32_t *damage) {
	/* This space deliberately left blank */
	return false;
}

static const struct wlr_output_impl output_impl = {
	.transform = transform,
	/*
	 * wlroots requires these to be implemented, but since Godot handles the
	 * render lifecycle for us, these are just shims.
	 */
	.make_current = make_current,
	.swap_buffers = swap_buffers,
};

}

void WlrOutput::_size_changed() {
	if (wlr_output == NULL) {
		return;
	}
  wlr_output_update_custom_mode(wlr_output, 2560, 1440, 90000); //1024x768
}

void WlrOutput::_bind_methods() {
	ClassDB::bind_method(D_METHOD("_size_changed"), &WlrOutput::_size_changed);
}

WlrBackend *WlrOutput::get_wlr_backend() {
	Node *parent = get_parent();
	WlrBackend *backend = dynamic_cast<WlrBackend *>(parent);
	while (parent && !backend) {
		parent = parent->get_parent();
		backend = dynamic_cast<WlrBackend *>(parent);
	}
	return backend;
}

void WlrOutput::ensure_wl_global(WaylandDisplay *display) {
	if (wlr_output) {
		return;
	}
	auto backend = get_wlr_backend();
	wlr_output = (struct wlr_output *)calloc(sizeof(struct wlr_output), 1);
	wlr_output_init(wlr_output, backend->get_wlr_backend(), &output_impl,
			display->get_wayland_display());
	strncpy(wlr_output->make, "Godot", sizeof(wlr_output->make));
	strncpy(wlr_output->model, "Godot", sizeof(wlr_output->model));
	// TODO: multiple outputs with unique names
	strncpy(wlr_output->name, "GD-1", sizeof(wlr_output->name));
	_size_changed();
	wlr_output_create_global(wlr_output);
}

void WlrOutput::destroy_wl_global(WaylandDisplay *display) {
	wlr_output_destroy(wlr_output);
	wlr_output = NULL;
}

void WlrOutput::_notification(int p_what) {
	switch (p_what) {
	case NOTIFICATION_ENTER_TREE:
		viewport = get_tree()->get_root();
//		viewport->connect("size_changed", this, "_size_changed");
		break;
	case NOTIFICATION_EXIT_TREE:
//		viewport->disconnect("size_changed", this, "_size_changed");
		break;
	}
	WaylandGlobal::_notification(p_what);
}

WlrOutput::WlrOutput() {
	wlr_output = NULL;
}

WlrOutput::~WlrOutput() {
	wlr_output_destroy(wlr_output);
	wlr_output = NULL;
}

struct wlr_output *WlrOutput::get_wlr_output() const {
	return wlr_output;
}

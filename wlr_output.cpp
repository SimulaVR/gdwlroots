#include "core/object.h"
#include "scene/main/viewport.h"
#include "wayland_display.h"
#include "wlr_backend.h"
#include "wlr_output.h"
#include <stdlib.h>
#include <typeinfo>
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

static struct wlr_output_impl output_impl = {
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
	// TODO: This always returns 0x0, but the internet suggests that it should
	// work correctly.
	//auto size = viewport->get_visible_rect().size;
	wlr_output_set_custom_mode(wlr_output, 1280, 720, 0);
}

void WlrOutput::_bind_methods() {
	ClassDB::bind_method(D_METHOD("_size_changed"), &WlrOutput::_size_changed);
}

WaylandDisplay *WlrOutput::get_wayland_display() {
	Node *parent = get_parent();
	WaylandDisplay *display = dynamic_cast<WaylandDisplay *>(parent);
	while (parent && !display) {
		parent = parent->get_parent();
		display = dynamic_cast<WaylandDisplay *>(parent);
	}
	return display;
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

void WlrOutput::ensure_wlr_output() {
	if (wlr_output) {
		return;
	}
	// TODO: We probably need a backend
	auto display = get_wayland_display();
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

void WlrOutput::_notification(int p_what) {
	switch (p_what) {
	case NOTIFICATION_ENTER_TREE:
		viewport = get_tree()->get_root();
		viewport->connect("size_changed", this, "_size_changed");
		ensure_wlr_output();
		break;
	case NOTIFICATION_EXIT_TREE:
		wlr_output_destroy(wlr_output);
		wlr_output = NULL;
		viewport->disconnect("size_changed", this, "_size_changed");
		break;
	}
}

WlrOutput::WlrOutput() {
	wlr_output = NULL;
}

WlrOutput::~WlrOutput() {
	wlr_output_destroy(wlr_output);
	wlr_output = NULL;
}

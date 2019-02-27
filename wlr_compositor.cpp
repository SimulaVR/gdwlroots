#include "core/object.h"
#include "scene/main/node.h"
#include "wayland_display.h"
#include "wlr_compositor.h"
#include "wlr_backend.h"
#include "wlr_surface.h"
extern "C" {
#include <wlr/types/wlr_compositor.h>

void WlrCompositor::handle_new_surface(
		struct wl_listener *listener, void *data) {
	WlrCompositor *compositor = wl_container_of(
			listener, compositor, new_surface);
	auto surface = new WlrSurface((struct wlr_surface *)data);
	compositor->emit_signal("new_surface", surface);
}

}

void WlrCompositor::_bind_methods() {
	ADD_SIGNAL(MethodInfo("new_surface",
				PropertyInfo(Variant::OBJECT,
					"surface", PROPERTY_HINT_RESOURCE_TYPE, "WlrSurface")));
}

WaylandDisplay *WlrCompositor::get_wayland_display() {
	Node *parent = get_parent();
	WaylandDisplay *display = dynamic_cast<WaylandDisplay *>(parent);
	while (parent && !display) {
		parent = parent->get_parent();
		display = dynamic_cast<WaylandDisplay *>(parent);
	}
	return display;
}

WlrBackend *WlrCompositor::get_wlr_backend() {
	Node *parent = get_parent();
	WlrBackend *backend = dynamic_cast<WlrBackend *>(parent);
	while (parent && !backend) {
		parent = parent->get_parent();
		backend = dynamic_cast<WlrBackend *>(parent);
	}
	return backend;
}

void WlrCompositor::ensure_wlr_compositor() {
	if (wlr_compositor) {
		return;
	}
	auto display = get_wayland_display();
	auto backend = get_wlr_backend();
	auto renderer = backend->get_renderer();
	wlr_compositor = wlr_compositor_create(
			display->get_wayland_display(), renderer->get_wlr_renderer());
	new_surface.notify = handle_new_surface;
	wl_signal_add(&wlr_compositor->events.new_surface, &new_surface);
}

void WlrCompositor::_notification(int p_what) {
	switch (p_what) {
	case NOTIFICATION_ENTER_TREE:
		ensure_wlr_compositor();
		break;
	case NOTIFICATION_EXIT_TREE:
		wlr_compositor_destroy(wlr_compositor);
		wlr_compositor = NULL;
		break;
	}
}

WlrCompositor::WlrCompositor() {
	wlr_compositor = NULL;
}

WlrCompositor::~WlrCompositor() {
	wlr_compositor_destroy(wlr_compositor);
	wlr_compositor = NULL;
}

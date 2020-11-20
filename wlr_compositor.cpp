#include "core/object.h"
#include "scene/main/node.h"
#include "wayland_display.h"
#include "wlr_compositor.h"
#include "wlr_backend.h"
#include "wlr_surface.h"
#include <iostream>
extern "C" {
#include <wlr/types/wlr_compositor.h>

void WlrCompositor::handle_new_surface(
		struct wl_listener *listener, void *data) {
	//std::cout << "WlrCompositor::handle_new_surface(..): " << (struct wlr_surface *)data << std::endl;
	WlrCompositor *compositor = wl_container_of(
			listener, compositor, new_surface);
	auto surface = WlrSurface::from_wlr_surface((struct wlr_surface *)data);
	compositor->emit_signal("new_surface", surface);
}

}

void WlrCompositor::_bind_methods() {
	ADD_SIGNAL(MethodInfo("new_surface",
				PropertyInfo(Variant::OBJECT,
					"surface", PROPERTY_HINT_RESOURCE_TYPE, "WlrSurface")));
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

void WlrCompositor::ensure_wl_global(WaylandDisplay *display) {
	if (wlr_compositor) {
		return;
	}
	auto backend = get_wlr_backend();
	auto renderer = backend->get_renderer();
	wlr_compositor = wlr_compositor_create(
			display->get_wayland_display(), renderer->get_wlr_renderer());
	new_surface.notify = handle_new_surface;
	wl_signal_add(&wlr_compositor->events.new_surface, &new_surface);
}

void WlrCompositor::destroy_wl_global(WaylandDisplay *display) {
	wlr_compositor_destroy(wlr_compositor);
	wlr_compositor = NULL;
}

WlrCompositor::WlrCompositor() {
	wlr_compositor = NULL;
}

WlrCompositor::~WlrCompositor() {
	wlr_compositor_destroy(wlr_compositor);
	wlr_compositor = NULL;
}

struct wlr_compositor * WlrCompositor::get_wlr_compositor() {
  return wlr_compositor;
}

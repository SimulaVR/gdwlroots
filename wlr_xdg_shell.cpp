#include "core/object.h"
#include "scene/main/node.h"
#include "wayland_display.h"
#include "wlr_surface.h"
#include "wlr_xdg_shell.h"
extern "C" {
#include <wayland-server.h>
#include <wlr/types/wlr_xdg_shell.h>

void WlrXdgShell::handle_new_xdg_surface(
		struct wl_listener *listener, void *data) {
	WlrXdgShell *xdg_shell = wl_container_of(
			listener, xdg_shell, new_xdg_surface);
	auto surface = WlrXdgSurface::from_wlr_xdg_surface(
			(struct wlr_xdg_surface *)data);
	xdg_shell->emit_signal("new_surface", surface);
}

}

void WlrXdgShell::_bind_methods() {
	ADD_SIGNAL(MethodInfo("new_surface",
				PropertyInfo(Variant::OBJECT,
					"surface", PROPERTY_HINT_RESOURCE_TYPE, "WlrXdgSurface")));
}

/* TODO: Consider abstracting this with a generic wlr node base class */
WaylandDisplay *WlrXdgShell::get_wayland_display() {
	Node *parent = get_parent();
	WaylandDisplay *display = dynamic_cast<WaylandDisplay *>(parent);
	while (parent && !display) {
		parent = parent->get_parent();
		display = dynamic_cast<WaylandDisplay *>(parent);
	}
	return display;
}

void WlrXdgShell::ensure_wlr_xdg_shell() {
	if (wlr_xdg_shell) {
		return;
	}
	auto display = get_wayland_display();
	wlr_xdg_shell = wlr_xdg_shell_create(display->get_wayland_display());
	new_xdg_surface.notify = handle_new_xdg_surface;
	wl_signal_add(&wlr_xdg_shell->events.new_surface,
			&new_xdg_surface);
}

void WlrXdgShell::_notification(int p_what) {
	switch (p_what) {
	case NOTIFICATION_ENTER_TREE:
		ensure_wlr_xdg_shell();
		break;
	case NOTIFICATION_EXIT_TREE:
		wlr_xdg_shell_destroy(wlr_xdg_shell);
		wlr_xdg_shell = NULL;
		break;
	}
}

WlrXdgShell::WlrXdgShell() {
	wlr_xdg_shell = NULL;
}

WlrXdgShell::~WlrXdgShell() {
	wlr_xdg_shell_destroy(wlr_xdg_shell);
	wlr_xdg_shell = NULL;
}

Rect2 WlrXdgSurface::get_geometry() {
	return Rect2(wlr_xdg_surface->geometry.x, wlr_xdg_surface->geometry.y,
			wlr_xdg_surface->geometry.width, wlr_xdg_surface->geometry.height);
}

WlrSurface *WlrXdgSurface::get_wlr_surface() const {
	return WlrSurface::from_wlr_surface(wlr_xdg_surface->surface);
}

void WlrXdgSurface::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_geometry"),
			&WlrXdgSurface::get_geometry);
	ClassDB::bind_method(D_METHOD("get_wlr_surface"),
			&WlrXdgSurface::get_wlr_surface);
}

WlrXdgSurface::WlrXdgSurface() {
	/* Not used */
}

WlrXdgSurface::WlrXdgSurface(struct wlr_xdg_surface *xdg_surface) {
	// TODO: Handle surface destroyed
	wlr_xdg_surface = xdg_surface;
	xdg_surface->data = this;
}


WlrXdgSurface *WlrXdgSurface::from_wlr_xdg_surface(
		struct wlr_xdg_surface *xdg_surface) {
	if (xdg_surface->data) {
		return (WlrXdgSurface *)xdg_surface->data;
	}
	return new WlrXdgSurface(xdg_surface);
}

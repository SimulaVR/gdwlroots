#include <assert.h>
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

void WlrXdgShell::ensure_wl_global(WaylandDisplay *display) {
	if (wlr_xdg_shell) {
		return;
	}
	wlr_xdg_shell = wlr_xdg_shell_create(display->get_wayland_display());
	new_xdg_surface.notify = handle_new_xdg_surface;
	wl_signal_add(&wlr_xdg_shell->events.new_surface,
			&new_xdg_surface);
}

void WlrXdgShell::destroy_wl_global(WaylandDisplay *display) {
	wlr_xdg_shell_destroy(wlr_xdg_shell);
	wlr_xdg_shell = NULL;
}

WlrXdgShell::WlrXdgShell() {
	wlr_xdg_shell = NULL;
}

WlrXdgShell::~WlrXdgShell() {
	wlr_xdg_shell_destroy(wlr_xdg_shell);
	wlr_xdg_shell = NULL;
}

WlrXdgSurface::XdgSurfaceRole WlrXdgSurface::get_role() const {
	switch (wlr_xdg_surface->role) {
	case WLR_XDG_SURFACE_ROLE_TOPLEVEL:
		return XDG_SURFACE_ROLE_TOPLEVEL;
	case WLR_XDG_SURFACE_ROLE_POPUP:
		return XDG_SURFACE_ROLE_POPUP;
	case WLR_XDG_SURFACE_ROLE_NONE:
	default:
		return XDG_SURFACE_ROLE_NONE;
	}
}

WlrXdgToplevel *WlrXdgSurface::get_xdg_toplevel() const {
	assert(wlr_xdg_surface->role == WLR_XDG_SURFACE_ROLE_TOPLEVEL);
	return toplevel;
}

WlrXdgPopup *WlrXdgSurface::get_xdg_popup() const {
	assert(wlr_xdg_surface->role == WLR_XDG_SURFACE_ROLE_POPUP);
	return popup;
}

Rect2 WlrXdgSurface::get_geometry() {
	return Rect2(wlr_xdg_surface->geometry.x, wlr_xdg_surface->geometry.y,
			wlr_xdg_surface->geometry.width, wlr_xdg_surface->geometry.height);
}

WlrSurface *WlrXdgSurface::get_wlr_surface() const {
	return WlrSurface::from_wlr_surface(wlr_xdg_surface->surface);
}

void WlrXdgSurface::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_role"), &WlrXdgSurface::get_role);
	ClassDB::bind_method(D_METHOD("get_xdg_toplevel"),
			&WlrXdgSurface::get_xdg_toplevel);
	ClassDB::bind_method(D_METHOD("get_xdg_popup"),
			&WlrXdgSurface::get_xdg_popup);
	ClassDB::bind_method(D_METHOD("get_geometry"),
			&WlrXdgSurface::get_geometry);
	ClassDB::bind_method(D_METHOD("get_wlr_surface"),
			&WlrXdgSurface::get_wlr_surface);

	BIND_ENUM_CONSTANT(XDG_SURFACE_ROLE_NONE);
	BIND_ENUM_CONSTANT(XDG_SURFACE_ROLE_TOPLEVEL);
	BIND_ENUM_CONSTANT(XDG_SURFACE_ROLE_POPUP);

	ADD_SIGNAL(MethodInfo("destroy",
			PropertyInfo(Variant::OBJECT,
				"xdg_surface", PROPERTY_HINT_RESOURCE_TYPE, "WlrXdgSurface")));
}

extern "C" {

void WlrXdgSurface::handle_destroy(
		struct wl_listener *listener, void *data) {
	WlrXdgSurface *xdg_surface = wl_container_of(
			listener, xdg_surface, destroy);
	xdg_surface->emit_signal("destroy", xdg_surface);
}

}

WlrXdgSurface::WlrXdgSurface() {
	/* Not used */
}

WlrXdgSurface::WlrXdgSurface(struct wlr_xdg_surface *xdg_surface) {
	wlr_xdg_surface = xdg_surface;
	xdg_surface->data = this;
	destroy.notify = handle_destroy;
	wl_signal_add(&xdg_surface->events.destroy, &destroy);
	switch (xdg_surface->role) {
	case WLR_XDG_SURFACE_ROLE_TOPLEVEL:
		toplevel = WlrXdgToplevel::from_wlr_xdg_toplevel(
				wlr_xdg_surface->toplevel);
		break;
	case WLR_XDG_SURFACE_ROLE_POPUP:
		popup = WlrXdgPopup::from_wlr_xdg_popup(wlr_xdg_surface->popup);
		break;
	case WLR_XDG_SURFACE_ROLE_NONE:
		assert(0);
		break;
	}
}

WlrXdgSurface *WlrXdgSurface::from_wlr_xdg_surface(
		struct wlr_xdg_surface *xdg_surface) {
	if (xdg_surface->data) {
		return (WlrXdgSurface *)xdg_surface->data;
	}
	return new WlrXdgSurface(xdg_surface);
}

extern "C" {

void WlrXdgToplevel::handle_set_parent(
		struct wl_listener *listener, void *data) {
	WlrXdgToplevel *xdg_toplevel = wl_container_of(
			listener, xdg_toplevel, set_parent);
	xdg_toplevel->emit_signal("set_parent", xdg_toplevel);
}

void WlrXdgToplevel::handle_set_title(
		struct wl_listener *listener, void *data) {
	WlrXdgToplevel *xdg_toplevel = wl_container_of(
			listener, xdg_toplevel, set_title);
	xdg_toplevel->emit_signal("set_title", xdg_toplevel);
}

void WlrXdgToplevel::handle_set_app_id(
		struct wl_listener *listener, void *data) {
	WlrXdgToplevel *xdg_toplevel = wl_container_of(
			listener, xdg_toplevel, set_app_id);
	xdg_toplevel->emit_signal("set_app_id", xdg_toplevel);
}

}

WlrXdgToplevel::WlrXdgToplevel() {
	/* Not used */
}

WlrXdgToplevel::WlrXdgToplevel(struct wlr_xdg_toplevel *xdg_toplevel) {
	wlr_xdg_toplevel = xdg_toplevel;
	// TODO: Bind more listeners
	set_parent.notify = handle_set_parent;
	wl_signal_add(&wlr_xdg_toplevel->events.set_parent, &set_parent);
	set_title.notify = handle_set_title;
	wl_signal_add(&wlr_xdg_toplevel->events.set_title, &set_title);
	set_app_id.notify = handle_set_app_id;
	wl_signal_add(&wlr_xdg_toplevel->events.set_app_id, &set_app_id);
}

WlrXdgToplevel *WlrXdgToplevel::from_wlr_xdg_toplevel(
		struct wlr_xdg_toplevel *xdg_toplevel) {
	WlrXdgSurface *surface = (WlrXdgSurface *)xdg_toplevel->base->data;
	if (surface->toplevel) {
		return surface->toplevel;
	}
	return new WlrXdgToplevel(xdg_toplevel);
}

WlrXdgToplevel *WlrXdgToplevel::get_parent() const {
	return from_wlr_xdg_toplevel(wlr_xdg_toplevel->parent->toplevel);
}

String WlrXdgToplevel::get_app_id() const {
	return String(wlr_xdg_toplevel->app_id);
}

String WlrXdgToplevel::get_title() const {
	return String(wlr_xdg_toplevel->title);
}

void WlrXdgToplevel::set_size(Vector2 size) {
	wlr_xdg_toplevel_set_size(wlr_xdg_toplevel->base, size.width, size.height);
}

void WlrXdgToplevel::set_activated(bool activated) {
	wlr_xdg_toplevel_set_activated(wlr_xdg_toplevel->base, activated);
}

void WlrXdgToplevel::set_maximized(bool maximized) {
	wlr_xdg_toplevel_set_maximized(wlr_xdg_toplevel->base, maximized);
}

void WlrXdgToplevel::set_fullscreen(bool fullscreen) {
	wlr_xdg_toplevel_set_fullscreen(wlr_xdg_toplevel->base, fullscreen);
}

void WlrXdgToplevel::set_resizing(bool resizing) {
	wlr_xdg_toplevel_set_resizing(wlr_xdg_toplevel->base, resizing);
}

void WlrXdgToplevel::set_tiled(bool tiled) {
	wlr_xdg_toplevel_set_tiled(wlr_xdg_toplevel->base, tiled);
}

void WlrXdgToplevel::send_close() {
	wlr_xdg_toplevel_send_close(wlr_xdg_toplevel->base);
}

void WlrXdgToplevel::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_parent"), &WlrXdgToplevel::get_parent);
	ClassDB::bind_method(D_METHOD("get_title"), &WlrXdgToplevel::get_title);
	ClassDB::bind_method(D_METHOD("get_app_id"), &WlrXdgToplevel::get_app_id);
	ClassDB::bind_method(D_METHOD("set_size", "size"),
			&WlrXdgToplevel::set_size);
	ClassDB::bind_method(D_METHOD("set_activated", "activated"),
			&WlrXdgToplevel::set_activated);
	ClassDB::bind_method(D_METHOD("set_maximized", "maximized"),
			&WlrXdgToplevel::set_maximized);
	ClassDB::bind_method(D_METHOD("set_fullscreen", "fullscreen"),
			&WlrXdgToplevel::set_fullscreen);
	ClassDB::bind_method(D_METHOD("set_resizing", "resizing"),
			&WlrXdgToplevel::set_resizing);
	ClassDB::bind_method(D_METHOD("set_tiled", "tiled"),
			&WlrXdgToplevel::set_tiled);
	ClassDB::bind_method(D_METHOD("send_close"), &WlrXdgToplevel::send_close);

	ADD_SIGNAL(MethodInfo("set_app_id",
			PropertyInfo(Variant::OBJECT,
				"xdg_toplevel", PROPERTY_HINT_RESOURCE_TYPE, "WlrXdgToplevel")));
	ADD_SIGNAL(MethodInfo("set_title",
			PropertyInfo(Variant::OBJECT,
				"xdg_toplevel", PROPERTY_HINT_RESOURCE_TYPE, "WlrXdgToplevel")));
	ADD_SIGNAL(MethodInfo("set_parent",
			PropertyInfo(Variant::OBJECT,
				"xdg_toplevel", PROPERTY_HINT_RESOURCE_TYPE, "WlrXdgToplevel")));
}

WlrXdgToplevelState::WlrXdgToplevelState() {
	/* Not used */
}

void WlrXdgToplevelState::_bind_methods() {
	// TODO: bind all that stuff
}

WlrXdgPopup::WlrXdgPopup() {
	/* Not used */
}

WlrXdgPopup::WlrXdgPopup(struct wlr_xdg_popup *xdg_popup) {
	wlr_xdg_popup = xdg_popup;
	// TOOD: Bind listeners
}

WlrXdgPopup *WlrXdgPopup::from_wlr_xdg_popup(struct wlr_xdg_popup *xdg_popup) {
	WlrXdgSurface *surface = (WlrXdgSurface *)xdg_popup->base->data;
	if (surface->popup) {
		return surface->popup;
	}
	return new WlrXdgPopup(xdg_popup);
}

void WlrXdgPopup::_bind_methods() {
	// TODO: bind classdb
}

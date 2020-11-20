#include <assert.h>
#include "core/func_ref.h"
#include "core/object.h"
#include "scene/main/node.h"
#include "wayland_display.h"
#include "wlr_surface.h"
#include "wlr_xdg_shell.h"
#include <iostream>
extern "C" {
#include <wayland-server.h>
#include <wlr/types/wlr_xdg_shell.h>

void WlrXdgShell::handle_new_surface(
		struct wl_listener *listener, void *data) {
	WlrXdgShell *xdg_shell = wl_container_of(
			listener, xdg_shell, new_surface);
	auto surface = WlrXdgSurface::from_wlr_xdg_surface(
			(struct wlr_xdg_surface *)data);
	//std::cout << "WlrXdgShell::handle_new_surface called w/surface: " << surface << std::endl;
	xdg_shell->emit_signal("new_surface", surface);
}

void WlrXdgShell::handle_destroy(struct wl_listener *listener, void *data) {
	//struct wlr_xdg_shell * xdg_shell = (struct wlr_xdg_shell *)data;
	WlrXdgShell *xdg_shell = wl_container_of(listener, xdg_shell, destroy);
	xdg_shell->emit_signal("destroy", xdg_shell);
}

}

void WlrXdgShell::_bind_methods() {
	ADD_SIGNAL(MethodInfo("new_surface", PropertyInfo(Variant::OBJECT, "surface", PROPERTY_HINT_RESOURCE_TYPE, "WlrXdgSurface")));
	ADD_SIGNAL(MethodInfo("destroy", PropertyInfo(Variant::OBJECT, "xdg_shell", PROPERTY_HINT_RESOURCE_TYPE, "WlrXdgShell")));
}

void WlrXdgShell::ensure_wl_global(WaylandDisplay *display) {
	if (wlr_xdg_shell) {
		return;
	}
	wlr_xdg_shell = wlr_xdg_shell_create(display->get_wayland_display());

	new_surface.notify = handle_new_surface;
	wl_signal_add(&wlr_xdg_shell->events.new_surface, &new_surface);
	destroy.notify = handle_destroy;
	wl_signal_add(&wlr_xdg_shell->events.destroy, &destroy);
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

// Possibly needed: wlr_xdg_surface_for_each_popup, wlr_xdg_surface_for_each_surface
Array WlrXdgSurface::get_children() {
  struct wlr_xdg_popup * xdgp;

  children.clear();

  wl_list_for_each(xdgp, &wlr_xdg_surface->popups, link) {
		//std::cout << "get_children (data, mapped): (" << (xdgp->base->data) << ", " << (xdgp->base->mapped) << ")" << std::endl;
		if (xdgp->base->data && xdgp->base->mapped) {
			WlrXdgSurface * xDGS;
			xDGS = (WlrXdgSurface *)xdgp->base->data;    //Only return children for whome we have WlrXdgSurface formed already
			Variant _xDGS = Variant( (Object *) xDGS );
			children.push_front(_xDGS);
		}
  }

  return children;
}

WlrSurface *WlrXdgSurface::get_wlr_surface() const {
	return WlrSurface::from_wlr_surface(wlr_xdg_surface->surface);
}

extern "C" {

static void for_each_surface_iter(struct wlr_surface *surface,
		int sx, int sy, void *data) {
	FuncRef *func = (FuncRef *)data;
	Variant vSurface(WlrSurface::from_wlr_surface(surface))
		, vSx(sx), vSy(sy);
	const Variant *args[] = { &vSurface, &vSx, &vSy };
	Variant::CallError error;
	func->call_func(&args[0], sizeof(args)/sizeof(args[0]), error);
	if (error.error != Variant::CallError::Error::CALL_OK) {
		printf("call error %d\n", error.error);
	}
}

static void for_each_surface_iter_ffi(struct wlr_surface *surface,
                                  int sx, int sy, void *data) {
  surface_iter_t func = (surface_iter_t)data;

  WlrSurface * wlrSurface = WlrSurface::from_wlr_surface(surface);

  func(wlrSurface, sx, sy);

}

}

void WlrXdgSurface::for_each_surface(Ref<FuncRef> fn) {
	wlr_xdg_surface_for_each_surface(
			wlr_xdg_surface, for_each_surface_iter, fn.ptr());
}

//void WlrXdgSurface::for_each_surface_ffi(surface_iter_t func) {
void WlrXdgSurface::for_each_surface_ffi(void * func) {
	wlr_xdg_surface_for_each_surface(
                                   wlr_xdg_surface,
                                   for_each_surface_iter_ffi,
                                   //(void *) func
                                   func
                                   );
}

WlrSurfaceAtResult *WlrXdgSurface::surface_at(double sx, double sy) {
	double sub_x, sub_y;
	struct wlr_surface *result = wlr_xdg_surface_surface_at(wlr_xdg_surface, sx, sy, &sub_x, &sub_y);
	//struct wlr_surface *result = wlr_surface_surface_at(wlr_xdg_surface->surface, sx, sy, &sub_x, &sub_y);
	return new WlrSurfaceAtResult(
			WlrSurface::from_wlr_surface(result), sub_x, sub_y);
}

void WlrXdgSurface::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_role"), &WlrXdgSurface::get_role);
	ClassDB::bind_method(D_METHOD("get_xdg_toplevel"),
			&WlrXdgSurface::get_xdg_toplevel);
	ClassDB::bind_method(D_METHOD("get_xdg_popup"),
			&WlrXdgSurface::get_xdg_popup);
	ClassDB::bind_method(D_METHOD("get_geometry"),
			&WlrXdgSurface::get_geometry);
	ClassDB::bind_method(D_METHOD("get_children"),
											 &WlrXdgSurface::get_children);
	ClassDB::bind_method(D_METHOD("get_wlr_surface"),
			&WlrXdgSurface::get_wlr_surface);
	ClassDB::bind_method(D_METHOD("for_each_surface", "func"),
			&WlrXdgSurface::for_each_surface);
	ClassDB::bind_method(D_METHOD("for_each_surface_ffi", "func"),
      &WlrXdgSurface::for_each_surface);
	ClassDB::bind_method(D_METHOD("surface_at", "sx", "sy"),
			&WlrXdgSurface::surface_at);

	BIND_ENUM_CONSTANT(XDG_SURFACE_ROLE_NONE);
	BIND_ENUM_CONSTANT(XDG_SURFACE_ROLE_TOPLEVEL);
	BIND_ENUM_CONSTANT(XDG_SURFACE_ROLE_POPUP);

	ADD_SIGNAL(MethodInfo("destroy", PropertyInfo(Variant::OBJECT,
			"xdg_surface", PROPERTY_HINT_RESOURCE_TYPE, "WlrXdgSurface")));
	ADD_SIGNAL(MethodInfo("ping_timeout", PropertyInfo(Variant::OBJECT, "xdg_surface", PROPERTY_HINT_RESOURCE_TYPE, "WlrXdgSurface")));
	ADD_SIGNAL(MethodInfo("new_popup", PropertyInfo(Variant::OBJECT, "xdg_popup", PROPERTY_HINT_RESOURCE_TYPE, "WlrXdgPopup")));
	ADD_SIGNAL(MethodInfo("map", PropertyInfo(Variant::OBJECT, "xdg_surface", PROPERTY_HINT_RESOURCE_TYPE, "WlrXdgSurface")));
	ADD_SIGNAL(MethodInfo("unmap", PropertyInfo(Variant::OBJECT, "xdg_surface", PROPERTY_HINT_RESOURCE_TYPE, "WlrXdgSurface")));
	ADD_SIGNAL(MethodInfo("configure",
												PropertyInfo(Variant::OBJECT, "xdg_surface", PROPERTY_HINT_RESOURCE_TYPE, "WlrXdgSurface"),
												PropertyInfo(Variant::OBJECT, "xdg_toplevel_state", PROPERTY_HINT_RESOURCE_TYPE, "WlrToplevelState"),
												PropertyInfo(Variant::INT, "serial")
												));
	ADD_SIGNAL(MethodInfo("ack_configure",
												PropertyInfo(Variant::OBJECT, "xdg_surface", PROPERTY_HINT_RESOURCE_TYPE, "WlrXdgSurface"),
												PropertyInfo(Variant::OBJECT, "xdg_toplevel_state", PROPERTY_HINT_RESOURCE_TYPE, "WlrToplevelState"),
												PropertyInfo(Variant::INT, "serial")
												));

}

extern "C" {

void WlrXdgSurface::handle_destroy(
		struct wl_listener *listener, void *data) {
	//std::cout << "WlrXdgSurface::handle_destroy called w/XdgSurface: " << xdg_surface << std::endl;
	WlrXdgSurface *xdg_surface = wl_container_of(listener, xdg_surface, destroy);

  wl_list_remove(&xdg_surface->destroy.link);
  wl_list_remove(&xdg_surface->ping_timeout.link);
  wl_list_remove(&xdg_surface->new_popup.link);
  wl_list_remove(&xdg_surface->map.link);
  wl_list_remove(&xdg_surface->unmap.link);
  //wl_list_remove(&xdg_surface->configure.link);
  //wl_list_remove(&xdg_surface->ack_configure.link);

	if(xdg_surface->get_role() == XDG_SURFACE_ROLE_TOPLEVEL) {
		xdg_surface->toplevel->remove_listeners();
	}

	xdg_surface->emit_signal("destroy", xdg_surface);
}

void WlrXdgSurface::handle_map(
		struct wl_listener *listener, void *data) {
	WlrXdgSurface *xdg_surface = wl_container_of(
			listener, xdg_surface, map);
  //std::cout << "WlrXdgSurface::handle_map(..) called w/xdg_surface: " << xdg_surface << std::endl;
	xdg_surface->emit_signal("map", xdg_surface);
}

void WlrXdgSurface::handle_ping_timeout(struct wl_listener *listener, void *data) {
	//std::cout << "WlrXdgSurface::handle_ping_timeout(..) " << std::endl;
	WlrXdgSurface *xdg_surface = wl_container_of(listener, xdg_surface, ping_timeout);
	xdg_surface->emit_signal("ping_timeout", xdg_surface);
}

// Don't include the configure handlers for now
// void WlrXdgSurface::handle_configure(struct wl_listener *listener, void *data) {
// 	std::cout << "WlrXdgSurface::handle_configure(..) " << std::endl;
// 	WlrXdgSurface *xdg_surface = wl_container_of(listener, xdg_surface, ping_timeout);
// 	xdg_surface->emit_signal("configure", xdg_surface, serial, wlrToplevelState);
// }

// void WlrXdgSurface::handle_ack_configure(struct wl_listener *listener, void *data) {
// 	std::cout << "WlrXdgSurface::handle_configure(..) " << std::endl;
// 	WlrXdgSurface *xdg_surface = wl_container_of(listener, xdg_surface, ping_timeout);
// 	xdg_surface->emit_signal("configure", xdg_surface, serial, wlrToplevelState);
// }

void WlrXdgSurface::handle_new_popup(struct wl_listener *listener, void *data) {
	WlrXdgSurface *xdg_surface = wl_container_of(listener, xdg_surface, new_popup);
	// std::cout << "WlrXdgSurface::handle_new_popup called w/xdg_surface: " << xdg_surface << " and popup: " << xdg_surface->popup << std::endl;
	xdg_surface->emit_signal("new_popup", xdg_surface->popup);
}

void WlrXdgSurface::handle_unmap(
		struct wl_listener *listener, void *data) {
	WlrXdgSurface *xdg_surface = wl_container_of(listener, xdg_surface, unmap);
  //std::cout << "WlrXdgSurface::handle_unmap(..) called w/xdg_surface: " << xdg_surface << std::endl;
	xdg_surface->emit_signal("unmap", xdg_surface);
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
	new_popup.notify = handle_new_popup;
	wl_signal_add(&xdg_surface->events.new_popup, &new_popup);
	map.notify = handle_map;
	wl_signal_add(&xdg_surface->events.map, &map);
	unmap.notify = handle_unmap;
	wl_signal_add(&xdg_surface->events.unmap, &unmap);
	ping_timeout.notify = handle_unmap;
	wl_signal_add(&xdg_surface->events.ping_timeout, &ping_timeout);
	// configure.notify = handle_unmap;
	// wl_signal_add(&xdg_surface->events.configure, &configure);
	// ack_configure.notify = handle_unmap;
	// wl_signal_add(&xdg_surface->events.ack_configure, &ack_configure);

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


void WlrXdgToplevel::remove_listeners() {
	wl_list_remove(&request_maximize.link);
	wl_list_remove(&request_fullscreen.link);
	wl_list_remove(&request_minimize.link);
	wl_list_remove(&request_move.link);
	wl_list_remove(&request_resize.link);
	wl_list_remove(&request_show_window_menu.link);
	wl_list_remove(&set_parent.link);
	wl_list_remove(&set_title.link);
	wl_list_remove(&set_app_id.link);
}


void WlrXdgToplevel::handle_request_maximize(
		struct wl_listener *listener, void *data) {
	WlrXdgToplevel *xdg_toplevel = wl_container_of(
			listener, xdg_toplevel, request_maximize);
	xdg_toplevel->emit_signal("request_maximize", xdg_toplevel);
}

void WlrXdgToplevel::handle_request_fullscreen(
		struct wl_listener *listener, void *data) {
	WlrXdgToplevel *xdg_toplevel = wl_container_of(
			listener, xdg_toplevel, request_fullscreen);
	struct wlr_xdg_toplevel_set_fullscreen_event *event =
		(struct wlr_xdg_toplevel_set_fullscreen_event *)data;
	// TODO: Implement WlrOutput::from_wlr_output
	xdg_toplevel->emit_signal("request_fullscreen",
			xdg_toplevel, event->fullscreen);
}

void WlrXdgToplevel::handle_request_minimize(
		struct wl_listener *listener, void *data) {
	WlrXdgToplevel *xdg_toplevel = wl_container_of(
			listener, xdg_toplevel, request_minimize);
	xdg_toplevel->emit_signal("request_minimize", xdg_toplevel);
}

void WlrXdgToplevel::handle_request_move(
		struct wl_listener *listener, void *data) {
	WlrXdgToplevel *xdg_toplevel = wl_container_of(
			listener, xdg_toplevel, request_move);
	struct wlr_xdg_toplevel_move_event *event =
		(struct wlr_xdg_toplevel_move_event *)data;
	xdg_toplevel->emit_signal("request_move", xdg_toplevel, event->serial);
}

void WlrXdgToplevel::handle_request_resize(
		struct wl_listener *listener, void *data) {
	WlrXdgToplevel *xdg_toplevel = wl_container_of(
			listener, xdg_toplevel, request_resize);
	struct wlr_xdg_toplevel_resize_event *event =
		(struct wlr_xdg_toplevel_resize_event *)data;
	xdg_toplevel->emit_signal("request_resize", xdg_toplevel,
			event->serial, event->edges);
}

void WlrXdgToplevel::handle_request_show_window_menu(
		struct wl_listener *listener, void *data) {
	WlrXdgToplevel *xdg_toplevel = wl_container_of(
			listener, xdg_toplevel, request_show_window_menu);
	struct wlr_xdg_toplevel_show_window_menu_event *event =
		(struct wlr_xdg_toplevel_show_window_menu_event *)data;
	//std::cout << "WlrXdgTopLevel::handle_request_show_window_menu" << std::endl;
	xdg_toplevel->emit_signal("request_show_window_menu", xdg_toplevel,
			event->serial, event->x, event->y);
}

void WlrXdgToplevel::handle_set_parent(
		struct wl_listener *listener, void *data) {
	//std::cout << "WlrXdgTopLevel::handle_set_parent" << std::endl;
	WlrXdgToplevel *xdg_toplevel = wl_container_of(listener, xdg_toplevel, set_parent);
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
	request_maximize.notify = handle_request_maximize;
	wl_signal_add(&wlr_xdg_toplevel->events.request_maximize,
			&request_maximize);
	request_fullscreen.notify = handle_request_fullscreen;
	wl_signal_add(&wlr_xdg_toplevel->events.request_fullscreen,
			&request_fullscreen);
	request_minimize.notify = handle_request_minimize;
	wl_signal_add(&wlr_xdg_toplevel->events.request_minimize,
			&request_minimize);
	request_move.notify = handle_request_move;
	wl_signal_add(&wlr_xdg_toplevel->events.request_move, &request_move);
	request_resize.notify = handle_request_resize;
	wl_signal_add(&wlr_xdg_toplevel->events.request_resize, &request_resize);
	request_show_window_menu.notify = handle_request_show_window_menu;
	wl_signal_add(&wlr_xdg_toplevel->events.request_show_window_menu,
			&request_show_window_menu);
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

extern "C" {
void WlrXdgToplevel::set_size(Vector2 size) {
	wlr_xdg_toplevel_set_size(wlr_xdg_toplevel->base, size.width, size.height);
}
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
	wlr_xdg_toplevel_set_tiled(wlr_xdg_toplevel->base, tiled );
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

	ADD_SIGNAL(MethodInfo("request_maximize",
			PropertyInfo(Variant::OBJECT,
				"xdg_toplevel", PROPERTY_HINT_RESOURCE_TYPE, "WlrXdgToplevel")));
	ADD_SIGNAL(MethodInfo("request_fullscreen",
			PropertyInfo(Variant::OBJECT,
				"xdg_toplevel", PROPERTY_HINT_RESOURCE_TYPE, "WlrXdgToplevel"),
			PropertyInfo(Variant::BOOL, "fullscreen")));
	ADD_SIGNAL(MethodInfo("request_minimize",
			PropertyInfo(Variant::OBJECT,
				"xdg_toplevel", PROPERTY_HINT_RESOURCE_TYPE, "WlrXdgToplevel")));
	ADD_SIGNAL(MethodInfo("request_move",
			PropertyInfo(Variant::OBJECT,
				"xdg_toplevel", PROPERTY_HINT_RESOURCE_TYPE, "WlrXdgToplevel"),
			PropertyInfo(Variant::INT, "serial")));
	ADD_SIGNAL(MethodInfo("request_resize",
			PropertyInfo(Variant::OBJECT,
				"xdg_toplevel", PROPERTY_HINT_RESOURCE_TYPE, "WlrXdgToplevel"),
			PropertyInfo(Variant::INT, "serial"),
			PropertyInfo(Variant::INT, "edges")));
	ADD_SIGNAL(MethodInfo("request_show_window_menu",
			PropertyInfo(Variant::OBJECT,
				"xdg_toplevel", PROPERTY_HINT_RESOURCE_TYPE, "WlrXdgToplevel"),
			PropertyInfo(Variant::INT, "serial"),
			PropertyInfo(Variant::INT, "x"),
			PropertyInfo(Variant::INT, "y")));
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

bool WlrXdgToplevelState::get_maximized() const {
	return state->maximized;
}

bool WlrXdgToplevelState::get_fullscreen() const {
	return state->fullscreen;
}

bool WlrXdgToplevelState::get_resizing() const {
	return state->resizing;
}

bool WlrXdgToplevelState::get_activated() const {
	return state->activated;
}

bool WlrXdgToplevelState::get_tiled() const {
	return state->tiled;
}

uint32_t WlrXdgToplevelState::get_width() const {
	return state->width;
}

uint32_t WlrXdgToplevelState::get_height() const {
	return state->height;
}

uint32_t WlrXdgToplevelState::get_min_width() const {
	return state->min_width;
}

uint32_t WlrXdgToplevelState::get_min_height() const {
	return state->min_height;
}

uint32_t WlrXdgToplevelState::get_max_width() const {
	return state->max_width;
}

uint32_t WlrXdgToplevelState::get_max_height() const {
	return state->max_height;
}

WlrXdgToplevelState::WlrXdgToplevelState() {
	/* Not used */
}

void WlrXdgToplevelState::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_maximized"),
			&WlrXdgToplevelState::get_maximized);
	ClassDB::bind_method(D_METHOD("get_fullscreen"),
			&WlrXdgToplevelState::get_fullscreen);
	ClassDB::bind_method(D_METHOD("get_resizing"),
			&WlrXdgToplevelState::get_resizing);
	ClassDB::bind_method(D_METHOD("get_activated"),
			&WlrXdgToplevelState::get_activated);
	ClassDB::bind_method(D_METHOD("get_tiled"),
			&WlrXdgToplevelState::get_tiled);
	ClassDB::bind_method(D_METHOD("get_width"),
			&WlrXdgToplevelState::get_width);
	ClassDB::bind_method(D_METHOD("get_height"),
			&WlrXdgToplevelState::get_height);
	ClassDB::bind_method(D_METHOD("get_min_width"),
			&WlrXdgToplevelState::get_min_width);
	ClassDB::bind_method(D_METHOD("get_min_height"),
			&WlrXdgToplevelState::get_min_height);
	ClassDB::bind_method(D_METHOD("get_max_width"),
			&WlrXdgToplevelState::get_max_width);
	ClassDB::bind_method(D_METHOD("get_max_height"),
			&WlrXdgToplevelState::get_max_height);
}

WlrXdgPopup::WlrXdgPopup() {
	/* Not used */
}

WlrXdgPopup::WlrXdgPopup(struct wlr_xdg_popup *xdg_popup) {
	wlr_xdg_popup = xdg_popup;
	//wlr_xdg_popup->data = this; //doesn't exist
	// TOOD: Bind listeners
}

Rect2 WlrXdgPopup::get_geometry() {
	return Rect2(wlr_xdg_popup->geometry.x, wlr_xdg_popup->geometry.y,
							 wlr_xdg_popup->geometry.width, wlr_xdg_popup->geometry.height);
}

int WlrXdgPopup::get_x() {
	return wlr_xdg_popup->geometry.x;
}

int WlrXdgPopup::get_y() {
	return wlr_xdg_popup->geometry.y;
}

int WlrXdgPopup::get_width() {
	return wlr_xdg_popup->geometry.width;
}

int WlrXdgPopup::get_height() {
	return wlr_xdg_popup->geometry.height;
}

WlrXdgPopup *WlrXdgPopup::from_wlr_xdg_popup(struct wlr_xdg_popup *xdg_popup) {
	WlrXdgSurface *surface = (WlrXdgSurface *)xdg_popup->base->data;
	if (surface->popup) {
		return surface->popup;
	}
	return new WlrXdgPopup(xdg_popup);
}

void WlrXdgPopup::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_x"),
											 &WlrXdgPopup::get_x);
	ClassDB::bind_method(D_METHOD("get_y"),
											 &WlrXdgPopup::get_y);
	ClassDB::bind_method(D_METHOD("get_height"),
											 &WlrXdgPopup::get_height);
	ClassDB::bind_method(D_METHOD("get_width"),
											 &WlrXdgPopup::get_width);

	ClassDB::bind_method(D_METHOD("get_geometry"),
					&WlrXdgSurface::get_geometry);
	// TODO: bind classdb
}

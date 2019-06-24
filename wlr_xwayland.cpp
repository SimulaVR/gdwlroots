#include <assert.h>
#include "core/func_ref.h"
#include "core/object.h"
#include "scene/main/node.h"
#include "wayland_display.h"
#include "wlr_surface.h"
#include "wlr_xwayland.h"
#include "wlr_compositor.h"
#include <iostream>

extern "C" {
#include <wayland-server.h>
//#include <wlr/xwayland.h>
#include "xwayland.h"

void WlrXWayland::handle_new_xwayland_surface(
		struct wl_listener *listener, void *data) {
  WlrXWayland *xwayland = wl_container_of(
			listener, xwayland, new_xwayland_surface);
	auto surface = WlrXWaylandSurface::from_wlr_xwayland_surface(
      (struct wlr_xwayland_surface *)data);
	xwayland->emit_signal("new_surface", surface);
}

}

void WlrXWayland::_bind_methods() {
	ADD_SIGNAL(MethodInfo("new_surface",
				PropertyInfo(Variant::OBJECT,
					"surface", PROPERTY_HINT_RESOURCE_TYPE, "WlrXWaylandSurface")));
}

void WlrXWayland::start_xwayland(WlrCompositor * compositor) {
	if (wlr_xwayland) {
    std::cout << "Xwayland is already started." << std::endl;
		return;
	}

  struct wlr_compositor * wlr_compositor = compositor->get_wlr_compositor();
  struct wl_display * wl_display = get_wayland_display()->get_wayland_display();

  if (wl_display && wlr_compositor) {
    wlr_xwayland = wlr_xwayland_create(wl_display, wlr_compositor, true); //`true` forces XWayland to start in lazy mode

    new_xwayland_surface.notify = handle_new_xwayland_surface;
		wl_signal_add(&wlr_xwayland->events.new_surface,
                  &new_xwayland_surface
                  );

    //Things we omit from XWayland initialization:
    //1. Adjusting DISPLAY environment variable
    //2. xcursor stuff
    //See i.e. https://github.com/swaywm/wlroots/blob/b3f42548d068996995490585e27e16c191b4a64c/rootston/desktop.c#L358

  } else {
    std::cout << "Failed to start xwayland." << std::endl;
  }
}

void WlrXWayland::ensure_wl_global(WaylandDisplay *display) {
  //This function (automatically called by WaylandGlobal) intentionally left blank.
  // We use start_xwayland (called from GDScript/godot-haskell) instead so we can gain access to a WlrCompositor
}

void WlrXWayland::destroy_wl_global(WaylandDisplay *display) {
	wlr_xwayland_destroy(wlr_xwayland);
	wlr_xwayland = NULL;
}

WlrXWayland::WlrXWayland() {
	wlr_xwayland = NULL;
}

WlrXWayland::~WlrXWayland() {
	wlr_xwayland_destroy(wlr_xwayland);
	wlr_xwayland = NULL;
}

Rect2 WlrXWaylandSurface::get_geometry() {
	return Rect2(wlr_xwayland_surface->x,
               wlr_xwayland_surface->y,
               wlr_xwayland_surface->width,
               wlr_xwayland_surface->height);
}

WlrSurface *WlrXWaylandSurface::get_wlr_surface() const {
	return WlrSurface::from_wlr_surface(wlr_xwayland_surface->surface);
}

extern "C" {

static void for_each_surface_iter(struct wlr_surface *surface,
		int sx, int sy, void *data) {
	FuncRef *func = (FuncRef *)data;
	const Variant *args[] = {
		new Variant(WlrSurface::from_wlr_surface(surface)),
		new Variant(sx),
		new Variant(sy),
	};
	Variant::CallError error;
	func->call_func((const Variant **)&args[0], 3, error);
	if (error.error != Variant::CallError::Error::CALL_OK) {
		printf("call error %d\n", error.error);
	}
}

//We omit implementing this due to lack of clearly usable
// `*xwayland_children_for_each_surface`
// void WlrXWaylandSurface::for_each_surface(Variant func) {
// 	auto fn = (Ref<FuncRef>)func;
// 	wlr_xwayland_surface_for_each_surface(
// 			wlr_xwayland_surface, for_each_surface_iter, fn.ptr());
// }

WlrSurfaceAtResult *WlrXWaylandSurface::surface_at(double sx, double sy) {
  auto surface = wlr_xwayland_surface->surface;
  return new WlrSurfaceAtResult(WlrSurface::from_wlr_surface(surface), sx, sy);
}

extern "C" {

void WlrXWaylandSurface::handle_destroy(
		struct wl_listener *listener, void *data) {
	WlrXWaylandSurface *xwayland_surface = wl_container_of(
			listener, xwayland_surface, destroy);
	xwayland_surface->emit_signal("destroy", xwayland_surface);
}

void WlrXWaylandSurface::handle_map(
		struct wl_listener *listener, void *data) {
	WlrXWaylandSurface *xwayland_surface = wl_container_of(
			listener, xwayland_surface, map);
	xwayland_surface->emit_signal("map", xwayland_surface);
}

void WlrXWaylandSurface::handle_unmap(
		struct wl_listener *listener, void *data) {
	WlrXWaylandSurface *xwayland_surface = wl_container_of(
			listener, xwayland_surface, unmap);
	xwayland_surface->emit_signal("unmap", xwayland_surface);
}

}

// WlrXdgSurface::WlrXdgSurface() {
// 	/* Not used */
// }

// WlrXdgSurface::WlrXdgSurface(struct wlr_xdg_surface *xdg_surface) {
// 	wlr_xdg_surface = xdg_surface;
// 	xdg_surface->data = this;
// 	destroy.notify = handle_destroy;
// 	wl_signal_add(&xdg_surface->events.destroy, &destroy);
// 	map.notify = handle_map;
// 	wl_signal_add(&xdg_surface->events.map, &map);
// 	unmap.notify = handle_unmap;
// 	wl_signal_add(&xdg_surface->events.unmap, &unmap);
// 	switch (xdg_surface->role) {
// 	case WLR_XDG_SURFACE_ROLE_TOPLEVEL:
// 		toplevel = WlrXdgToplevel::from_wlr_xdg_toplevel(
// 				wlr_xdg_surface->toplevel);
// 		break;
// 	case WLR_XDG_SURFACE_ROLE_POPUP:
// 		popup = WlrXdgPopup::from_wlr_xdg_popup(wlr_xdg_surface->popup);
// 		break;
// 	case WLR_XDG_SURFACE_ROLE_NONE:
// 		assert(0);
// 		break;
// 	}
// }

WlrXWaylandSurface *WlrXWaylandSurface::from_wlr_xwayland_surface(
		struct wlr_xwayland_surface *xwayland_surface) {
	if (xwayland_surface->data) {
		return (WlrXWaylandSurface *)xwayland_surface->data;
	}
	return new WlrXWaylandSurface(xwayland_surface);
}

// extern "C" {

void WlrXWaylandSurface::handle_request_maximize(
		struct wl_listener *listener, void *data) {
	WlrXWaylandSurface *xwayland_surface = wl_container_of(
			listener, xwayland_surface, request_maximize);
	xwayland_surface->emit_signal("request_maximize", xwayland_surface);
}

// void WlrXdgToplevel::handle_request_fullscreen(
// 		struct wl_listener *listener, void *data) {
// 	WlrXdgToplevel *xdg_toplevel = wl_container_of(
// 			listener, xdg_toplevel, request_fullscreen);
// 	struct wlr_xdg_toplevel_set_fullscreen_event *event =
// 		(struct wlr_xdg_toplevel_set_fullscreen_event *)data;
// 	// TODO: Implement WlrOutput::from_wlr_output
// 	xdg_toplevel->emit_signal("request_fullscreen",
// 			xdg_toplevel, event->fullscreen);
// }

// void WlrXdgToplevel::handle_request_minimize(
// 		struct wl_listener *listener, void *data) {
// 	WlrXdgToplevel *xdg_toplevel = wl_container_of(
// 			listener, xdg_toplevel, request_minimize);
// 	xdg_toplevel->emit_signal("request_minimize", xdg_toplevel);
// }

// void WlrXdgToplevel::handle_request_move(
// 		struct wl_listener *listener, void *data) {
// 	WlrXdgToplevel *xdg_toplevel = wl_container_of(
// 			listener, xdg_toplevel, request_move);
// 	struct wlr_xdg_toplevel_move_event *event =
// 		(struct wlr_xdg_toplevel_move_event *)data;
// 	xdg_toplevel->emit_signal("request_move", xdg_toplevel, event->serial);
// }

// void WlrXdgToplevel::handle_request_resize(
// 		struct wl_listener *listener, void *data) {
// 	WlrXdgToplevel *xdg_toplevel = wl_container_of(
// 			listener, xdg_toplevel, request_resize);
// 	struct wlr_xdg_toplevel_resize_event *event =
// 		(struct wlr_xdg_toplevel_resize_event *)data;
// 	xdg_toplevel->emit_signal("request_resize", xdg_toplevel,
// 			event->serial, event->edges);
// }

// void WlrXdgToplevel::handle_request_show_window_menu(
// 		struct wl_listener *listener, void *data) {
// 	WlrXdgToplevel *xdg_toplevel = wl_container_of(
// 			listener, xdg_toplevel, request_show_window_menu);
// 	struct wlr_xdg_toplevel_show_window_menu_event *event =
// 		(struct wlr_xdg_toplevel_show_window_menu_event *)data;
// 	xdg_toplevel->emit_signal("request_show_window_menu", xdg_toplevel,
// 			event->serial, event->x, event->y);
// }

// void WlrXdgToplevel::handle_set_parent(
// 		struct wl_listener *listener, void *data) {
// 	WlrXdgToplevel *xdg_toplevel = wl_container_of(
// 			listener, xdg_toplevel, set_parent);
// 	xdg_toplevel->emit_signal("set_parent", xdg_toplevel);
// }

// void WlrXdgToplevel::handle_set_title(
// 		struct wl_listener *listener, void *data) {
// 	WlrXdgToplevel *xdg_toplevel = wl_container_of(
// 			listener, xdg_toplevel, set_title);
// 	xdg_toplevel->emit_signal("set_title", xdg_toplevel);
// }

// void WlrXdgToplevel::handle_set_app_id(
// 		struct wl_listener *listener, void *data) {
// 	WlrXdgToplevel *xdg_toplevel = wl_container_of(
// 			listener, xdg_toplevel, set_app_id);
// 	xdg_toplevel->emit_signal("set_app_id", xdg_toplevel);
// }

// }

// WlrXdgToplevel::WlrXdgToplevel() {
// 	/* Not used */
// }

WlrXWaylandSurface::WlrXWaylandSurface(struct wlr_xwayland_surface *xwayland_surface) {
	wlr_xwayland_surface = xwayland_surface;
	request_maximize.notify = handle_request_maximize;
	wl_signal_add(&wlr_xwayland_surface->events.request_maximize,
			&request_maximize);
	// request_fullscreen.notify = handle_request_fullscreen;
	// wl_signal_add(&wlr_xdg_toplevel->events.request_fullscreen,
	// 		&request_fullscreen);
	// request_minimize.notify = handle_request_minimize;
	// wl_signal_add(&wlr_xdg_toplevel->events.request_minimize,
	// 		&request_minimize);
	// request_move.notify = handle_request_move;
	// wl_signal_add(&wlr_xdg_toplevel->events.request_move, &request_move);
	// request_resize.notify = handle_request_resize;
	// wl_signal_add(&wlr_xdg_toplevel->events.request_resize, &request_resize);
	// request_show_window_menu.notify = handle_request_show_window_menu;
	// wl_signal_add(&wlr_xdg_toplevel->events.request_show_window_menu,
	// 		&request_show_window_menu);
	// set_parent.notify = handle_set_parent;
	// wl_signal_add(&wlr_xdg_toplevel->events.set_parent, &set_parent);
	// set_title.notify = handle_set_title;
	// wl_signal_add(&wlr_xdg_toplevel->events.set_title, &set_title);
	// set_app_id.notify = handle_set_app_id;
	// wl_signal_add(&wlr_xdg_toplevel->events.set_app_id, &set_app_id);
}

// WlrXdgToplevel *WlrXdgToplevel::from_wlr_xdg_toplevel(
// 		struct wlr_xdg_toplevel *xdg_toplevel) {
// 	WlrXdgSurface *surface = (WlrXdgSurface *)xdg_toplevel->base->data;
// 	if (surface->toplevel) {
// 		return surface->toplevel;
// 	}
// 	return new WlrXdgToplevel(xdg_toplevel);
// }

// WlrXdgToplevel *WlrXdgToplevel::get_parent() const {
// 	return from_wlr_xdg_toplevel(wlr_xdg_toplevel->parent->toplevel);
// }

// String WlrXdgToplevel::get_app_id() const {
// 	return String(wlr_xdg_toplevel->app_id);
// }

// String WlrXdgToplevel::get_title() const {
// 	return String(wlr_xdg_toplevel->title);
// }

// void WlrXdgToplevel::set_size(Vector2 size) {
// 	wlr_xdg_toplevel_set_size(wlr_xdg_toplevel->base, size.width, size.height);
// }

// void WlrXdgToplevel::set_activated(bool activated) {
// 	wlr_xdg_toplevel_set_activated(wlr_xdg_toplevel->base, activated);
// }

void WlrXWaylandSurface::set_maximized(bool maximized) {
	wlr_xwayland_surface_set_maximized(wlr_xwayland_surface, maximized);
}

// void WlrXdgToplevel::set_fullscreen(bool fullscreen) {
// 	wlr_xdg_toplevel_set_fullscreen(wlr_xdg_toplevel->base, fullscreen);
// }

// void WlrXdgToplevel::set_resizing(bool resizing) {
// 	wlr_xdg_toplevel_set_resizing(wlr_xdg_toplevel->base, resizing);
// }

// void WlrXdgToplevel::set_tiled(bool tiled) {
// 	wlr_xdg_toplevel_set_tiled(wlr_xdg_toplevel->base, tiled);
// }

// void WlrXdgToplevel::send_close() {
// 	wlr_xdg_toplevel_send_close(wlr_xdg_toplevel->base);
// }

void WlrXWaylandSurface::_bind_methods() {
// 	ClassDB::bind_method(D_METHOD("get_parent"), &WlrXdgToplevel::get_parent);
// 	ClassDB::bind_method(D_METHOD("get_title"), &WlrXdgToplevel::get_title);
// 	ClassDB::bind_method(D_METHOD("get_app_id"), &WlrXdgToplevel::get_app_id);
// 	ClassDB::bind_method(D_METHOD("set_size", "size"),
// 			&WlrXdgToplevel::set_size);
// 	ClassDB::bind_method(D_METHOD("set_activated", "activated"),
// 			&WlrXdgToplevel::set_activated);
	ClassDB::bind_method(D_METHOD("get_maximized"),
                       &WlrXWaylandSurface::get_maximized);
	ClassDB::bind_method(D_METHOD("set_maximized", "maximized"),
			&WlrXWaylandSurface::set_maximized);
// 	ClassDB::bind_method(D_METHOD("set_fullscreen", "fullscreen"),
// 			&WlrXdgToplevel::set_fullscreen);
// 	ClassDB::bind_method(D_METHOD("set_resizing", "resizing"),
// 			&WlrXdgToplevel::set_resizing);
// 	ClassDB::bind_method(D_METHOD("set_tiled", "tiled"),
// 			&WlrXdgToplevel::set_tiled);
// 	ClassDB::bind_method(D_METHOD("send_close"), &WlrXdgToplevel::send_close);

	ADD_SIGNAL(MethodInfo("request_maximize",
			PropertyInfo(Variant::OBJECT,
				"xwayland_surface", PROPERTY_HINT_RESOURCE_TYPE, "WlrXWaylandSurface")));

	ADD_SIGNAL(MethodInfo("destroy", PropertyInfo(Variant::OBJECT,
                                                "xwayland_surface", PROPERTY_HINT_RESOURCE_TYPE, "WlrXWaylandSurface")));
	ADD_SIGNAL(MethodInfo("map", PropertyInfo(Variant::OBJECT,
                                            "xwayland_surface", PROPERTY_HINT_RESOURCE_TYPE, "WlrXWaylandSurface")));
	ADD_SIGNAL(MethodInfo("unmap", PropertyInfo(Variant::OBJECT,
                                              "xwayland_surface", PROPERTY_HINT_RESOURCE_TYPE, "WlrXWaylandSurface")));


// 	ADD_SIGNAL(MethodInfo("request_fullscreen",
// 			PropertyInfo(Variant::OBJECT,
// 				"xdg_toplevel", PROPERTY_HINT_RESOURCE_TYPE, "WlrXdgToplevel"),
// 			PropertyInfo(Variant::BOOL, "fullscreen")));
// 	ADD_SIGNAL(MethodInfo("request_minimize",
// 			PropertyInfo(Variant::OBJECT,
// 				"xdg_toplevel", PROPERTY_HINT_RESOURCE_TYPE, "WlrXdgToplevel")));
// 	ADD_SIGNAL(MethodInfo("request_move",
// 			PropertyInfo(Variant::OBJECT,
// 				"xdg_toplevel", PROPERTY_HINT_RESOURCE_TYPE, "WlrXdgToplevel"),
// 			PropertyInfo(Variant::INT, "serial")));
// 	ADD_SIGNAL(MethodInfo("request_resize",
// 			PropertyInfo(Variant::OBJECT,
// 				"xdg_toplevel", PROPERTY_HINT_RESOURCE_TYPE, "WlrXdgToplevel"),
// 			PropertyInfo(Variant::INT, "serial"),
// 			PropertyInfo(Variant::INT, "edges")));
// 	ADD_SIGNAL(MethodInfo("request_show_window_menu",
// 			PropertyInfo(Variant::OBJECT,
// 				"xdg_toplevel", PROPERTY_HINT_RESOURCE_TYPE, "WlrXdgToplevel"),
// 			PropertyInfo(Variant::INT, "serial"),
// 			PropertyInfo(Variant::INT, "x"),
// 			PropertyInfo(Variant::INT, "y")));
// 	ADD_SIGNAL(MethodInfo("set_app_id",
// 			PropertyInfo(Variant::OBJECT,
// 				"xdg_toplevel", PROPERTY_HINT_RESOURCE_TYPE, "WlrXdgToplevel")));
// 	ADD_SIGNAL(MethodInfo("set_title",
// 			PropertyInfo(Variant::OBJECT,
// 				"xdg_toplevel", PROPERTY_HINT_RESOURCE_TYPE, "WlrXdgToplevel")));
// 	ADD_SIGNAL(MethodInfo("set_parent",
// 			PropertyInfo(Variant::OBJECT,
// 				"xdg_toplevel", PROPERTY_HINT_RESOURCE_TYPE, "WlrXdgToplevel")));
}

bool WlrXWaylandSurface::get_maximized() const {
	bool maximized = wlr_xwayland_surface->maximized_vert && wlr_xwayland_surface->maximized_horz;
	return maximized;
}

// bool WlrXdgToplevelState::get_fullscreen() const {
// 	return state->fullscreen;
// }

// bool WlrXdgToplevelState::get_resizing() const {
// 	return state->resizing;
// }

// bool WlrXdgToplevelState::get_activated() const {
// 	return state->activated;
// }

// bool WlrXdgToplevelState::get_tiled() const {
// 	return state->tiled;
// }

// uint32_t WlrXdgToplevelState::get_width() const {
// 	return state->width;
// }

// uint32_t WlrXdgToplevelState::get_height() const {
// 	return state->height;
// }

// uint32_t WlrXdgToplevelState::get_min_width() const {
// 	return state->min_width;
// }

// uint32_t WlrXdgToplevelState::get_min_height() const {
// 	return state->min_height;
// }

// uint32_t WlrXdgToplevelState::get_max_width() const {
// 	return state->max_width;
// }

// uint32_t WlrXdgToplevelState::get_max_height() const {
// 	return state->max_height;
// }

// WlrXdgToplevelState::WlrXdgToplevelState() {
// 	/* Not used */
// }

// void WlrXWaylandSurface::_bind_methods() {
	// ClassDB::bind_method(D_METHOD("get_fullscreen"),
	// 		&WlrXdgToplevelState::get_fullscreen);
	// ClassDB::bind_method(D_METHOD("get_resizing"),
	// 		&WlrXdgToplevelState::get_resizing);
	// ClassDB::bind_method(D_METHOD("get_activated"),
	// 		&WlrXdgToplevelState::get_activated);
	// ClassDB::bind_method(D_METHOD("get_tiled"),
	// 		&WlrXdgToplevelState::get_tiled);
	// ClassDB::bind_method(D_METHOD("get_width"),
	// 		&WlrXdgToplevelState::get_width);
	// ClassDB::bind_method(D_METHOD("get_height"),
	// 		&WlrXdgToplevelState::get_height);
	// ClassDB::bind_method(D_METHOD("get_min_width"),
	// 		&WlrXdgToplevelState::get_min_width);
	// ClassDB::bind_method(D_METHOD("get_min_height"),
	// 		&WlrXdgToplevelState::get_min_height);
	// ClassDB::bind_method(D_METHOD("get_max_width"),
	// 		&WlrXdgToplevelState::get_max_width);
	// ClassDB::bind_method(D_METHOD("get_max_height"),
	// 		&WlrXdgToplevelState::get_max_height);
// }

// WlrXdgPopup::WlrXdgPopup() {
// 	/* Not used */
// }

// WlrXdgPopup::WlrXdgPopup(struct wlr_xdg_popup *xdg_popup) {
// 	wlr_xdg_popup = xdg_popup;
// 	// TOOD: Bind listeners
// }

// WlrXdgPopup *WlrXdgPopup::from_wlr_xdg_popup(struct wlr_xdg_popup *xdg_popup) {
// 	WlrXdgSurface *surface = (WlrXdgSurface *)xdg_popup->base->data;
// 	if (surface->popup) {
// 		return surface->popup;
// 	}
// 	return new WlrXdgPopup(xdg_popup);
// }

// void WlrXdgPopup::_bind_methods() {
// 	// TODO: bind classdb
//}
}

// void WlrXdgSurface::_bind_methods() {
// 	ClassDB::bind_method(D_METHOD("get_role"), &WlrXdgSurface::get_role);
// 	ClassDB::bind_method(D_METHOD("get_xdg_toplevel"),
// 			&WlrXdgSurface::get_xdg_toplevel);
// 	ClassDB::bind_method(D_METHOD("get_xdg_popup"),
// 			&WlrXdgSurface::get_xdg_popup);
// 	ClassDB::bind_method(D_METHOD("get_geometry"),
// 			&WlrXdgSurface::get_geometry);
// 	ClassDB::bind_method(D_METHOD("get_wlr_surface"),
// 			&WlrXdgSurface::get_wlr_surface);
// 	ClassDB::bind_method(D_METHOD("for_each_surface", "func"),
// 			&WlrXdgSurface::for_each_surface);
// 	ClassDB::bind_method(D_METHOD("for_each_surface_ffi", "func"),
//       &WlrXdgSurface::for_each_surface);
// 	ClassDB::bind_method(D_METHOD("surface_at", "sx", "sy"),
// 			&WlrXdgSurface::surface_at);

// 	BIND_ENUM_CONSTANT(XDG_SURFACE_ROLE_NONE);
// 	BIND_ENUM_CONSTANT(XDG_SURFACE_ROLE_TOPLEVEL);
// 	BIND_ENUM_CONSTANT(XDG_SURFACE_ROLE_POPUP);

// }

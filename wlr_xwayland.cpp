#include <assert.h>
#include "core/func_ref.h"
#include "core/object.h"
#include "scene/main/node.h"
#include "wayland_display.h"
#include "wlr_surface.h"
#include "wlr_xwayland.h"
#include "wlr_compositor.h"
#include "wlr_output.h"
#include "wlr_seat.h"

#include <unistd.h>
#include <iostream>

extern "C" {
#include <time.h>
#include <wayland-server.h>
#include <wlr/xcursor.h>
#include <wlr/types/wlr_xcursor_manager.h>
#include "wlr/interfaces/wlr_output.h"
#include "wlr/xwayland.h"
#include <signal.h>
#include <wayland-server.h>
#include <wayland-util.h>

//We override xwayland.h to avoid the `class` keyword
#include "xwayland.h" //as opposed to: <wlr/xwayland.h>

#include <xcb/composite.h>
#include <xcb/render.h>
#include <xcb/xfixes.h>
#include <xcb/xproto.h>
}

void WlrXWaylandSurface::handle_request_configure(struct wl_listener *listener, void *data) {
  struct wlr_xwayland_surface_configure_event * event = (wlr_xwayland_surface_configure_event *) data;

  WlrXWaylandSurface *xwayland_surface = wl_container_of(listener, xwayland_surface, request_configure);

  wlr_xwayland_surface_configure(xwayland_surface->wlr_xwayland_surface, 0, 0,
                                 event->width, event->height);
}

void WlrXWayland::handle_new_xwayland_surface(
		struct wl_listener *listener, void *data) {
  WlrXWayland *xwayland = wl_container_of(
			listener, xwayland, new_xwayland_surface);
	auto surface = WlrXWaylandSurface::from_wlr_xwayland_surface(
      (struct wlr_xwayland_surface *)data);

	xwayland->emit_signal("new_surface", surface);
}

void WlrXWayland::_bind_methods() {
	ADD_SIGNAL(MethodInfo("new_surface",
				PropertyInfo(Variant::OBJECT,
					"xwayland_surface", PROPERTY_HINT_RESOURCE_TYPE, "WlrXWaylandSurface")));
}


void WlrXWayland::start_xwayland(Variant _compositor, Variant _seat) {
	auto compositor = dynamic_cast<WlrCompositor *>((Node *)_compositor);
	auto seat = dynamic_cast<WlrSeat *>((Node *)_seat);
  struct wlr_seat * w_seat = seat->get_wlr_seat();

	if (wlr_xwayland) {
    std::cout << "Xwayland is already started." << std::endl;
		return;
	}

  struct wlr_compositor * wlr_compositor = compositor->get_wlr_compositor();
  struct wl_display * wl_display = get_wayland_display()->get_wayland_display();

  if (wl_display && wlr_compositor) {
    wlr_xwayland = wlr_xwayland_create(wl_display, wlr_compositor, false); //`true` forces XWayland to start in lazy mode

    new_xwayland_surface.notify = handle_new_xwayland_surface;
		wl_signal_add(&wlr_xwayland->events.new_surface,
                  &new_xwayland_surface);

		wlr_xwayland_set_seat(wlr_xwayland, w_seat);

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
	return Rect2(0, 0, wlr_xwayland_surface->width, wlr_xwayland_surface->height);
}

WlrSurface *WlrXWaylandSurface::get_wlr_surface() const {
	return WlrSurface::from_wlr_surface(wlr_xwayland_surface->surface);
}

WlrSurfaceAtResult *WlrXWaylandSurface::surface_at(double sx, double sy) {
	double sub_x, sub_y;
	struct wlr_surface *result = wlr_surface_surface_at(wlr_xwayland_surface->surface, sx, sy, &sub_x, &sub_y);

	return new WlrSurfaceAtResult(WlrSurface::from_wlr_surface(result), sub_x, sub_y);
}


extern "C" {

void WlrXWaylandSurface::handle_destroy(
		struct wl_listener *listener, void *data) {
	WlrXWaylandSurface *xwayland_surface = wl_container_of(
			listener, xwayland_surface, destroy);

  wl_list_remove(&xwayland_surface->destroy.link);
  wl_list_remove(&xwayland_surface->request_configure.link);
  wl_list_remove(&xwayland_surface->request_move.link);
  wl_list_remove(&xwayland_surface->request_resize.link);
  wl_list_remove(&xwayland_surface->request_maximize.link);
  wl_list_remove(&xwayland_surface->map.link);
  wl_list_remove(&xwayland_surface->unmap.link);

	xwayland_surface->emit_signal("destroy", xwayland_surface);
}

void WlrXWaylandSurface::handle_map(
		struct wl_listener *listener, void *data) {
	WlrXWaylandSurface *xwayland_surface = wl_container_of(
			listener, xwayland_surface, map);

  auto width = xwayland_surface->get_width();
  auto height = xwayland_surface->get_height();

  if (width < 200 || height < 200) {
    wlr_xwayland_surface_configure(xwayland_surface->wlr_xwayland_surface, 0, 0, 1024, 768);
  }

  wlr_xwayland_surface_set_maximized(xwayland_surface->wlr_xwayland_surface, true);
	xwayland_surface->emit_signal("map", xwayland_surface);
}

void WlrXWaylandSurface::handle_unmap(
		struct wl_listener *listener, void *data) {
	WlrXWaylandSurface *xwayland_surface = wl_container_of(
			listener, xwayland_surface, unmap);

	//wl_list_remove(&xwayland_surface->surface_commit.link);
	//wl_signal_emit(&view->events.unmap, view); //in rootston

	//wl_list_remove(&xwayland_surface->surface_commit.link);

	xwayland_surface->emit_signal("unmap", xwayland_surface);
}
}

WlrXWaylandSurface *WlrXWaylandSurface::from_wlr_xwayland_surface(
		struct wlr_xwayland_surface *xwayland_surface) {
	if (xwayland_surface->data) {
		return (WlrXWaylandSurface *)xwayland_surface->data;
	}
	return new WlrXWaylandSurface(xwayland_surface);
}

extern "C" {

void WlrXWaylandSurface::handle_request_maximize(
		struct wl_listener *listener, void *data) {
	WlrXWaylandSurface *xwayland_surface = wl_container_of(
			listener, xwayland_surface, request_maximize);
	xwayland_surface->emit_signal("request_maximize", xwayland_surface);
}

void WlrXWaylandSurface::handle_request_fullscreen(
		struct wl_listener *listener, void *data) {
	WlrXWaylandSurface *xwayland_surface = wl_container_of(
			listener, xwayland_surface, request_fullscreen);

  bool fullscreen = !(xwayland_surface->wlr_xwayland_surface->fullscreen);

	xwayland_surface->emit_signal("request_fullscreen",
			xwayland_surface, fullscreen);
}

void WlrXWaylandSurface::handle_request_move(
		struct wl_listener *listener, void *data) {
	WlrXWaylandSurface *xwayland_surface = wl_container_of(
			listener, xwayland_surface, request_move);
	//struct wlr_xwayland_move_event *event = (struct wlr_xwayland_move_event *)data;
	xwayland_surface->emit_signal("request_move", xwayland_surface);
}

void WlrXWaylandSurface::handle_request_resize(
		struct wl_listener *listener, void *data) {
	WlrXWaylandSurface *xwayland_surface = wl_container_of(
			listener, xwayland_surface, request_resize);
	struct wlr_xwayland_resize_event *event =
		(struct wlr_xwayland_resize_event *)data;
	xwayland_surface->emit_signal("request_resize", xwayland_surface, event->edges);
}

void WlrXWaylandSurface::handle_set_parent(
		struct wl_listener *listener, void *data) {
	WlrXWaylandSurface *xwayland_surface = wl_container_of(
			listener, xwayland_surface, set_parent);
	xwayland_surface->emit_signal("set_parent", xwayland_surface);
}

}

WlrXWaylandSurface::WlrXWaylandSurface() {
	/* Not used */
}

WlrXWaylandSurface::WlrXWaylandSurface(struct wlr_xwayland_surface *xwayland_surface) {
  wlr_xwayland_surface = xwayland_surface;
  xwayland_surface->data = this;

  wlr_xwayland_surface_ping(xwayland_surface);
  destroy.notify = handle_destroy;
  wl_signal_add(&xwayland_surface->events.destroy, &destroy);
  request_configure.notify = handle_request_configure;
  wl_signal_add(&xwayland_surface->events.request_configure, &request_configure);
  map.notify = handle_map;
  wl_signal_add(&xwayland_surface->events.map, &map);
  unmap.notify = handle_unmap;
  wl_signal_add(&xwayland_surface->events.unmap, &unmap);
  request_maximize.notify = handle_request_maximize;
  wl_signal_add(&wlr_xwayland_surface->events.request_maximize, &request_maximize);
  request_fullscreen.notify = handle_request_fullscreen;
  wl_signal_add(&wlr_xwayland_surface->events.request_fullscreen, &request_fullscreen);
  request_move.notify = handle_request_move;
  wl_signal_add(&wlr_xwayland_surface->events.request_move, &request_move);
  request_resize.notify = handle_request_resize;
  wl_signal_add(&wlr_xwayland_surface->events.request_resize, &request_resize);
}

WlrXWaylandSurface *WlrXWaylandSurface::get_parent() const {
	return from_wlr_xwayland_surface(wlr_xwayland_surface->parent);
}

String WlrXWaylandSurface::get_title() const {
	return String(wlr_xwayland_surface->title);
}

void WlrXWaylandSurface::set_size(Vector2 size) {
  wlr_xwayland_surface_configure(wlr_xwayland_surface,
                                 wlr_xwayland_surface->x,
                                 wlr_xwayland_surface->y,
                                 size.width,
                                 size.height);
}

void WlrXWaylandSurface::set_activated(bool activated) {
  //wlr_xwayland_or_surface_wants_focus(wlr_xwayland_surface);
  wlr_xwayland_surface_activate(wlr_xwayland_surface, activated);
}

void WlrXWaylandSurface::set_maximized(bool maximized) {
	wlr_xwayland_surface_set_maximized(wlr_xwayland_surface, maximized);
}

void WlrXWaylandSurface::set_fullscreen(bool fullscreen) {
  wlr_xwayland_surface_set_fullscreen(wlr_xwayland_surface, fullscreen);
}

void WlrXWaylandSurface::send_close() {
  wlr_xwayland_surface_close(wlr_xwayland_surface);
}

bool WlrXWaylandSurface::get_maximized() const {
	bool maximized = wlr_xwayland_surface->maximized_vert && wlr_xwayland_surface->maximized_horz;
	return maximized;
}

bool WlrXWaylandSurface::get_fullscreen() const {
	return wlr_xwayland_surface->fullscreen;
}

uint32_t WlrXWaylandSurface::get_width() const {
	return wlr_xwayland_surface->width;
}

String WlrXWaylandSurface::get_role() const {
	return wlr_xwayland_surface->role;
}

uint32_t WlrXWaylandSurface::get_height() const {
	return wlr_xwayland_surface->height;
}

uint32_t WlrXWaylandSurface::get_min_width() const {
  return wlr_xwayland_surface->size_hints->min_width;
}

uint32_t WlrXWaylandSurface::get_min_height() const {
  return wlr_xwayland_surface->size_hints->min_height;
}

uint32_t WlrXWaylandSurface::get_max_width() const {
	return wlr_xwayland_surface->size_hints->max_width;
}

uint32_t WlrXWaylandSurface::get_max_height() const {
	return wlr_xwayland_surface->size_hints->max_height;
}

//Warning: not tested!
Array WlrXWaylandSurface::get_children() {
  Array array_xwayland_surfaces;
  struct wlr_xwayland_surface * xws;
  std::cout << "wlr_xwayland_surface: " << wlr_xwayland_surface << std::endl;
  int length = wl_list_length(&wlr_xwayland_surface->children);
  int length2 = wl_list_length(&wlr_xwayland_surface->surface->subsurfaces);
  // std::cout << "wl_list_length(wlr_xwayland_surface->children): " << length << std::endl;
  // std::cout << "wl_list_length(&wlr_xwayland_surface->surface->subsurfaces): " << length2 << std::endl;
  wl_list_for_each(xws, &wlr_xwayland_surface->children, parent_link) {
    Variant _xws = (Variant) xws;
    array_xwayland_surfaces.push_front(_xws);
  }
  return array_xwayland_surfaces;
}

void WlrXWaylandSurface::_bind_methods() {

  ClassDB::bind_method(D_METHOD("get_role"), &WlrXWaylandSurface::get_role);
  ClassDB::bind_method(D_METHOD("start_xwayland", "compositor", "seat"), &WlrXWayland::start_xwayland);
  ClassDB::bind_method(D_METHOD("get_geometry"), &WlrXWaylandSurface::get_geometry);
  ClassDB::bind_method(D_METHOD("get_wlr_surface"), &WlrXWaylandSurface::get_wlr_surface);
  ClassDB::bind_method(D_METHOD("surface_at", "sx", "sy"), &WlrXWaylandSurface::surface_at);

	ClassDB::bind_method(D_METHOD("get_fullscreen"), &WlrXWaylandSurface::get_fullscreen);
	ClassDB::bind_method(D_METHOD("get_width"), &WlrXWaylandSurface::get_width);
	ClassDB::bind_method(D_METHOD("get_height"), &WlrXWaylandSurface::get_height);
	ClassDB::bind_method(D_METHOD("get_min_width"), &WlrXWaylandSurface::get_min_width);
	ClassDB::bind_method(D_METHOD("get_min_height"), &WlrXWaylandSurface::get_min_height);
	ClassDB::bind_method(D_METHOD("get_max_width"), &WlrXWaylandSurface::get_max_width);
	ClassDB::bind_method(D_METHOD("get_max_height"), &WlrXWaylandSurface::get_max_height);
	ClassDB::bind_method(D_METHOD("get_children"), &WlrXWaylandSurface::get_children);
	ClassDB::bind_method(D_METHOD("get_parent"), &WlrXWaylandSurface::get_parent);
	ClassDB::bind_method(D_METHOD("get_title"), &WlrXWaylandSurface::get_title);
	ClassDB::bind_method(D_METHOD("set_size", "size"), &WlrXWaylandSurface::set_size);
	ClassDB::bind_method(D_METHOD("set_activated", "activated"), &WlrXWaylandSurface::set_activated);
	ClassDB::bind_method(D_METHOD("get_maximized"), &WlrXWaylandSurface::get_maximized);
	ClassDB::bind_method(D_METHOD("set_maximized", "maximized"), &WlrXWaylandSurface::set_maximized);
	ClassDB::bind_method(D_METHOD("set_fullscreen", "fullscreen"), &WlrXWaylandSurface::set_fullscreen);
	ClassDB::bind_method(D_METHOD("send_close"), &WlrXWaylandSurface::send_close);

	ADD_SIGNAL(MethodInfo("request_maximize", PropertyInfo(Variant::OBJECT, "xwayland_surface", PROPERTY_HINT_RESOURCE_TYPE, "WlrXWaylandSurface")));
	ADD_SIGNAL(MethodInfo("destroy", PropertyInfo(Variant::OBJECT, "xwayland_surface", PROPERTY_HINT_RESOURCE_TYPE, "WlrXWaylandSurface")));
	ADD_SIGNAL(MethodInfo("map", PropertyInfo(Variant::OBJECT, "xwayland_surface", PROPERTY_HINT_RESOURCE_TYPE, "WlrXWaylandSurface")));
	ADD_SIGNAL(MethodInfo("surface_commit", PropertyInfo(Variant::OBJECT, "xwayland_surface", PROPERTY_HINT_RESOURCE_TYPE, "WlrXWaylandSurface")));
	ADD_SIGNAL(MethodInfo("unmap", PropertyInfo(Variant::OBJECT, "xwayland_surface", PROPERTY_HINT_RESOURCE_TYPE, "WlrXWaylandSurface")));
	ADD_SIGNAL(MethodInfo("request_fullscreen", PropertyInfo(Variant::OBJECT, "xwayland_surface", PROPERTY_HINT_RESOURCE_TYPE, "WlrXWaylandSurface"), PropertyInfo(Variant::BOOL, "fullscreen")));

	ADD_SIGNAL(MethodInfo("request_move", PropertyInfo(Variant::OBJECT, "xwayland_surface", PROPERTY_HINT_RESOURCE_TYPE, "WlrXWaylandSurface")));
  ADD_SIGNAL(MethodInfo("request_resize", PropertyInfo(Variant::OBJECT, "xwayland_surface", PROPERTY_HINT_RESOURCE_TYPE, "WlrXWaylandSurface"), PropertyInfo(Variant::INT, "edges")));
	ADD_SIGNAL(MethodInfo("set_title", PropertyInfo(Variant::OBJECT, "xwayland_surface", PROPERTY_HINT_RESOURCE_TYPE, "WlrXWaylandSurface")));
	ADD_SIGNAL(MethodInfo("set_parent", PropertyInfo(Variant::OBJECT, "xwayland_surface", PROPERTY_HINT_RESOURCE_TYPE, "WlrXWaylandSurface")));
}
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
#include "debug.h"

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
#include <xwayland/xwm.h>

}

bool xwm_atoms_contains(struct wlr_xwm *xwm, xcb_atom_t *atoms,
												size_t num_atoms, enum atom_name needle) {

	if(xwm->atoms == NULL || atoms == NULL) {
		return false;
	}

	xcb_atom_t atom = xwm->atoms[needle];

	for (size_t i = 0; i < num_atoms; ++i) {
		if (atom == atoms[i]) {
			return true;
		}
	}

	return false;
}

void WlrXWaylandSurface::handle_request_configure(struct wl_listener *listener, void *data) {
	//std::cout << "WlrXWaylandSurface::handle_request_configure(..)" << std::endl;
  struct wlr_xwayland_surface_configure_event * event = (wlr_xwayland_surface_configure_event *) data;

  WlrXWaylandSurface *xwayland_surface = wl_container_of(listener, xwayland_surface, request_configure);

  wlr_xwayland_surface_configure(xwayland_surface->wlr_xwayland_surface, 0, 0,
                                 event->width, event->height);
}

void WlrXWayland::handle_new_xwayland_surface(
		struct wl_listener *listener, void *data) {
	//std::cout << "WlrXWayland::handle_new_xwayland_surface(..)" << std::endl;
  WlrXWayland *xwayland = wl_container_of(
			listener, xwayland, new_xwayland_surface);
	auto surface = WlrXWaylandSurface::from_wlr_xwayland_surface((struct wlr_xwayland_surface *)data);

	xwayland->emit_signal("new_surface", surface);
}

void WlrXWayland::_bind_methods() {
	ADD_SIGNAL(MethodInfo("new_surface",
				PropertyInfo(Variant::OBJECT,
					"xwayland_surface", PROPERTY_HINT_RESOURCE_TYPE, "WlrXWaylandSurface")));
}


void WlrXWayland::start_xwayland(Object* _compositor, Object* _seat) {
	auto compositor = Object::cast_to<WlrCompositor>(_compositor);
	auto seat = Object::cast_to<WlrSeat>(_seat);
	if (!compositor || !seat) return;

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
  //std::cout << "WlrXWayland::destroy_wl_global" << std::endl;
	wlr_xwayland_destroy(wlr_xwayland);
	wlr_xwayland = NULL;
}

WlrXWayland::WlrXWayland() {
	wlr_xwayland = NULL;
}

WlrXWayland::~WlrXWayland() {
  //std::cout << "WlrXWayland::~WlrXWayland" << std::endl;
	wlr_xwayland_destroy(wlr_xwayland);
	wlr_xwayland = NULL;
}

void WlrXWaylandSurface::terminate() {
  kill(wlr_xwayland_surface->pid, SIGTERM);
}

Rect2 WlrXWaylandSurface::get_geometry() {
	return Rect2(0, 0, wlr_xwayland_surface->width, wlr_xwayland_surface->height);
}

WlrSurface *WlrXWaylandSurface::get_wlr_surface() const {
  return WlrSurface::from_wlr_surface(wlr_xwayland_surface->surface);
}

struct wlr_surface *wlr_surface_surface_at_spillover(struct wlr_surface *surface, double sx, double sy, double *sub_x, double *sub_y) {
	struct wlr_subsurface *subsurface;
	wl_list_for_each_reverse(subsurface, &surface->subsurfaces, parent_link) {
		double _sub_x = subsurface->current.x;
		double _sub_y = subsurface->current.y;
		struct wlr_surface *sub = wlr_surface_surface_at_spillover(subsurface->surface,
				sx - _sub_x, sy - _sub_y, sub_x, sub_y);
		if (sub != NULL) {
			return sub;
		}
	}

	/* Remove this to fix XWayland spillover popup functionality
	if (wlr_surface_point_accepts_input(surface, sx, sy)) {
		,*sub_x = sx;
		,*sub_y = sy;
		return surface;
	}
  */

	*sub_x = sx;
	*sub_y = sy;
	return surface;
}

WlrSurfaceAtResult *WlrXWaylandSurface::surface_at(double sx, double sy) {
  double sub_x, sub_y;
  struct wlr_surface *result = wlr_surface_surface_at_spillover(wlr_xwayland_surface->surface, sx, sy, &sub_x, &sub_y);

  return new WlrSurfaceAtResult(WlrSurface::from_wlr_surface(result), sub_x, sub_y);
}


extern "C" {

void WlrXWaylandSurface::handle_destroy(
    struct wl_listener *listener, void *data) {
  //std::cout << "WlrXWaylandSurface::handle_destroy(..)" << std::endl;
  WlrXWaylandSurface *xwayland_surface = wl_container_of(
      listener, xwayland_surface, destroy);

  wl_list_remove(&xwayland_surface->destroy.link);
  wl_list_remove(&xwayland_surface->request_configure.link);
  wl_list_remove(&xwayland_surface->request_move.link);
  wl_list_remove(&xwayland_surface->request_resize.link);
  wl_list_remove(&xwayland_surface->request_maximize.link);
  wl_list_remove(&xwayland_surface->map.link);
  wl_list_remove(&xwayland_surface->unmap.link);

  xwayland_surface->emit_signal("destroy", xwayland_surface); //We `delete` this surface elsewhere
}

void WlrXWaylandSurface::handle_map(struct wl_listener *listener, void *data) {
	//std::cout << "WlrXWaylandSurface::handle_map(..)" << std::endl;
		WlrXWaylandSurface *xwayland_surface = wl_container_of(
																													 listener, xwayland_surface, map);

		bool is_splash_surface = xwm_atoms_contains(xwayland_surface->wlr_xwayland_surface->xwm,
																								xwayland_surface->wlr_xwayland_surface->window_type,
																								1,
																								NET_WM_WINDOW_TYPE_SPLASH);

		bool is_normal_surface = xwm_atoms_contains(xwayland_surface->wlr_xwayland_surface->xwm,
																								xwayland_surface->wlr_xwayland_surface->window_type,
																								1,
																								NET_WM_WINDOW_TYPE_NORMAL);

		if ( is_splash_surface ) {
			//std::cout << "handle_map(..) splash surface -> map" << std::endl;
			xwayland_surface->emit_signal("map", xwayland_surface);
    } else if( xwayland_surface->wlr_xwayland_surface->parent == NULL && (! is_normal_surface) ) {
			//std::cout << "handle_map(..) map_free_child" << std::endl;
			xwayland_surface->emit_signal("map_free_child", xwayland_surface);
		} else if( xwayland_surface->wlr_xwayland_surface->parent == NULL && is_normal_surface ) {
			//std::cout << "handle_map(..) map" << std::endl;
			xwayland_surface->emit_signal("map", xwayland_surface);
		} else if( xwayland_surface->wlr_xwayland_surface->parent != NULL && !is_normal_surface ) {
			//std::cout << "handle_map(..) map_child" << std::endl;
			xwayland_surface->emit_signal("map_child", xwayland_surface);
		} else if( xwayland_surface->wlr_xwayland_surface->window_type == NULL) {
			xwayland_surface->emit_signal("map", xwayland_surface);
		} else {
			//std::cout << "handle_map(..) called without anywhere to route surface!" << std::endl;
			xwayland_surface->emit_signal("map_free_child", xwayland_surface);
		}
}

void WlrXWaylandSurface::handle_unmap(
    struct wl_listener *listener, void *data) {
	//std::cout << "WlrXWaylandSurface::handle_unmap(..)" << std::endl;

  WlrXWaylandSurface *xwayland_surface = wl_container_of(
      listener, xwayland_surface, unmap);
  //std::cout << "WlrXWaylandSurface::handle_unmap" << std::endl;

		bool is_splash_surface = xwm_atoms_contains(xwayland_surface->wlr_xwayland_surface->xwm,
																								xwayland_surface->wlr_xwayland_surface->window_type,
																								1,
																								NET_WM_WINDOW_TYPE_SPLASH);

		bool is_normal_surface = xwm_atoms_contains(xwayland_surface->wlr_xwayland_surface->xwm,
																								xwayland_surface->wlr_xwayland_surface->window_type,
																								1,
																								NET_WM_WINDOW_TYPE_NORMAL);

		//roughly mirrors our logic in handle_map
		if ( is_splash_surface ) {
			//std::cout << "handle_unmap(..) splash surface -> unmap" << std::endl;
			xwayland_surface->emit_signal("unmap", xwayland_surface);
    } else if( xwayland_surface->wlr_xwayland_surface->parent == NULL && (! is_normal_surface) ) {
			//std::cout << "handle_unmap(..) unmap_free_child" << std::endl;
			xwayland_surface->emit_signal("unmap_free_child", xwayland_surface);
		} else if( xwayland_surface->wlr_xwayland_surface->parent == NULL && is_normal_surface ) {
			//std::cout << "handle_unmap(..) unmap" << std::endl;
			xwayland_surface->emit_signal("unmap", xwayland_surface);
		} else if( xwayland_surface->wlr_xwayland_surface->parent != NULL && !is_normal_surface ) {
			//std::cout << "handle_unmap(..) unmap_child" << std::endl;
			xwayland_surface->emit_signal("unmap_child", xwayland_surface);
		} else if( xwayland_surface->wlr_xwayland_surface->window_type == NULL) {
			xwayland_surface->emit_signal("unmap", xwayland_surface);
		} else {
			//std::cout << "handle_unmap(..) called without anywhere to route surface!" << std::endl;
			xwayland_surface->emit_signal("unmap_free_child", xwayland_surface);
		}
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
	//std::cout << "WlrXWaylandSurface::handle_request_move(..)" << std::endl;
  WlrXWaylandSurface *xwayland_surface = wl_container_of(
      listener, xwayland_surface, request_move);
  //struct wlr_xwayland_move_event *event = (struct wlr_xwayland_move_event *)data;
  xwayland_surface->emit_signal("request_move", xwayland_surface);
}

void WlrXWaylandSurface::handle_request_resize(
    struct wl_listener *listener, void *data) {
	//std::cout << "WlrXWaylandSurface::handle_request_resize(..)" << std::endl;
  WlrXWaylandSurface *xwayland_surface = wl_container_of(
      listener, xwayland_surface, request_resize);
  struct wlr_xwayland_resize_event *event =
    (struct wlr_xwayland_resize_event *)data;
  xwayland_surface->emit_signal("request_resize", xwayland_surface, event->edges);
}

void WlrXWaylandSurface::handle_set_parent(
    struct wl_listener *listener, void *data) {
	//std::cout << "WlrXWaylandSurface::handle_set_parent(..)" << std::endl;
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

  set_parent.notify = handle_set_parent;
  wl_signal_add(&wlr_xwayland_surface->events.set_parent, &set_parent);
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

void WlrXWaylandSurface::set_xy(Vector2 xy) {
  wlr_xwayland_surface_configure(wlr_xwayland_surface,
																 xy.width,
																 xy.height,
																 wlr_xwayland_surface->width,
																 wlr_xwayland_surface->height);
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

uint16_t WlrXWaylandSurface::get_width() const {
  return wlr_xwayland_surface->width;
}

String WlrXWaylandSurface::get_role() const {
  return wlr_xwayland_surface->role;
}

int16_t WlrXWaylandSurface::get_x() const {
  return wlr_xwayland_surface->x;
}

int16_t WlrXWaylandSurface::get_y() const {
  return wlr_xwayland_surface->y;
}

void WlrXWaylandSurface::print_xwayland_surface_properties() {
    std::cout << "wlr_xwayland_surface->c_class: " << wlr_xwayland_surface->c_class << std::endl;
    std::cout << "wlr_xwayland_surface->instance: " << wlr_xwayland_surface->instance << std::endl;
    std::cout << "wlr_xwayland_surface->role: " << wlr_xwayland_surface->role << std::endl;
    std::cout << "wl_list_length(&wlr_xwayland_surface->parent_link): " << wl_list_length(&wlr_xwayland_surface->parent_link) << std::endl;
    std::cout << "wlr_xwayland_surface->window_type: " << wlr_xwayland_surface->window_type << std::endl;
    std::cout << "wlr_xwayland_surface->protocols: " << wlr_xwayland_surface->protocols << std::endl;
    std::cout << "wlr_xwayland_surface->override_redirect: " << wlr_xwayland_surface->override_redirect << std::endl;
}

uint16_t WlrXWaylandSurface::get_height() const {
  return wlr_xwayland_surface->height;
}

uint16_t WlrXWaylandSurface::get_min_width() const {
  return wlr_xwayland_surface->size_hints->min_width;
}

uint16_t WlrXWaylandSurface::get_min_height() const {
  return wlr_xwayland_surface->size_hints->min_height;
}

uint16_t WlrXWaylandSurface::get_max_width() const {
  return wlr_xwayland_surface->size_hints->max_width;
}

uint16_t WlrXWaylandSurface::get_max_height() const {
  return wlr_xwayland_surface->size_hints->max_height;
}

Array WlrXWaylandSurface::get_children() {
  struct wlr_xwayland_surface * xws;

  children.clear();

  wl_list_for_each(xws, &wlr_xwayland_surface->children, parent_link) {
		//std::cout << "get_children (data, mapped): (" << (xws->data) << ", " << (xws->mapped) << ")" << std::endl;
    if (xws->data && xws->mapped) {
      WlrXWaylandSurface * xWS;
      xWS = (WlrXWaylandSurface *)xws->data; //Only return children for whom we have WlrXWaylandSurface's formed already
      Variant _xWS = Variant( (Object *) xWS );
      children.push_front(_xWS);
    }
  }

  return children;
}

int WlrXWaylandSurface::get_pid() {
	return ((int) wlr_xwayland_surface->pid);
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
	ClassDB::bind_method(D_METHOD("get_x"), &WlrXWaylandSurface::get_x);
	ClassDB::bind_method(D_METHOD("get_y"), &WlrXWaylandSurface::get_y);
	ClassDB::bind_method(D_METHOD("terminate"), &WlrXWaylandSurface::terminate);
	ClassDB::bind_method(D_METHOD("print_xwayland_surface_properties"), &WlrXWaylandSurface::print_xwayland_surface_properties);
	ClassDB::bind_method(D_METHOD("get_min_width"), &WlrXWaylandSurface::get_min_width);
	ClassDB::bind_method(D_METHOD("get_min_height"), &WlrXWaylandSurface::get_min_height);
	ClassDB::bind_method(D_METHOD("get_max_width"), &WlrXWaylandSurface::get_max_width);
	ClassDB::bind_method(D_METHOD("get_max_height"), &WlrXWaylandSurface::get_max_height);
	ClassDB::bind_method(D_METHOD("get_children"), &WlrXWaylandSurface::get_children);
	ClassDB::bind_method(D_METHOD("get_parent"), &WlrXWaylandSurface::get_parent);
	ClassDB::bind_method(D_METHOD("get_title"), &WlrXWaylandSurface::get_title);
	ClassDB::bind_method(D_METHOD("set_size", "size"), &WlrXWaylandSurface::set_size);
	ClassDB::bind_method(D_METHOD("set_xy", "size"), &WlrXWaylandSurface::set_xy);
	ClassDB::bind_method(D_METHOD("set_activated", "activated"), &WlrXWaylandSurface::set_activated);
	ClassDB::bind_method(D_METHOD("get_maximized"), &WlrXWaylandSurface::get_maximized);
	ClassDB::bind_method(D_METHOD("set_maximized", "maximized"), &WlrXWaylandSurface::set_maximized);
	ClassDB::bind_method(D_METHOD("set_fullscreen", "fullscreen"), &WlrXWaylandSurface::set_fullscreen);
	ClassDB::bind_method(D_METHOD("send_close"), &WlrXWaylandSurface::send_close);
	ClassDB::bind_method(D_METHOD("get_pid"), &WlrXWaylandSurface::get_pid);

	ADD_SIGNAL(MethodInfo("request_maximize", PropertyInfo(Variant::OBJECT, "xwayland_surface", PROPERTY_HINT_RESOURCE_TYPE, "WlrXWaylandSurface")));
	ADD_SIGNAL(MethodInfo("destroy", PropertyInfo(Variant::OBJECT, "xwayland_surface", PROPERTY_HINT_RESOURCE_TYPE, "WlrXWaylandSurface")));
	ADD_SIGNAL(MethodInfo("map", PropertyInfo(Variant::OBJECT, "xwayland_surface", PROPERTY_HINT_RESOURCE_TYPE, "WlrXWaylandSurface")));
	ADD_SIGNAL(MethodInfo("map_free_child", PropertyInfo(Variant::OBJECT, "xwayland_surface", PROPERTY_HINT_RESOURCE_TYPE, "WlrXWaylandSurface")));
	ADD_SIGNAL(MethodInfo("map_child", PropertyInfo(Variant::OBJECT, "xwayland_surface", PROPERTY_HINT_RESOURCE_TYPE, "WlrXWaylandSurface")));
	ADD_SIGNAL(MethodInfo("unmap_free_child", PropertyInfo(Variant::OBJECT, "xwayland_surface", PROPERTY_HINT_RESOURCE_TYPE, "WlrXWaylandSurface")));
	ADD_SIGNAL(MethodInfo("unmap_child", PropertyInfo(Variant::OBJECT, "xwayland_surface", PROPERTY_HINT_RESOURCE_TYPE, "WlrXWaylandSurface")));
	ADD_SIGNAL(MethodInfo("surface_commit", PropertyInfo(Variant::OBJECT, "xwayland_surface", PROPERTY_HINT_RESOURCE_TYPE, "WlrXWaylandSurface")));
	ADD_SIGNAL(MethodInfo("unmap", PropertyInfo(Variant::OBJECT, "xwayland_surface", PROPERTY_HINT_RESOURCE_TYPE, "WlrXWaylandSurface")));
	ADD_SIGNAL(MethodInfo("request_fullscreen", PropertyInfo(Variant::OBJECT, "xwayland_surface", PROPERTY_HINT_RESOURCE_TYPE, "WlrXWaylandSurface"), PropertyInfo(Variant::BOOL, "fullscreen")));

	ADD_SIGNAL(MethodInfo("request_move", PropertyInfo(Variant::OBJECT, "xwayland_surface", PROPERTY_HINT_RESOURCE_TYPE, "WlrXWaylandSurface")));
  ADD_SIGNAL(MethodInfo("request_resize", PropertyInfo(Variant::OBJECT, "xwayland_surface", PROPERTY_HINT_RESOURCE_TYPE, "WlrXWaylandSurface"), PropertyInfo(Variant::INT, "edges")));
	ADD_SIGNAL(MethodInfo("set_title", PropertyInfo(Variant::OBJECT, "xwayland_surface", PROPERTY_HINT_RESOURCE_TYPE, "WlrXWaylandSurface")));
	ADD_SIGNAL(MethodInfo("set_parent", PropertyInfo(Variant::OBJECT, "xwayland_surface", PROPERTY_HINT_RESOURCE_TYPE, "WlrXWaylandSurface")));
}

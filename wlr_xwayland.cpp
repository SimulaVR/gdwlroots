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

//#include "xwayland/xwm.h" We are unable to access this :(
#include <iostream>

extern "C" {
#include <signal.h>
#include <wayland-server.h>

//We override xwayland.h to avoid the `class` keyword
//#include <wlr/xwayland.h>
#include "xwayland.h"

void signal_callback_handler(int signum) {
  printf("Caught signal from gdwlroots: %d\n",signum);
  signal(SIGUSR1, SIG_IGN);
  //exit(signum);
}

void WlrXWayland::handle_new_xwayland_surface(
		struct wl_listener *listener, void *data) {
  WlrXWayland *xwayland = wl_container_of(
			listener, xwayland, new_xwayland_surface);
	auto surface = WlrXWaylandSurface::from_wlr_xwayland_surface(
      (struct wlr_xwayland_surface *)data);

  //struct wlr_xwayland_surface * w_xwayland_surface = (struct wlr_xwayland_surface *)data;
  //wlr_xwayland_surface_ping(w_xwayland_surface); test

	xwayland->emit_signal("new_surface", surface);
  std::cout << "handle_new_xwayland_surface called for surface: " << surface << std::endl;
}

}

void WlrXWayland::_bind_methods() {
	ADD_SIGNAL(MethodInfo("new_surface",
				PropertyInfo(Variant::OBJECT,
					"xwayland_surface", PROPERTY_HINT_RESOURCE_TYPE, "WlrXWaylandSurface")));
}

void WlrXWayland::start_xwayland(Variant _compositor, Variant _seat) {
  std::cout << "WlrXWayland::start_xwayland" << std::endl;
  pid_t ppid;
  ppid = getppid();
  std::cout << "gdwlroots ppid: " << ppid << std::endl;
	auto compositor = dynamic_cast<WlrCompositor *>((Node *)_compositor);
	auto seat = dynamic_cast<WlrSeat *>((Node *)_seat);
  struct wlr_seat * w_seat = seat->get_wlr_seat();

	if (wlr_xwayland) {
    std::cout << "Xwayland is already started." << std::endl;
		return;
	}

//  signal(SIGUSR1, SIG_IGN); //<- Here
  //signal(SIGUSR1, signal_callback_handler); //Possibly useful if SIG_IGN isn't the right approach
  struct wlr_compositor * wlr_compositor = compositor->get_wlr_compositor();
  struct wl_display * wl_display = get_wayland_display()->get_wayland_display();

  if (wl_display && wlr_compositor) {
    wlr_xwayland = wlr_xwayland_create(wl_display, wlr_compositor, false); //`true` forces XWayland to start in lazy mode

    //Duplicated by xwayland_start_display()? But also done in rootston/desktop.c?
    // char display_name[16];
    // snprintf(display_name, sizeof(display_name), ":%d", wlr_xwayland->display);
    // setenv("DISPLAY", display_name, true); //in newest wlroots we can just use wlr_xwayland->display_name


		wlr_xwayland_set_seat(wlr_xwayland, w_seat);

    new_xwayland_surface.notify = handle_new_xwayland_surface;
		wl_signal_add(&wlr_xwayland->events.new_surface,
                  &new_xwayland_surface
                  );

    //We omit xcursor stuff here
		// if (wlr_xcursor_manager_load(desktop->xcursor_manager, 1)) {
		// 	wlr_log(WLR_ERROR, "Cannot load XWayland XCursor theme");
		// }
		// struct wlr_xcursor *xcursor = wlr_xcursor_manager_get_xcursor(
    //                                                               desktop->xcursor_manager, cursor_default, 1);
		// if (xcursor != NULL) {
		// 	struct wlr_xcursor_image *image = xcursor->images[0];
		// 	wlr_xwayland_set_cursor(desktop->xwayland, image->buffer,
    //                           image->width * 4, image->width, image->height, image->hotspot_x,
    //                           image->hotspot_y);
		// }

    //Things we omit from XWayland initialization:
    //1. Adjusting DISPLAY environment variable\
    //2. xcursor stuff
    //See i.e. https://github.com/swaywm/wlroots/blob/b3f42548d068996995490585e27e16c191b4a64c/rootston/desktop.c#L358

  } else {
    std::cout << "Failed to start xwayland." << std::endl;
  }
}

void WlrXWayland::ensure_wl_global(WaylandDisplay *display) {
  std::cout << "WlrXWayland::ensure_wl_global(..)" << std::endl;

  //This function (automatically called by WaylandGlobal) intentionally left blank.
  // We use start_xwayland (called from GDScript/godot-haskell) instead so we can gain access to a WlrCompositor
}

void WlrXWayland::destroy_wl_global(WaylandDisplay *display) {
  std::cout << "WlrXWayland::destroy_wl_global(..)" << std::endl;
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

// static void for_each_surface_iter(struct wlr_surface *surface,
// 		int sx, int sy, void *data) {
// 	FuncRef *func = (FuncRef *)data;
// 	const Variant *args[] = {
// 		new Variant(WlrSurface::from_wlr_surface(surface)),
// 		new Variant(sx),
// 		new Variant(sy),
// 	};
// 	Variant::CallError error;
// 	func->call_func((const Variant **)&args[0], 3, error);
// 	if (error.error != Variant::CallError::Error::CALL_OK) {
// 		printf("call error %d\n", error.error);
// 	}
// }

//We omit implementing this due to lack of clearly usable `*xwayland_children_for_each_surface`:
/*
void WlrXWaylandSurface::for_each_surface(Variant func) {
	auto fn = (Ref<FuncRef>)func;
	wlr_xwayland_surface_for_each_surface(
			wlr_xwayland_surface, for_each_surface_iter, fn.ptr());
}
*/

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

void WlrXWaylandSurface::handle_surface_commit(
     struct wl_listener *listener, void *data) {
    //std::cout << "WlrXWaylandSurface::handle_surface_commit(..) called w/data: " << data << std::endl;
   	WlrXWaylandSurface *xwayland_surface = wl_container_of(listener, xwayland_surface, surface_commit);
    //wlr_output_schedule_frame(xwayland_surface->wlr_xwayland_surface->wlr_output)
    xwayland_surface->emit_signal("surface_commit", xwayland_surface);
}

// Possibly useful for subsurface rendering from Haskell; if you see this after a
// while and it's not being used: delete.
void WlrXWaylandSurface::schedule_frame(Variant _output) {
  std::cout << "WlrXWaylandSurface::schedule_frame(..)" << std::endl;
	auto output = dynamic_cast<WlrOutput *>((Node *)_output);
  struct wlr_output * w_output = output->get_wlr_output();
  wlr_output_schedule_frame(w_output);
}

void WlrXWaylandSurface::handle_map(
		struct wl_listener *listener, void *data) {
	WlrXWaylandSurface *xwayland_surface = wl_container_of(
			listener, xwayland_surface, map);

  xwayland_surface->surface_commit.notify = handle_surface_commit;
  wl_signal_add(&xwayland_surface->wlr_xwayland_surface->surface->events.commit, &xwayland_surface->surface_commit);

	xwayland_surface->emit_signal("map", xwayland_surface);
}

void WlrXWaylandSurface::handle_unmap(
		struct wl_listener *listener, void *data) {
	WlrXWaylandSurface *xwayland_surface = wl_container_of(
			listener, xwayland_surface, unmap);
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

/*
void WlrXWaylandSurface::handle_request_minimize(
		struct wl_listener *listener, void *data) {
	WlrXWaylandSurface *xwayland_surface = wl_container_of(
			listener, xwayland_surface, request_minimize);
	xwayland_surface->emit_signal("request_minimize", xwayland_surface);
}
*/

void WlrXWaylandSurface::handle_request_move(
		struct wl_listener *listener, void *data) {
	WlrXWaylandSurface *xwayland_surface = wl_container_of(
			listener, xwayland_surface, request_move);

  // This is what's in the `void *data` argument, but we don't make use of it:
	// struct wlr_xwayland_move_event *event = (struct wlr_xwayland_move_event *)data;

	xwayland_surface->emit_signal("request_move",
                                xwayland_surface
                                // event->serial //we remove the serial argument in XWayland
                                );
}

void WlrXWaylandSurface::handle_request_resize(
		struct wl_listener *listener, void *data) {
	WlrXWaylandSurface *xwayland_surface = wl_container_of(
			listener, xwayland_surface, request_resize);
	struct wlr_xwayland_resize_event *event =
		(struct wlr_xwayland_resize_event *)data;
	xwayland_surface->emit_signal("request_resize", xwayland_surface,
      //event->serial, //We remove the serial argument in xwayland
      event->edges);
}

// We elminate this event/signal in xwayland
/* void WlrXWaylandSurface::handle_request_show_window_menu(
   		struct wl_listener *listener, void *data) {
   	WlrXWaylandSurface *xwayland_surface = wl_container_of(
   			listener, xwayland_surface, request_show_window_menu);
   	struct wlr_xwayland_surface_show_window_menu_event *event =
   		(struct wlr_xwayland_surface_show_window_menu_event *)data;
   	xwayland_surface->emit_signal("request_show_window_menu", xwayland_surface,
   			event->serial, event->x, event->y);
        }
*/

void WlrXWaylandSurface::handle_set_parent(
		struct wl_listener *listener, void *data) {
	WlrXWaylandSurface *xwayland_surface = wl_container_of(
			listener, xwayland_surface, set_parent);
	xwayland_surface->emit_signal("set_parent", xwayland_surface);
}

void WlrXWaylandSurface::handle_set_title(
		struct wl_listener *listener, void *data) {
	WlrXWaylandSurface *xwayland_surface = wl_container_of(
			listener, xwayland_surface, set_title);
	xwayland_surface->emit_signal("set_title", xwayland_surface);
}

/*
void WlrXWaylandSurface::handle_set_app_id(
		struct wl_listener *listener, void *data) {
	WlrXWaylandSurface *xwayland_surface = wl_container_of(
			listener, xwayland_surface, set_app_id);
	xwayland_surface->emit_signal("set_app_id", xwayland_surface);
}
*/
}

WlrXWaylandSurface::WlrXWaylandSurface() {
	/* Not used */
}

WlrXWaylandSurface::WlrXWaylandSurface(struct wlr_xwayland_surface *xwayland_surface) {
    std::cout << "WlrXWaylandSurface(..) constructor called." << std::endl;

  	destroy.notify = handle_destroy;
  	wl_signal_add(&xwayland_surface->events.destroy, &destroy);
  	map.notify = handle_map;
  	wl_signal_add(&xwayland_surface->events.map, &map);
  	unmap.notify = handle_unmap;
  	wl_signal_add(&xwayland_surface->events.unmap, &unmap);

	wlr_xwayland_surface = xwayland_surface;
  xwayland_surface->data = this;
	request_maximize.notify = handle_request_maximize;
	wl_signal_add(&wlr_xwayland_surface->events.request_maximize,
			&request_maximize);
	request_fullscreen.notify = handle_request_fullscreen;
	wl_signal_add(&wlr_xwayland_surface->events.request_fullscreen,
			&request_fullscreen);
	// request_minimize.notify = handle_request_minimize;
	// wl_signal_add(&wlr_xwayland_surface->events.request_minimize,
	// 		&request_minimize);
	request_move.notify = handle_request_move;
	wl_signal_add(&wlr_xwayland_surface->events.request_move, &request_move);
	request_resize.notify = handle_request_resize;
	wl_signal_add(&wlr_xwayland_surface->events.request_resize, &request_resize);
	// request_show_window_menu.notify = handle_request_show_window_menu;
	// wl_signal_add(&wlr_xwayland_surface->events.request_show_window_menu,
	// 		&request_show_window_menu);
	set_parent.notify = handle_set_parent;
	wl_signal_add(&wlr_xwayland_surface->events.set_parent, &set_parent);
	set_title.notify = handle_set_title;
	// wl_signal_add(&wlr_xwayland_surface->events.set_title, &set_title);
	// set_app_id.notify = handle_set_app_id;
	// wl_signal_add(&wlr_xwayland_surface->events.set_app_id, &set_app_id);
}

WlrXWaylandSurface *WlrXWaylandSurface::get_parent() const {
	return from_wlr_xwayland_surface(wlr_xwayland_surface->parent);
}

/*
String WlrXWaylandSurface::get_app_id() const {
	return String(wlr_xwayland_surface->app_id);
}
*/

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
  wlr_xwayland_surface_activate(wlr_xwayland_surface, activated);
}

void WlrXWaylandSurface::set_maximized(bool maximized) {
	wlr_xwayland_surface_set_maximized(wlr_xwayland_surface, maximized);
}

void WlrXWaylandSurface::set_fullscreen(bool fullscreen) {
  wlr_xwayland_surface_set_fullscreen(wlr_xwayland_surface, fullscreen);
}

/*
void WlrXWaylandSurface::set_resizing(bool resizing) {
	wlr_xdg_toplevel_set_resizing(wlr_xdg_toplevel->base, resizing);
}
*/

/*
void WlrXWaylandSurface::set_tiled(bool tiled) {
	wlr_xdg_toplevel_set_tiled(wlr_xdg_toplevel->base, tiled);
}
*/

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

/*
bool WlrXdgToplevelWlr_Xwayland_Surface::get_resizing() const {
	return wlr_xwayland_surface->resizing;
}
*/

// XWayland doesn't seem to have an "activated" field, so we use this as a hack
// EDIT: Actually, we can't use this hack since we don't have access to xwm.h
/*
bool WlrXWaylandSurface::get_activated() const {
  return (wlr_xwayland_surface->xwm->focus_surface == wlr_xwayland_surface);
}
*/

/*
bool WlrXWaylandSurface::get_tiled() const {
	return wlr_xwayland_surface->tiled;
}
*/

uint32_t WlrXWaylandSurface::get_width() const {
	return wlr_xwayland_surface->width;
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

// void WlrXdgSurface::_bind_methods() {


// }

void WlrXWaylandSurface::_bind_methods() {

  // 	ClassDB::bind_method(D_METHOD("get_role"), &WlrXWaylandSurface::get_role);
  ClassDB::bind_method(D_METHOD("start_xwayland", "compositor", "seat"),
                       &WlrXWayland::start_xwayland);

  	ClassDB::bind_method(D_METHOD("get_geometry"),
  			&WlrXWaylandSurface::get_geometry);
  	ClassDB::bind_method(D_METHOD("get_wlr_surface"),
  			&WlrXWaylandSurface::get_wlr_surface);

  // 	ClassDB::bind_method(D_METHOD("for_each_surface", "func"),
  // 			&WlrXWaylandSurface::for_each_surface);

  	ClassDB::bind_method(D_METHOD("surface_at", "sx", "sy"),
  			&WlrXWaylandSurface::surface_at);

  // 	BIND_ENUM_CONSTANT(XDG_SURFACE_ROLE_NONE);
  // 	BIND_ENUM_CONSTANT(XDG_SURFACE_ROLE_TOPLEVEL);
  // 	BIND_ENUM_CONSTANT(XDG_SURFACE_ROLE_POPUP);

	ClassDB::bind_method(D_METHOD("get_fullscreen"),
			&WlrXWaylandSurface::get_fullscreen);
	// ClassDB::bind_method(D_METHOD("get_resizing"),
	// 		&WlrXWaylandSurface::get_resizing);
	// ClassDB::bind_method(D_METHOD("get_activated"),
	// 		&WlrXWaylandSurface::get_activated);
	// ClassDB::bind_method(D_METHOD("get_tiled"),
	// 		&WlrXWaylandSurface::get_tiled);
	ClassDB::bind_method(D_METHOD("get_width"),
			&WlrXWaylandSurface::get_width);
	ClassDB::bind_method(D_METHOD("get_height"),
			&WlrXWaylandSurface::get_height);
	ClassDB::bind_method(D_METHOD("get_min_width"),
			&WlrXWaylandSurface::get_min_width);
	ClassDB::bind_method(D_METHOD("get_min_height"),
			&WlrXWaylandSurface::get_min_height);
	ClassDB::bind_method(D_METHOD("get_max_width"),
			&WlrXWaylandSurface::get_max_width);
	ClassDB::bind_method(D_METHOD("get_max_height"),
			&WlrXWaylandSurface::get_max_height);

	ClassDB::bind_method(D_METHOD("get_parent"), &WlrXWaylandSurface::get_parent);
	ClassDB::bind_method(D_METHOD("get_title"), &WlrXWaylandSurface::get_title);
// 	ClassDB::bind_method(D_METHOD("get_app_id"), &WlrXWaylandSurface::get_app_id);
	ClassDB::bind_method(D_METHOD("set_size", "size"),
			&WlrXWaylandSurface::set_size);
	ClassDB::bind_method(D_METHOD("set_activated", "activated"),
			&WlrXWaylandSurface::set_activated);
	ClassDB::bind_method(D_METHOD("get_maximized"),
                       &WlrXWaylandSurface::get_maximized);
	ClassDB::bind_method(D_METHOD("set_maximized", "maximized"),
			&WlrXWaylandSurface::set_maximized);
	ClassDB::bind_method(D_METHOD("set_fullscreen", "fullscreen"),
			&WlrXWaylandSurface::set_fullscreen);
// 	ClassDB::bind_method(D_METHOD("set_resizing", "resizing"),
// 			&WlrXWaylandSurface::set_resizing);
// 	ClassDB::bind_method(D_METHOD("set_tiled", "tiled"),
// 			&WlrXWaylandSurface::set_tiled);
	ClassDB::bind_method(D_METHOD("send_close"), &WlrXWaylandSurface::send_close);

	ADD_SIGNAL(MethodInfo("request_maximize",
			PropertyInfo(Variant::OBJECT,
				"xwayland_surface", PROPERTY_HINT_RESOURCE_TYPE, "WlrXWaylandSurface")));

	ADD_SIGNAL(MethodInfo("destroy", PropertyInfo(Variant::OBJECT,
                                                "xwayland_surface", PROPERTY_HINT_RESOURCE_TYPE, "WlrXWaylandSurface")));
	ADD_SIGNAL(MethodInfo("map", PropertyInfo(Variant::OBJECT,
                                            "xwayland_surface", PROPERTY_HINT_RESOURCE_TYPE, "WlrXWaylandSurface")));
	ADD_SIGNAL(MethodInfo("surface_commit", PropertyInfo(Variant::OBJECT,
                                            "xwayland_surface", PROPERTY_HINT_RESOURCE_TYPE, "WlrXWaylandSurface")));
	ADD_SIGNAL(MethodInfo("unmap", PropertyInfo(Variant::OBJECT,
                                              "xwayland_surface", PROPERTY_HINT_RESOURCE_TYPE, "WlrXWaylandSurface")));


	ADD_SIGNAL(MethodInfo("request_fullscreen",
			PropertyInfo(Variant::OBJECT,
				"xwayland_surface", PROPERTY_HINT_RESOURCE_TYPE, "WlrXWaylandSurface"),
			PropertyInfo(Variant::BOOL, "fullscreen")));

  //Xwayland doesn't seem to support the minimize event.
	// ADD_SIGNAL(MethodInfo("request_minimize",
	// 		PropertyInfo(Variant::OBJECT,
	// 			"xwayland_surface", PROPERTY_HINT_RESOURCE_TYPE, "WlrXWaylandSurface")));

  //We remove the serial argument from the request_move signal.
	ADD_SIGNAL(MethodInfo("request_move",
			PropertyInfo(Variant::OBJECT,
				"xwayland_surface", PROPERTY_HINT_RESOURCE_TYPE, "WlrXWaylandSurface")
    //PropertyInfo(Variant::INT, "serial")
                        ));

	ADD_SIGNAL(MethodInfo("request_resize",
			PropertyInfo(Variant::OBJECT,
				"xwayland_surface", PROPERTY_HINT_RESOURCE_TYPE, "WlrXWaylandSurface"),
			// PropertyInfo(Variant::INT, "serial"), //We remove the serial argument in XWayland
			PropertyInfo(Variant::INT, "edges")));
	// ADD_SIGNAL(MethodInfo("request_show_window_menu",
	// 		PropertyInfo(Variant::OBJECT,
	// 			"xwayland_surface", PROPERTY_HINT_RESOURCE_TYPE, "WlrXWaylandSurface"),
	// 		PropertyInfo(Variant::INT, "serial"),
	// 		PropertyInfo(Variant::INT, "x"),
	// 		PropertyInfo(Variant::INT, "y")));

	// ADD_SIGNAL(MethodInfo("set_app_id",
	// 		PropertyInfo(Variant::OBJECT,
	// 			"xwayland_surface", PROPERTY_HINT_RESOURCE_TYPE, "WlrXWaylandSurface")));
	ADD_SIGNAL(MethodInfo("set_title",
			PropertyInfo(Variant::OBJECT,
				"xwayland_surface", PROPERTY_HINT_RESOURCE_TYPE, "WlrXWaylandSurface")));
	ADD_SIGNAL(MethodInfo("set_parent",
			PropertyInfo(Variant::OBJECT,
				"xwayland_surface", PROPERTY_HINT_RESOURCE_TYPE, "WlrXWaylandSurface")));
}

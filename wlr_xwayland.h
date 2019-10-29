#ifndef GDWLR_WLR_XWAYLAND_SURFACE_H
#define GDWLR_WLR_XWAYLAND_SURFACE_H
#include "core/func_ref.h"
#include "renderer.h"
#include "scene/main/node.h"
#include "wayland_display.h"
#include "wayland_global.h"
#include "wlr_surface.h"
#include "wlr_compositor.h"
#include "wlr_output.h"
#include "wlr_seat.h"
//#include "xwayland/xwm.h" We are unable to access this :(
extern "C" {
/* #include <wlr/types/wlr_output_damage.h> */
/* #include <wlr/xcursor.h> */
/* #include <wlr/types/wlr_xcursor_manager.h> */
#include <wayland-server.h>

//We override xwayland.h to avoid the `class` keyword
//#include <wlr/xwayland.h>
#include "xwayland.h"
/* #include "xwayland/xwm.h" */
}

class WlrXWaylandSurface: public Resource {
 private:
  GDCLASS(WlrXWaylandSurface, Resource);
  struct wlr_xwayland_surface *wlr_xwayland_surface;

	static void handle_request_maximize(
			struct wl_listener *listener, void *data);
	static void handle_request_fullscreen(struct wl_listener *listener, void *data);
	static void handle_request_minimize(struct wl_listener *listener, void *data);
	static void handle_request_move(struct wl_listener *listener, void *data);
	static void handle_request_resize(struct wl_listener *listener, void *data);
	//static void handle_request_show_window_menu( // We elminate this event/signal in xwayland
  //    struct wl_listener *listener, void *data); //"
	static void handle_set_parent(struct wl_listener *listener, void *data);

	struct wl_listener request_maximize;
	struct wl_listener request_fullscreen;
	struct wl_listener request_minimize;
	struct wl_listener request_move;
	struct wl_listener request_resize;
	struct wl_listener request_show_window_menu;
	struct wl_listener set_parent;
	struct wl_listener surface_commit;

	struct wl_listener destroy;
	struct wl_listener map;
	struct wl_listener unmap;

	struct wl_listener request_configure;
	struct wl_listener set_title;
	struct wl_listener set_class;

	/* static void handle_set_title(struct wl_listener *listener, void *data); */
	/* static void handle_set_class(struct wl_listener *listener, void *data); */
	static void handle_request_configure(struct wl_listener *listener, void *data);
	/* static void handle_surface_commit(struct wl_listener *listener, void *data); */
	static void handle_destroy(struct wl_listener *listener, void *data);
	static void handle_map(struct wl_listener *listener, void *data);
	static void handle_unmap(struct wl_listener *listener, void *data);

  Array children;

 protected:
	static void _bind_methods();
	WlrXWaylandSurface(); /* Necessary for Object */
	WlrXWaylandSurface(struct wlr_xwayland_surface *xwayland_surface);
 public:
	/* struct wl_listener handle_set_title; */
	/* struct wl_listener handle_set_class; */
	static WlrXWaylandSurface *from_wlr_xwayland_surface(struct wlr_xwayland_surface *xwayland_surface);
  Array get_children();

	/* enum TilingEdges { */
  /*                   TILING_EDGE_NONE = 0, */
  /*                   TILING_EDGE_TOP = 1, */
  /*                   TILING_EDGE_BOTTOM = 2, */
  /*                   TILING_EDGE_LEFT = 4, */
  /*                   TILING_EDGE_RIGHT = 8, */
	/* }; */

	bool get_maximized() const;
	bool get_fullscreen() const;
	bool get_resizing() const;
	bool get_activated() const;
	bool get_tiled() const;
	String get_role() const;
	uint32_t get_width() const;
	uint32_t get_height() const;
	uint32_t get_min_width() const;
	uint32_t get_min_height() const;
	uint32_t get_max_width() const;
	uint32_t get_max_height() const;
	uint32_t get_x() const;
	void print_xwayland_surface_properties();
	uint32_t get_y() const;

	/* WlrXWaylandSurface *get_client_pending_state() const; */
	/* WlrXWaylandSurface *get_server_pending_state() const; */
	/* WlrXWaylandSurface *get_current_state() const; */
	WlrXWaylandSurface *get_parent() const;
	String get_title() const;
	/* String get_app_id() const; */

	void set_size(Vector2 size);
	void set_activated(bool activated);
	void set_maximized(bool maximized);
	void set_fullscreen(bool fullscreen);
	/* void set_resizing(bool resizing); */
	/* void set_tiled(bool tiled); */
	void send_close();


	WlrSurface *get_wlr_surface() const;
	Rect2 get_geometry();
	void for_each_surface(Variant func);
	/* void schedule_frame(Variant _output); */
	WlrSurfaceAtResult *surface_at(double sx, double sy);
	void delete_wlr_xwayland_surface();
};

class WlrXWayland: public WaylandGlobal {
 private:
	GDCLASS(WlrXWayland, Node);

	struct wlr_xwayland *wlr_xwayland;
  /* WaylandDisplay *waylandDisplay = NULL; */ //we already have this via WaylandGlobal

	void ensure_wl_global(WaylandDisplay *display);
	void destroy_wl_global(WaylandDisplay *display);

	struct wl_listener new_xwayland_surface;

	static void handle_new_xwayland_surface(
                struct wl_listener *listener, void *data);
 protected:
	static void _bind_methods();
 public:
  void start_xwayland(Variant _compositor, Variant _seat);
  /* void set_cursor(); */
	WlrXWayland(WlrCompositor * WlrCompositor);
	WlrXWayland();
	~WlrXWayland();
};

#endif
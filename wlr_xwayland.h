#ifndef GDWLR_WLR_XDG_SHELL_H
#define GDWLR_WLR_XDG_SHELL_H
#include "core/func_ref.h"
#include "renderer.h"
#include "scene/main/node.h"
#include "wayland_display.h"
#include "wayland_global.h"
#include "wlr_surface.h"
extern "C" {
#include <wayland-server.h>
//#include <wlr/xwayland.h>
#include "xwayland.h"
}

class WlrXWaylandSurface: public Resource {
 private:
  GDCLASS(WlrXWaylandSurface, Resource);
  struct wlr_xwayland_surface *wlr_xwayland_surface;

	static void handle_request_maximize(
			struct wl_listener *listener, void *data);
	/* static void handle_request_fullscreen( */
	/* 		struct wl_listener *listener, void *data); */
	/* static void handle_request_minimize(struct wl_listener *listener, void *data); */
	/* static void handle_request_move(struct wl_listener *listener, void *data); */
	/* static void handle_request_resize(struct wl_listener *listener, void *data); */
	/* static void handle_request_show_window_menu( */
	/* 		struct wl_listener *listener, void *data); */
	/* static void handle_set_parent(struct wl_listener *listener, void *data); */
	/* static void handle_set_title(struct wl_listener *listener, void *data); */
	/* static void handle_set_app_id(struct wl_listener *listener, void *data); */

	struct wl_listener request_maximize;
	/* struct wl_listener request_fullscreen; */
	/* struct wl_listener request_minimize; */
	/* struct wl_listener request_move; */
	/* struct wl_listener request_resize; */
	/* struct wl_listener request_show_window_menu; */
	/* struct wl_listener set_parent; */
	/* struct wl_listener set_title; */
	/* struct wl_listener set_app_id; */

	/* struct wl_listener destroy; */
	/* struct wl_listener map; */
	/* struct wl_listener unmap; */

	/* static void handle_destroy(struct wl_listener *listener, void *data); */
	/* static void handle_map(struct wl_listener *listener, void *data); */
	/* static void handle_unmap(struct wl_listener *listener, void *data); */
 protected:
	static void _bind_methods();
	WlrXWaylandSurface(); /* Necessary for Object */
	WlrXWaylandSurface(struct wlr_xwayland_surface *xwayland_surface);
 public:
	/* static WlrXdgPopup *from_wlr_xwayland_surface(struct wlr_xwayland_surface *xwayland_surface); */

	/* enum TilingEdges { */
  /*                   TILING_EDGE_NONE = 0, */
  /*                   TILING_EDGE_TOP = 1, */
  /*                   TILING_EDGE_BOTTOM = 2, */
  /*                   TILING_EDGE_LEFT = 4, */
  /*                   TILING_EDGE_RIGHT = 8, */
	/* }; */

	/* enum XdgSurfaceRole { */
  /*                      XDG_SURFACE_ROLE_NONE, */
  /*                      XDG_SURFACE_ROLE_TOPLEVEL, */
  /*                      XDG_SURFACE_ROLE_POPUP, */
	/* }; */

	bool get_maximized() const;
	/* bool get_fullscreen() const; */
	/* bool get_resizing() const; */
	/* bool get_activated() const; */
	/* bool get_tiled() const; */
	/* uint32_t get_width() const; */
	/* uint32_t get_height() const; */
	/* uint32_t get_min_width() const; */
	/* uint32_t get_min_height() const; */
	/* uint32_t get_max_width() const; */
	/* uint32_t get_max_height() const; */

	/* WlrXdgToplevelState *get_client_pending_state() const; */
	/* WlrXdgToplevelState *get_server_pending_state() const; */
	/* WlrXdgToplevelState *get_current_state() const; */
	/* WlrXdgToplevel *get_parent() const; */
	/* String get_title() const; */
	/* String get_app_id() const; */

	/* void set_size(Vector2 size); */
	/* void set_activated(bool activated); */
	void set_maximized(bool maximized);
	/* void set_fullscreen(bool fullscreen); */
	/* void set_resizing(bool resizing); */
	/* void set_tiled(bool tiled); */
	/* void send_close(); */

	/* XdgSurfaceRole get_role() const; */
	/* WlrXdgToplevel *get_xdg_toplevel() const; */
	/* WlrXdgPopup *get_xdg_popup() const; */

	/* WlrSurface *get_wlr_surface() const; */
	/* Rect2 get_geometry(); */
	/* void for_each_surface(Variant func); */
	/* //void for_each_surface_ffi(surface_iter_t func); */
	/* void for_each_surface_ffi(void * func); */
	/* WlrSurfaceAtResult *surface_at(double sx, double sy); */
};

//VARIANT_ENUM_CAST(WlrXdgSurface::XdgSurfaceRole);

class WlrXWayland: public WaylandGlobal {
 private:
	GDCLASS(WlrXWayland, Node);

	struct wlr_xwayland *wlr_xwayland;

	/* void ensure_wl_global(WaylandDisplay *display); */
	/* void destroy_wl_global(WaylandDisplay *display); */

	/* struct wl_listener new_xdg_surface; */

	/* static void handle_new_xdg_surface( */
  /*                                    struct wl_listener *listener, void *data); */
 protected:
	static void _bind_methods();
 public:
	WlrXWayland(); //Probably don't need a variant of this with a wlr_xdg_shell because we construct one on the fly (unlike with WlrXwaylandSurface)
	~WlrXWayland();
};

#endif

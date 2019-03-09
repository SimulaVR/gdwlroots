#ifndef GDWLR_WLR_XDG_SHELL_H
#define GDWLR_WLR_XDG_SHELL_H
#include "renderer.h"
#include "scene/main/node.h"
#include "wayland_display.h"
#include "wayland_global.h"
#include "wlr_surface.h"
extern "C" {
#include <wayland-server.h>
#include <wlr/types/wlr_xdg_shell.h>
}

class WlrXdgPopup : public Object {
	GDCLASS(WlrXdgPopup, Object);

	struct wlr_xdg_popup *wlr_xdg_popup;

protected:
	static void _bind_methods();

	WlrXdgPopup(); /* Necessary for Object */
	WlrXdgPopup(struct wlr_xdg_popup *xdg_popup);

public:
	static WlrXdgPopup *from_wlr_xdg_popup(struct wlr_xdg_popup *xdg_popup);
};

class WlrXdgToplevelState : public Object {
	GDCLASS(WlrXdgToplevelState, Object);

protected:
	static void _bind_methods();

	/* Necessary for Object */
	WlrXdgToplevelState();
};

class WlrXdgToplevel : public Object {
	GDCLASS(WlrXdgToplevel, Object);

	struct wlr_xdg_toplevel *wlr_xdg_toplevel;

	static void handle_request_maximize(
			struct wl_listener *listener, void *data);
	static void handle_request_fullscreen(
			struct wl_listener *listener, void *data);
	static void handle_request_minimize(struct wl_listener *listener, void *data);
	static void handle_request_move(struct wl_listener *listener, void *data);
	static void handle_request_resize(struct wl_listener *listener, void *data);
	static void handle_request_show_window_menu(
			struct wl_listener *listener, void *data);
	static void handle_set_parent(struct wl_listener *listener, void *data);
	static void handle_set_title(struct wl_listener *listener, void *data);
	static void handle_set_app_id(struct wl_listener *listener, void *data);

	struct wl_listener request_maximize;
	struct wl_listener request_fullscreen;
	struct wl_listener request_minimize;
	struct wl_listener request_move;
	struct wl_listener request_resize;
	struct wl_listener request_show_window_menu;
	struct wl_listener set_parent;
	struct wl_listener set_title;
	struct wl_listener set_app_id;

protected:
	static void _bind_methods();

	WlrXdgToplevel(); /* Necessary for Object */
	WlrXdgToplevel(struct wlr_xdg_toplevel *xdg_toplevel);

public:
	static WlrXdgToplevel *from_wlr_xdg_toplevel(
			struct wlr_xdg_toplevel *xdg_toplevel);

	WlrXdgToplevelState *get_client_pending_state() const;
	WlrXdgToplevelState *get_server_pending_state() const;
	WlrXdgToplevelState *get_current_state() const;
	WlrXdgToplevel *get_parent() const;
	String get_title() const;
	String get_app_id() const;

	void set_size(Vector2 size);
	void set_activated(bool activated);
	void set_maximized(bool maximized);
	void set_fullscreen(bool fullscreen);
	void set_resizing(bool resizing);
	void set_tiled(bool tiled);
	void send_close();
};

class WlrXdgSurface : public Resource {
	GDCLASS(WlrXdgSurface, Resource);
	friend class WlrXdgToplevel;
	friend class WlrXdgPopup;

	WlrXdgToplevel *toplevel = NULL;
	WlrXdgPopup *popup = NULL;

	struct wlr_xdg_surface *wlr_xdg_surface;

	struct wl_listener destroy;
	struct wl_listener map;
	struct wl_listener unmap;

	static void handle_destroy(struct wl_listener *listener, void *data);
	static void handle_map(struct wl_listener *listener, void *data);
	static void handle_unmap(struct wl_listener *listener, void *data);

protected:
	static void _bind_methods();

	WlrXdgSurface(); /* Necessary for Object */
	WlrXdgSurface(struct wlr_xdg_surface *xdg_surface);

public:
	enum XdgSurfaceRole {
		XDG_SURFACE_ROLE_NONE,
		XDG_SURFACE_ROLE_TOPLEVEL,
		XDG_SURFACE_ROLE_POPUP,
	};

	XdgSurfaceRole get_role() const;
	WlrXdgToplevel *get_xdg_toplevel() const;
	WlrXdgPopup *get_xdg_popup() const;

	WlrSurface *get_wlr_surface() const;
	Rect2 get_geometry();

	static WlrXdgSurface *from_wlr_xdg_surface(
			struct wlr_xdg_surface *xdg_surface);
};

VARIANT_ENUM_CAST(WlrXdgSurface::XdgSurfaceRole);

class WlrXdgShell : public WaylandGlobal {
	GDCLASS(WlrXdgShell, Node);

	struct wlr_xdg_shell *wlr_xdg_shell;

	void ensure_wl_global(WaylandDisplay *display);
	void destroy_wl_global(WaylandDisplay *display);

	struct wl_listener new_xdg_surface;

	static void handle_new_xdg_surface(
			struct wl_listener *listener, void *data);

protected:
	static void _bind_methods();

public:
	WlrXdgShell();
	~WlrXdgShell();
};

#endif

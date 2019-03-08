#ifndef GDWLR_WLR_XDG_SHELL_H
#define GDWLR_WLR_XDG_SHELL_H
#include "renderer.h"
#include "scene/main/node.h"
#include "wayland_display.h"
#include "wlr_surface.h"
extern "C" {
#include <wayland-server.h>
#include <wlr/types/wlr_xdg_shell.h>
}

class WlrXdgPopup : public Object {
	GDCLASS(WlrXdgPopup, Object);

protected:
	static void _bind_methods();

	/* Necessary for Object */
	WlrXdgPopup();
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

	/* Necessary for Object */
	WlrXdgToplevel();

public:
	WlrXdgToplevel(struct wlr_xdg_toplevel *xdg_toplevel);

	WlrXdgToplevelState *get_client_pending_state() const;
	WlrXdgToplevelState *get_server_pending_state() const;
	WlrXdgToplevelState *get_current_state() const;
	String get_title() const;
	String get_app_id() const;
};

class WlrXdgSurface : public Resource {
	GDCLASS(WlrXdgSurface, Resource);

	struct wlr_xdg_surface *wlr_xdg_surface;

	struct wl_listener destroy;

	static void handle_destroy(struct wl_listener *listener, void *data);

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

class WlrXdgShell : public Node {
	GDCLASS(WlrXdgShell, Node);

	struct wlr_xdg_shell *wlr_xdg_shell;

	void ensure_wlr_xdg_shell();
	WaylandDisplay *get_wayland_display();

	struct wl_listener new_xdg_surface;

	static void handle_new_xdg_surface(
			struct wl_listener *listener, void *data);

protected:
	void _notification(int p_what);

	static void _bind_methods();

public:
	WlrXdgShell();
	~WlrXdgShell();
};

#endif

#ifndef GDWLR_WLR_XDG_SHELL_H
#define GDWLR_WLR_XDG_SHELL_H
#include "renderer.h"
#include "scene/main/node.h"
#include "wayland_display.h"
#include "wayland_global.h"
#include "wlr_surface.h"

namespace wlr {
extern "C" {
#include <wayland-server.h>
#include <wlr/types/wlr_xdg_shell.h>
}
}


typedef void (*surface_iter_t)(WlrSurface * surface, int sx, int s);

class WlrXdgPopup : public Object {
	GDCLASS(WlrXdgPopup, Object);

		struct wlr::wlr_xdg_popup *wlr_xdg_popup;

	protected:
		static void _bind_methods();

		WlrXdgPopup(); /* Necessary for Object */
		WlrXdgPopup(struct wlr::wlr_xdg_popup *xdg_popup);

	public:
		int get_x();
		int get_y();
		int get_width();
		int get_height();
		Rect2 get_geometry();
		static WlrXdgPopup *from_wlr_xdg_popup(struct wlr::wlr_xdg_popup *xdg_popup);
	};

	class WlrXdgToplevelState : public Object {
		GDCLASS(WlrXdgToplevelState, Object);

		struct wlr::wlr_xdg_toplevel_state *state;

	protected:
		static void _bind_methods();

	public:
		enum TilingEdges {
			TILING_EDGE_NONE = 0,
			TILING_EDGE_TOP = 1,
			TILING_EDGE_BOTTOM = 2,
			TILING_EDGE_LEFT = 4,
			TILING_EDGE_RIGHT = 8,
		};

		bool get_maximized() const;
		bool get_fullscreen() const;
		bool get_resizing() const;
		bool get_activated() const;
		bool get_tiled() const;
		uint32_t get_width() const;
		uint32_t get_height() const;
		uint32_t get_min_width() const;
		uint32_t get_min_height() const;
		uint32_t get_max_width() const;
		uint32_t get_max_height() const;

		/* Necessary for Object */
		WlrXdgToplevelState();
	};

	class WlrXdgToplevel : public Object {
		GDCLASS(WlrXdgToplevel, Object);

		struct wlr::wlr_xdg_toplevel *wlr_xdg_toplevel;

		static void handle_request_maximize(
				struct wlr::wl_listener *listener, void *data);
		static void handle_request_fullscreen(
				struct wlr::wl_listener *listener, void *data);
		static void handle_request_minimize(struct wlr::wl_listener *listener, void *data);
		static void handle_request_move(struct wlr::wl_listener *listener, void *data);
		static void handle_request_resize(struct wlr::wl_listener *listener, void *data);
		static void handle_request_show_window_menu(
				struct wlr::wl_listener *listener, void *data);
		static void handle_set_parent(struct wlr::wl_listener *listener, void *data);
		static void handle_set_title(struct wlr::wl_listener *listener, void *data);
		static void handle_set_app_id(struct wlr::wl_listener *listener, void *data);

		struct wlr::wl_listener request_maximize;
		struct wlr::wl_listener request_fullscreen;
		struct wlr::wl_listener request_minimize;
		struct wlr::wl_listener request_move;
		struct wlr::wl_listener request_resize;
		struct wlr::wl_listener request_show_window_menu;
		struct wlr::wl_listener set_parent;
		struct wlr::wl_listener set_title;
		struct wlr::wl_listener set_app_id;

	protected:
		static void _bind_methods();

		WlrXdgToplevel(); /* Necessary for Object */
		WlrXdgToplevel(struct wlr::wlr_xdg_toplevel *xdg_toplevel);

	public:
		static WlrXdgToplevel *from_wlr_xdg_toplevel(
				struct wlr::wlr_xdg_toplevel *xdg_toplevel);

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

		void remove_listeners();
	};

	class WlrXdgSurface : public Resource {
		GDCLASS(WlrXdgSurface, Resource);
		friend class WlrXdgToplevel;
		friend class WlrXdgPopup;

		WlrXdgToplevel *toplevel = NULL;
		WlrXdgPopup *popup = NULL;

		struct wlr::wlr_xdg_surface *wlr_xdg_surface;

		struct wlr::wl_listener destroy;
		struct wlr::wl_listener ping_timeout;
		struct wlr::wl_listener new_popup;
		struct wlr::wl_listener map;
		struct wlr::wl_listener unmap;
		// struct wlr::wl_listener configure;
		// struct wlr::wl_listener ack_configure;

		static void handle_ping_timeout(struct wlr::wl_listener *listener, void *data);
		static void handle_destroy(struct wlr::wl_listener *listener, void *data);
		static void handle_new_popup(struct wlr::wl_listener *listener, void *data);
		static void handle_map(struct wlr::wl_listener *listener, void *data);
		static void handle_unmap(struct wlr::wl_listener *listener, void *data);
		// static void handle_configure(struct wlr::wl_listener *listener, void *data);
		// static void handle_ack_configure(struct wlr::wl_listener *listener, void *data);

		Array children;

	protected:
		static void _bind_methods();

		WlrXdgSurface(); /* Necessary for Object */
		WlrXdgSurface(struct wlr::wlr_xdg_surface *xdg_surface);

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
		void for_each_surface(Callable func);
		//void for_each_surface_ffi(surface_iter_t func);
		void for_each_surface_ffi(void * func);
		WlrSurfaceAtResult *surface_at(double sx, double sy);

		static WlrXdgSurface *from_wlr_xdg_surface(
				struct wlr::wlr_xdg_surface *xdg_surface);

		Array get_children();
	};

	VARIANT_ENUM_CAST(WlrXdgSurface::XdgSurfaceRole);

	class WlrXdgShell : public WaylandGlobal {
		GDCLASS(WlrXdgShell, Node);

		struct wlr::wlr_xdg_shell *wlr_xdg_shell;

		void ensure_wl_global(WaylandDisplay *display);
		void destroy_wl_global(WaylandDisplay *display);

		struct wlr::wl_listener new_surface;
		struct wlr::wl_listener destroy;

		static void handle_new_surface(struct wlr::wl_listener *listener, void *data);
		static void handle_destroy(struct wlr::wl_listener *listener, void *data);

protected:
	static void _bind_methods();

public:
	WlrXdgShell();
	~WlrXdgShell();
};

#endif

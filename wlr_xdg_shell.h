#ifndef GDWLR_WLR_XDG_SHELL_H
#define GDWLR_WLR_XDG_SHELL_H
#include "renderer.h"
#include "scene/main/node.h"
#include "wayland_display.h"
extern "C" {
#include <wayland-server.h>
#include <wlr/types/wlr_xdg_shell.h>
}

class WlrXdgSurface : public Object {
	GDCLASS(WlrXdgSurface, Object);

	struct wlr_xdg_surface *wlr_xdg_surface;

protected:
	static void _bind_methods();

	/* Necessary for Object */
	WlrXdgSurface();

public:
	WlrXdgSurface(struct wlr_xdg_surface *xdg_surface);
};

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

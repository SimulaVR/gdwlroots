#ifndef GDWLR_WLR_XDG_SHELL_H
#define GDWLR_WLR_XDG_SHELL_H
#include "renderer.h"
#include "scene/main/node.h"
#include "wayland_display.h"
extern "C" {
#include <wlr/types/wlr_xdg_shell.h>
}

class WlrXdgShell : public Node {
	GDCLASS(WlrXdgShell, Node);

	struct wlr_xdg_shell *wlr_xdg_shell;

	void ensure_wlr_xdg_shell();
	WaylandDisplay *get_wayland_display();

protected:
	void _notification(int p_what);

	static void _bind_methods();

public:
	WlrXdgShell();
	~WlrXdgShell();
};

#endif

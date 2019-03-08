#ifndef GDWLR_WAYLAND_NODE_H
#define GDWLR_WAYLAND_NODE_H
#include "scene/main/node.h"
#include "wayland_display.h"

class WaylandGlobal : public Node {
protected:
	WaylandDisplay *get_wayland_display();
	virtual void ensure_wl_global(WaylandDisplay *display) = 0;
	virtual void destroy_wl_global(WaylandDisplay *display) = 0;
	virtual void _notification(int p_what);
};

#endif

#ifndef GDWLR_WAYLAND_DISPLAY_H
#define GDWLR_WAYLAND_DISPLAY_H
#include "scene/main/node.h"
#include <wayland-server.h>

class WaylandDisplay : public Node {
	GDCLASS(WaylandDisplay, Node);

	struct wl_display *wl_display;
	struct wl_event_loop *wl_event_loop;
	String socket_name;

protected:
	void _notification(int p_what);

	static void _bind_methods();

public:
	String get_socket_name() const;
	void set_socket_name(const String &name);
	void run();
	struct wl_display *get_wayland_display();

	WaylandDisplay();
	~WaylandDisplay();
};

#endif

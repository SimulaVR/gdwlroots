#ifndef GDWLR_WLR_OUTPUT_H
#define GDWLR_WLR_OUTPUT_H
#include "scene/main/node.h"
#include "scene/main/viewport.h"
extern "C" {
#include <wlr/types/wlr_output.h>
}

class WlrOutput : public Node {
	GDCLASS(WlrOutput, Node);

	void _size_changed();

	Viewport *viewport;
	struct wlr_output *wlr_output;
	WaylandDisplay *get_wayland_display();

protected:
	void _notification(int p_what);

	static void _bind_methods();

public:
	WlrOutput();
	~WlrOutput();
};

#endif

#ifndef GDWLR_WLR_OUTPUT_H
#define GDWLR_WLR_OUTPUT_H
#include "scene/main/node.h"
#include "scene/main/viewport.h"
#include "wayland_display.h"
#include "wlr_backend.h"
extern "C" {
#include <wlr/types/wlr_output.h>
}

class WlrOutput : public Node {
	GDCLASS(WlrOutput, Node);

	Viewport *viewport;
	struct wlr_output *wlr_output;

	void _size_changed();
	void ensure_wlr_output();
	WaylandDisplay *get_wayland_display();
	WlrBackend *get_wlr_backend();

protected:
	void _notification(int p_what);

	static void _bind_methods();

public:
	WlrOutput();
	~WlrOutput();
};

#endif

#ifndef GDWLR_WLR_OUTPUT_H
#define GDWLR_WLR_OUTPUT_H
#include "scene/main/node.h"
#include "scene/main/viewport.h"
#include "wayland_display.h"
#include "wayland_global.h"
#include "wlr_backend.h"
extern "C" {
#include <wlr/types/wlr_output.h>
}

class WlrOutput : public WaylandGlobal {
	GDCLASS(WlrOutput, Node);

	Viewport *viewport;
	struct wlr_output *wlr_output;

	void _size_changed();
	void ensure_wl_global(WaylandDisplay *display);
	void destroy_wl_global(WaylandDisplay *display);
	WlrBackend *get_wlr_backend();

protected:
	static void _bind_methods();
	virtual void _notification(int p_what);

public:
	WlrOutput();
	~WlrOutput();
  struct wlr_output *get_wlr_output() const;
};

#endif

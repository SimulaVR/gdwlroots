#ifndef GDWLR_WLR_SURFACE_H
#define GDWLR_WLR_SURFACE_H
#include "core/object.h"
extern "C" {
#include <wlr/types/wlr_surface.h>
}

class WlrSurfaceState : public Object {
	GDCLASS(WlrSurfaceState, Object);

	const struct wlr_surface_state *state;

protected:
	static void _bind_methods();

	/* Necessary for Object */
	WlrSurfaceState();

public:
	int get_width();
	int get_height();
	int get_buffer_width();
	int get_buffer_height();
	int get_scale();

	WlrSurfaceState(const struct wlr_surface_state *state);
};

class WlrSurface : public Object {
	GDCLASS(WlrSurface, Object);

	struct wlr_surface *wlr_surface;

protected:
	static void _bind_methods();

	/* Necessary for Object */
	WlrSurface();

public:
	int get_sx();
	int get_sy();
	WlrSurfaceState *get_current_state() const;
	WlrSurfaceState *get_pending_state() const;
	WlrSurfaceState *get_previous_state() const;

	WlrSurface(struct wlr_surface *surface);
};

#endif

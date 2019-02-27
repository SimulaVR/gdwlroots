#ifndef GDWLR_WLR_SURFACE_H
#define GDWLR_WLR_SURFACE_H
#include "core/object.h"
#include "scene/resources/texture.h"
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

class WlrSurface : public Resource {
	GDCLASS(WlrSurface, Resource);

	struct wlr_surface *wlr_surface;

protected:
	static void _bind_methods();

	WlrSurface(); // Necessary for Object
	WlrSurface(struct wlr_surface *surface);

public:
	static WlrSurface *from_wlr_surface(struct wlr_surface *wlr_surface);

	int get_sx();
	int get_sy();
	WlrSurfaceState *get_current_state() const;
	WlrSurfaceState *get_pending_state() const;
	WlrSurfaceState *get_previous_state() const;
	Ref<Texture> get_texture() const;
	void send_frame_done();
};

#endif

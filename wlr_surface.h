#ifndef GDWLR_WLR_SURFACE_H
#define GDWLR_WLR_SURFACE_H
#include "core/object.h"
#include "scene/resources/texture.h"
extern "C" {
#include <wlr/types/wlr_surface.h>
}

class WlrSurfaceState : public Resource {
	GDCLASS(WlrSurfaceState, Resource);

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
  void delete_state();

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

	struct wlr_surface *get_wlr_surface() const;

	int get_sx();
	int get_sy();
	WlrSurfaceState *alloc_current_state() const;
	WlrSurfaceState *alloc_pending_state() const;
	WlrSurfaceState *alloc_previous_state() const;
	Ref<Texture> get_texture() const;
	Array get_damage_regions() const;
	void send_frame_done();
};

class WlrSurfaceAtResult : public Reference {
	GDCLASS(WlrSurfaceAtResult, Reference);

	WlrSurface *surface;
	double sub_x, sub_y;

protected:
	static void _bind_methods();

	/* Necessary for Object */
	WlrSurfaceAtResult();

public:
	WlrSurface *get_surface();
	double get_sub_x();
	double get_sub_y();

	WlrSurfaceAtResult(WlrSurface *surface, double sub_x, double sub_y);
};

#endif
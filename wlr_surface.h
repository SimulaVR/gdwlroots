#ifndef GDWLR_WLR_SURFACE_H
#define GDWLR_WLR_SURFACE_H
#include "core/object.h"
#include "scene/resources/texture.h"
#include "wlr_output.h"
extern "C" {
#include <wlr/types/wlr_surface.h>
#include "wlr/interfaces/wlr_output.h"
}

class WlrSurfaceAtResult;

class WlrSurfaceState : public Resource {
	GDCLASS(WlrSurfaceState, Resource);

	const struct wlr_surface_state *state;

protected:
	static void _bind_methods();

	/* Necessary for Object */
	WlrSurfaceState();

public:
	int32_t get_dx();
	int32_t get_dy();
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
  Array children;

  static void handle_new_subsurface(struct wl_listener *listener, void *data);
  struct wl_listener new_subsurface;
  static void handle_destroy(struct wl_listener *listener, void *data);
  struct wl_listener destroy;
  static void handle_commit(struct wl_listener *listener, void *data);
  struct wl_listener commit;

  protected:
  static void _bind_methods();

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
	Array accumulate_damage_regions(Array surfaces, Array xs, Array ys);
	Array get_damage_regions_with_damage(Array gsvsDamageBoxes) const;
  Array get_opaque_regions() const;
  Array get_input_regions() const;
  void send_frame_done();

  Array get_children();
	WlrSurfaceAtResult *surface_at(double sx, double sy);
	WlrSurface *get_root_surface();
	void surface_send_leave(Object * _output);

	bool is_wlr_subsurface();
	bool is_wlr_xwayland_surface();
	bool is_wlr_xdg_surface();
	bool is_valid();

	WlrSurface(); // Necessary for Object
	WlrSurface(struct wlr_surface *surface);
};

class WlrSubsurface : public Resource {
	GDCLASS(WlrSubsurface, Resource);

	struct wlr_subsurface *wlr_subsurface;
  Array children;

  static void handle_destroy(struct wl_listener *listener, void *data);
  struct wl_listener destroy;

 protected:
	static void _bind_methods();

	WlrSubsurface(); // Necessary for Object
	WlrSubsurface(struct wlr_subsurface *subsurface);

 public:

	static WlrSubsurface *from_wlr_subsurface(struct wlr_subsurface *wlr_subsurface);

	WlrSurface *from_wlr_surface(struct wlr_surface *wlr_surface);

	WlrSurface *getWlrSurface();

	struct wlr_subsurface *get_wlr_subsurface() const;

	int get_ssx();
	int get_ssy();

	Ref<Texture> get_texture() const;

  Array get_children();

	WlrSurface *get_wlr_surface_parent();
	WlrSurface *get_wlr_surface();
	bool is_valid();

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

#include <time.h>
#include "renderer.h"
#include "wlr_surface.h"
#include "wlr_output.h"
#include <iostream>
extern "C" {
#include <wlr/types/wlr_compositor.h>
#include <wlr/types/wlr_surface.h>
#include "wlr/interfaces/wlr_output.h"
#include <wlr/util/log.h>

#include <wlr/types/wlr_xdg_shell.h>
//We override xwayland.h to avoid the `class` keyword
#include "xwayland.h" //as opposed to: <wlr/xwayland.h>
}

WlrSurface *WlrSurfaceAtResult::get_surface() {
	return surface;
}

double WlrSurfaceAtResult::get_sub_x() {
	return sub_x;
}

double WlrSurfaceAtResult::get_sub_y() {
	return sub_y;
}

void WlrSurfaceAtResult::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_sub_x"), &WlrSurfaceAtResult::get_sub_x);
	ClassDB::bind_method(D_METHOD("get_sub_y"), &WlrSurfaceAtResult::get_sub_y);
	ClassDB::bind_method(D_METHOD("get_surface"),
											 &WlrSurfaceAtResult::get_surface);
}

WlrSurfaceAtResult::WlrSurfaceAtResult() {
	/* Not used */
}

WlrSurfaceAtResult::WlrSurfaceAtResult(WlrSurface *surface,
																			 double sub_x, double sub_y) {
	this->surface = surface;
	this->sub_x = sub_x;
	this->sub_y = sub_y;
}



int WlrSurfaceState::get_width() {
	return state->width;
}

int WlrSurfaceState::get_height() {
	return state->height;
}

int32_t WlrSurfaceState::get_dx() {
	return state->dx;
}

int32_t WlrSurfaceState::get_dy() {
	return state->dy;
}

int WlrSurfaceState::get_buffer_width() {
	return state->buffer_width;
}

int WlrSurfaceState::get_buffer_height() {
	return state->buffer_height;
}

int WlrSurfaceState::get_scale() {
	return state->scale;
}

void WlrSurfaceState::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_width"), &WlrSurfaceState::get_width);
	ClassDB::bind_method(D_METHOD("get_height"), &WlrSurfaceState::get_height);
	ClassDB::bind_method(D_METHOD("get_buffer_width"),
			&WlrSurfaceState::get_buffer_width);
	ClassDB::bind_method(D_METHOD("get_buffer_height"),
			&WlrSurfaceState::get_buffer_height);
	ClassDB::bind_method(D_METHOD("get_scale"), &WlrSurfaceState::get_scale);
	ClassDB::bind_method(D_METHOD("delete_state"), &WlrSurfaceState::delete_state);
	ClassDB::bind_method(D_METHOD("get_dx"), &WlrSurfaceState::get_dx);
	ClassDB::bind_method(D_METHOD("get_dy"), &WlrSurfaceState::get_dy);
}

WlrSurfaceState::WlrSurfaceState() {
	/* Not used */
}

WlrSurfaceState::WlrSurfaceState(const struct wlr_surface_state *state) {
	this->state = state;
}

bool WlrSurface::is_wlr_subsurface() {
  return wlr_surface_is_subsurface(wlr_surface);
}

bool WlrSurface::is_wlr_xwayland_surface() {
  return wlr_surface_is_xwayland_surface(wlr_surface);
}

bool WlrSurface::is_wlr_xdg_surface() {
  return wlr_surface_is_xdg_surface(wlr_surface);
}

struct wlr_surface *WlrSurface::get_wlr_surface() const {
	return wlr_surface;
}

int WlrSurface::get_sx() {
	return wlr_surface->sx;
}

int WlrSurface::get_sy() {
	return wlr_surface->sy;
}

WlrSurfaceState *WlrSurface::alloc_current_state() const {
	return memnew(WlrSurfaceState(&wlr_surface->current));
}

WlrSurfaceState *WlrSurface::alloc_pending_state() const {
	return memnew(WlrSurfaceState(&wlr_surface->pending));
}

WlrSurfaceState *WlrSurface::alloc_previous_state() const {
	return memnew(WlrSurfaceState(&wlr_surface->previous));
}

void WlrSurfaceState::delete_state() {
  memdelete(this);
}

Array WlrSurface::get_damage_regions() const {
	Array out;

	if (!wlr_surface_has_buffer(this->wlr_surface)) {
		return out;
  }

	pixman_region32_t dmg;
	pixman_region32_init(&dmg);
	wlr_surface_get_effective_damage(this->wlr_surface, &dmg);
	int len = -1;
	pixman_box32_t* rects =	pixman_region32_rectangles(&dmg, &len);


	for (int i = 0; i < len; i++) {
		real_t x1 = rects[i].x1;
		real_t x2 = rects[i].x2;
		real_t y1 = rects[i].y1;
		real_t y2 = rects[i].y2;

		real_t w = x2 - x1;
		real_t h = y2 - y1;

		Rect2 gRect(x1, y1, w, h);

		out.push_back(gRect);
	}

	pixman_region32_fini(&dmg);
	return out;
}

// We assume that `gsvsDamageBoxes` are already in surface-local coordinates
// WARNING: Experimental/untested function.
Array WlrSurface::get_damage_regions_with_damage(Array gsvsDamageBoxes) const {
  Array out;

  //Load the surface damage in `dmg`
  pixman_region32_t dmg;
  pixman_region32_init(&dmg);
  wlr_surface_get_effective_damage(this->wlr_surface, &dmg);

  //Add each gsvsDamageBox
  for (int k = 0; k < gsvsDamageBoxes.size(); k++) {
    Variant _gsvsDamageBox = gsvsDamageBoxes.pop_front();
	  //Rect2 gsvsDamageBox = dynamic_cast<Rect2>((Rect2) _gsvsDamageBox);
	  Rect2 gsvsDamageBox = (Rect2) _gsvsDamageBox;
    float x = gsvsDamageBox.position.x;
    float y = gsvsDamageBox.position.y;
    float w = gsvsDamageBox.size.width;
    float h = gsvsDamageBox.size.height;


    //Create a new region with just the gsvsDamageBox
    pixman_region32_t gsvsPixDmg;
    pixman_region32_init(&gsvsPixDmg);

		//David suggestion
		// pixman_region32_union_rect(&gsvsPixDmg, &gsvsPixDmg, x, y, w, h);
		// pixman_region32_union(&dmg, &gsvsPixDmg, &dmg);

    pixman_region32_union_rect(&gsvsPixDmg, &gsvsPixDmg, x, y, w, h);
    //Add it to the existing surface damage (clipping off at surface bounds)
    pixman_region32_union_rect(&dmg, &gsvsPixDmg, 0, 0, wlr_surface->current.width, wlr_surface->current.height);

    pixman_region32_fini(&gsvsPixDmg);
  }

	// can be done once outside of the loop
	pixman_region32_intersect_rect(&dmg, &dmg, 0, 0, wlr_surface->current.width, wlr_surface->current.height);

  //resume the old get_damage_regions:
  if (!wlr_surface_has_buffer(this->wlr_surface)) {
    return out;
  }

	int len = -1;
	pixman_box32_t* rects =	pixman_region32_rectangles(&dmg, &len);

  for (int i = 0; i < len; i++) {
    real_t x1 = rects[i].x1;
    real_t x2 = rects[i].x2;
    real_t y1 = rects[i].y1;
    real_t y2 = rects[i].y2;

    real_t w = x2 - x1;
    real_t h = y2 - y1;

    Rect2 gRect(x1, y1, w, h);

    out.push_back(gRect);
  }

  pixman_region32_fini(&dmg);
  return out;
}

Array WlrSurface::get_opaque_regions() const {
	Array out;


	pixman_region32_t opaque_region = wlr_surface->opaque_region;

	int len = -1;

	pixman_box32_t* rects =    pixman_region32_rectangles(&opaque_region, &len);

	for (int i = 0; i < len; i++) {
		real_t x1 = rects[i].x1;
		real_t x2 = rects[i].x2;
		real_t y1 = rects[i].y1;
		real_t y2 = rects[i].y2;

		real_t w = x2 - x1;
		real_t h = y2 - y1;

		Rect2 gRect(x1, y1, w, h);

		out.push_back(gRect);
	}

	return out;
}

Array WlrSurface::get_input_regions() const {
	Array out;

	pixman_region32_t input_region = wlr_surface->input_region;

	int len = -1;

	pixman_box32_t* rects =    pixman_region32_rectangles(&input_region, &len);

	for (int i = 0; i < len; i++) {
		real_t x1 = rects[i].x1;
		real_t x2 = rects[i].x2;
		real_t y1 = rects[i].y1;
		real_t y2 = rects[i].y2;

		real_t w = x2 - x1;
		real_t h = y2 - y1;

		Rect2 gRect(x1, y1, w, h);

		out.push_back(gRect);
	}

	return out;
}

Ref<Texture> WlrSurface::get_texture() const {
	struct wlr_texture *texture = wlr_surface_get_texture(wlr_surface);
	return Ref<Texture>(
			WlrRenderer::get_singleton()->texture_from_wlr(texture));
}

void WlrSurface::send_frame_done() {
	struct timespec now;
	clock_gettime(CLOCK_MONOTONIC, &now);
	wlr_surface_send_frame_done(wlr_surface, &now);
}

Array WlrSurface::get_children() {
  struct wlr_subsurface *subsurface;

  children.clear();

  wl_list_for_each(subsurface, &wlr_surface->subsurfaces, parent_link) {

    WlrSubsurface * wS;
    wS = (WlrSubsurface *)subsurface->data; //Only return children for whom we have WlrSubsurface's formed already
    Variant _wS = Variant( (Object *) wS );
    children.push_front(_wS);

    }

  return children;
}

void WlrSurface::handle_new_subsurface(struct wl_listener *listener, void *data) {
	//std::cout << "WlrSurface::handle_new_subsurface" << std::endl;
  struct wlr_subsurface * subsurface = (struct wlr_subsurface *)data;
	// std::cout << "WlrSurface::handle_new_subsurface wlr_subsurface: " << subsurface << std::endl;
	// std::cout << "WlrSurface::handle_new_subsurface wlr_subsurface->surface: " << subsurface->surface << std::endl;
	// std::cout << "WlrSurface::handle_new_subsurface wlr_subsurface->parent: " << (subsurface->parent) << std::endl;

  auto wlrSubsurface = WlrSubsurface::from_wlr_subsurface((struct wlr_subsurface *)data);
  auto wlrSurface = WlrSurface::from_wlr_surface(subsurface->parent);
  wlrSurface->emit_signal("new_subsurface", wlrSubsurface);
}

void WlrSurface::handle_destroy(
		struct wl_listener *listener, void *data) {
	//std::cout << "WlrSurface::handle_destroy(..)" << std::endl;
  struct wlr_surface * surface = (struct wlr_surface *)data;
	WlrSurface * wlrSurface = WlrSurface::from_wlr_surface(surface);
	wlrSurface->emit_signal("destroy", wlrSurface);
}


void WlrSurface::handle_commit(struct wl_listener *listener, void *data) {
	//std::cout << "WlrSurface::handle_commit(..)" << std::endl;
  struct wlr_surface * surface = (struct wlr_surface *)data;
	WlrSurface * wlrSurface = WlrSurface::from_wlr_surface(surface);
	wlrSurface->emit_signal("commit", wlrSurface);
}


WlrSurfaceAtResult *WlrSurface::surface_at(double sx, double sy) {
	double sub_x, sub_y;
	struct wlr_surface *result = wlr_surface_surface_at(wlr_surface, sx, sy, &sub_x, &sub_y);
	return memnew(WlrSurfaceAtResult(WlrSurface::from_wlr_surface(result), sub_x, sub_y));
}

void WlrSurface::surface_send_leave(Object * _output) {
  if (auto output = Object::cast_to<WlrOutput>(_output)) {
		auto wlrout = output->get_wlr_output();
		wlr_surface_send_leave(wlr_surface, wlrout);
  } else {
    std::cout << "Failed to cast to output in WlrSurface::surface_send_leave!" << std::endl;
		wlr_surface_send_leave(wlr_surface, NULL);
  }
}

WlrSurface *WlrSurface::get_root_surface() {
	struct wlr_surface *wlr_surface_get_root_surface(wlr_surface);
	WlrSurface * rootWlrSurface = WlrSurface::from_wlr_surface(wlr_surface);
	return rootWlrSurface;
}

void WlrSurface::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_sx"), &WlrSurface::get_sx);
	ClassDB::bind_method(D_METHOD("get_sy"), &WlrSurface::get_sy);
	ClassDB::bind_method(D_METHOD("alloc_current_state"),
			&WlrSurface::alloc_current_state);
	ClassDB::bind_method(D_METHOD("alloc_pending_state"),
			&WlrSurface::alloc_pending_state);
	ClassDB::bind_method(D_METHOD("alloc_previous_state"),
			&WlrSurface::alloc_previous_state);
	ClassDB::bind_method(D_METHOD("get_texture"),
			&WlrSurface::get_texture);
	ClassDB::bind_method(D_METHOD("get_damage_regions"),
			&WlrSurface::get_damage_regions);

	ClassDB::bind_method(D_METHOD("get_damage_regions_with_damage", "damage"),
											 &WlrSurface::get_damage_regions_with_damage);

	ClassDB::bind_method(D_METHOD("get_opaque_regions"),
											 &WlrSurface::get_opaque_regions);
	ClassDB::bind_method(D_METHOD("get_input_regions"),
											 &WlrSurface::get_input_regions);
	ClassDB::bind_method(D_METHOD("send_frame_done"),
			&WlrSurface::send_frame_done);
	ClassDB::bind_method(D_METHOD("surface_at", "sx", "sy"), &WlrSurface::surface_at);
	ClassDB::bind_method(D_METHOD("get_root_surface"), &WlrSurface::get_root_surface);
	ClassDB::bind_method(D_METHOD("surface_send_leave", "output"), &WlrSurface::surface_send_leave);

	ClassDB::bind_method(D_METHOD("get_children"), &WlrSurface::get_children);
	ClassDB::bind_method(D_METHOD("is_wlr_subsurface"), &WlrSurface::is_wlr_subsurface);
	ClassDB::bind_method(D_METHOD("is_wlr_xwayland_surface"), &WlrSurface::is_wlr_xwayland_surface);
	ClassDB::bind_method(D_METHOD("is_wlr_xdg_surface"), &WlrSurface::is_wlr_xdg_surface);

	ADD_SIGNAL(MethodInfo("new_subsurface", PropertyInfo(Variant::OBJECT, "surface", PROPERTY_HINT_RESOURCE_TYPE, "WlrSurface")));
	ADD_SIGNAL(MethodInfo("destroy", PropertyInfo(Variant::OBJECT, "surface", PROPERTY_HINT_RESOURCE_TYPE, "WlrSurface")));
	ADD_SIGNAL(MethodInfo("commit", PropertyInfo(Variant::OBJECT, "surface", PROPERTY_HINT_RESOURCE_TYPE, "WlrSurface")));
}

WlrSurface::WlrSurface() {
	/* Not used */
}



WlrSurface::WlrSurface(struct wlr_surface *surface) {
	wlr_log(WLR_DEBUG, "Created surface %p for %p", this, surface);
	// TODO: Handle surface destroyed
	wlr_surface = surface;
	surface->data = this;

	new_subsurface.notify = handle_new_subsurface;
	wl_signal_add(&wlr_surface->events.new_subsurface,
			&new_subsurface);

	destroy.notify = handle_destroy;
	wl_signal_add(&wlr_surface->events.destroy,
								&destroy);

	commit.notify = handle_commit;
	wl_signal_add(&wlr_surface->events.commit,
								&commit);

}

WlrSurface *WlrSurface::from_wlr_surface(struct wlr_surface *surface) {
	if (!surface) {
		return NULL;
	}
	if (surface->data) {
		auto s = (WlrSurface *)surface->data;
		return s;
	}
	return memnew(WlrSurface(surface));
}


void WlrSubsurface::handle_destroy(struct wl_listener *listener, void *data) {
	//std::cout << "WlrSubsurface::handle_destroy(..)" << std::endl;
  struct wlr_subsurface * subsurface = (struct wlr_subsurface *)data;
	WlrSubsurface * wlrSubsurface = WlrSubsurface::from_wlr_subsurface(subsurface);
	wlrSubsurface->emit_signal("destroy", wlrSubsurface);
}


WlrSurface * WlrSubsurface::get_wlr_surface() {
  return from_wlr_surface(wlr_subsurface->surface);
}

WlrSurface * WlrSubsurface::get_wlr_surface_parent() {
  return from_wlr_surface(wlr_subsurface->parent);
}

WlrSubsurface::WlrSubsurface() {
	/* Not used */
}

WlrSubsurface::WlrSubsurface(struct wlr_subsurface *subsurface) {
	wlr_subsurface = subsurface;
	subsurface->data = this;


	destroy.notify = handle_destroy;
	wl_signal_add(&wlr_subsurface->events.destroy,
								&destroy);

}

WlrSubsurface *WlrSubsurface::from_wlr_subsurface(struct wlr_subsurface *subsurface) {
	if (!subsurface) {
		return NULL;
	}
	if (subsurface->data) {
		auto s = (WlrSubsurface *)subsurface->data;
		return s;
	}
	return memnew(WlrSubsurface(subsurface));
}

struct wlr_subsurface *WlrSubsurface::get_wlr_subsurface() const {
	return wlr_subsurface;
}

int WlrSubsurface::get_ssx() {

  return wlr_subsurface->current.x;
}

int WlrSubsurface::get_ssy() {
  return wlr_subsurface->current.y;
}

Ref<Texture> WlrSubsurface::get_texture() const {
  struct wlr_surface * surface = wlr_subsurface->surface;
	struct wlr_texture *texture = wlr_surface_get_texture(surface);
	return Ref<Texture>(
			WlrRenderer::get_singleton()->texture_from_wlr(texture));
}


WlrSurface *WlrSubsurface::from_wlr_surface(struct wlr_surface *surface) {
	if (!surface) {
		return NULL;
	}
	if (surface->data) {
		auto s = (WlrSurface *)surface->data;
		return s;
	}
	return memnew(WlrSurface(surface));
}

WlrSurface *WlrSubsurface::getWlrSurface() {
  return from_wlr_surface(wlr_subsurface->surface);
}

Array WlrSubsurface::get_children() {
  struct wlr_surface * surface = wlr_subsurface->surface;
  WlrSurface * surface_ = from_wlr_surface(surface);
  return surface_->get_children();
}

void WlrSubsurface::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_ssx"), &WlrSubsurface::get_ssx);
	ClassDB::bind_method(D_METHOD("get_ssy"), &WlrSubsurface::get_ssy);
	ClassDB::bind_method(D_METHOD("get_texture"),
			&WlrSubsurface::get_texture);
	ClassDB::bind_method(D_METHOD("getWlrSurface"), &WlrSubsurface::getWlrSurface);
	ClassDB::bind_method(D_METHOD("get_wlr_surface"), &WlrSubsurface::get_wlr_surface);
	ClassDB::bind_method(D_METHOD("get_wlr_surface_parent"), &WlrSubsurface::get_wlr_surface_parent);

	ClassDB::bind_method(D_METHOD("get_children"), &WlrSubsurface::get_children);

	ADD_SIGNAL(MethodInfo("destroy", PropertyInfo(Variant::OBJECT, "surface", PROPERTY_HINT_RESOURCE_TYPE, "WlrSubsurface")));
}

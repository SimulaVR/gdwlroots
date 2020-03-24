#include <time.h>
#include "renderer.h"
#include "wlr_surface.h"
extern "C" {
#include <wlr/types/wlr_surface.h>
#include <wlr/util/log.h>
}

int WlrSurfaceState::get_width() {
	return state->width;
}

int WlrSurfaceState::get_height() {
	return state->height;
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
}

WlrSurfaceState::WlrSurfaceState() {
	/* Not used */
}

WlrSurfaceState::WlrSurfaceState(const struct wlr_surface_state *state) {
	this->state = state;
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
	return new WlrSurfaceState(&wlr_surface->current);
}

WlrSurfaceState *WlrSurface::alloc_pending_state() const {
	return new WlrSurfaceState(&wlr_surface->pending);
}

WlrSurfaceState *WlrSurface::alloc_previous_state() const {
	return new WlrSurfaceState(&wlr_surface->previous);
}

void WlrSurfaceState::delete_state() {
  delete this;
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
	ClassDB::bind_method(D_METHOD("send_frame_done"),
			&WlrSurface::send_frame_done);

	ClassDB::bind_method(D_METHOD("get_children"), &WlrSurface::get_children);
}

WlrSurface::WlrSurface() {
	/* Not used */
}

WlrSurface::WlrSurface(struct wlr_surface *surface) {
	wlr_log(WLR_DEBUG, "Created surface %p for %p", this, surface);
	// TODO: Handle surface destroyed
	wlr_surface = surface;
	surface->data = this;
}

WlrSurface *WlrSurface::from_wlr_surface(struct wlr_surface *surface) {
	if (!surface) {
		return NULL;
	}
	if (surface->data) {
		auto s = (WlrSurface *)surface->data;
		return s;
	}
	return new WlrSurface(surface);
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

WlrSubsurface::WlrSubsurface() {
	/* Not used */
}

WlrSubsurface::WlrSubsurface(struct wlr_subsurface *subsurface) {
	wlr_subsurface = subsurface;
	subsurface->data = this;
}

WlrSubsurface *WlrSubsurface::from_wlr_subsurface(struct wlr_subsurface *subsurface) {
	if (!subsurface) {
		return NULL;
	}
	if (subsurface->data) {
		auto s = (WlrSubsurface *)subsurface->data;
		return s;
	}
	return new WlrSubsurface(subsurface);
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
	return new WlrSurface(surface);
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

	ClassDB::bind_method(D_METHOD("get_children"), &WlrSubsurface::get_children);
}
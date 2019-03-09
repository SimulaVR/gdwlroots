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

WlrSurfaceState *WlrSurface::get_current_state() const {
	return new WlrSurfaceState(&wlr_surface->current);
}

WlrSurfaceState *WlrSurface::get_pending_state() const {
	return new WlrSurfaceState(&wlr_surface->pending);
}

WlrSurfaceState *WlrSurface::get_previous_state() const {
	return new WlrSurfaceState(&wlr_surface->previous);
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

void WlrSurface::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_sx"), &WlrSurface::get_sx);
	ClassDB::bind_method(D_METHOD("get_sy"), &WlrSurface::get_sy);
	ClassDB::bind_method(D_METHOD("get_current_state"),
			&WlrSurface::get_current_state);
	ClassDB::bind_method(D_METHOD("get_pending_state"),
			&WlrSurface::get_pending_state);
	ClassDB::bind_method(D_METHOD("get_previous_state"),
			&WlrSurface::get_previous_state);
	ClassDB::bind_method(D_METHOD("get_texture"),
			&WlrSurface::get_texture);
	ClassDB::bind_method(D_METHOD("send_frame_done"),
			&WlrSurface::send_frame_done);
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
	if (surface->data) {
		auto s = (WlrSurface *)surface->data;
		wlr_log(WLR_DEBUG, "Found surface %p for %p", s, surface);
		return s;
	}
	return new WlrSurface(surface);
}

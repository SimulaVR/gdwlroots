#include "wlr_surface.h"
extern "C" {
#include <wlr/types/wlr_surface.h>
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

int WlrSurface::get_sx() {
	return wlr_surface->sx;
}

int WlrSurface::get_sy() {
	return wlr_surface->sy;
}

WlrSurfaceState *WlrSurface::get_current_state() const {
	static WlrSurfaceState state = WlrSurfaceState(&wlr_surface->current);
	return &state;
}

WlrSurfaceState *WlrSurface::get_pending_state() const {
	static WlrSurfaceState state = WlrSurfaceState(&wlr_surface->pending);
	return &state;
}

WlrSurfaceState *WlrSurface::get_previous_state() const {
	static WlrSurfaceState state = WlrSurfaceState(&wlr_surface->previous);
	return &state;
}


void WlrSurface::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_sx"), &WlrSurface::get_sx);
	ClassDB::bind_method(D_METHOD("get_sx"), &WlrSurface::get_sy);
	ClassDB::bind_method(D_METHOD("get_current_state"),
			&WlrSurface::get_current_state);
	ClassDB::bind_method(D_METHOD("get_pending_state"),
			&WlrSurface::get_pending_state);
	ClassDB::bind_method(D_METHOD("get_previous_state"),
			&WlrSurface::get_previous_state);
}

WlrSurface::WlrSurface() {
	/* Not used */
}

WlrSurface::WlrSurface(struct wlr_surface *surface) {
	wlr_surface = surface;
	// TODO: Handle surface destroyed
}

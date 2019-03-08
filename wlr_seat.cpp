#include "core/os/input_event.h"
#include "scene/main/node.h"
#include "wayland_display.h"
#include "wlr_seat.h"
#include "wlr_surface.h"
extern "C" {
#include <wayland-server.h>
#include <wlr/types/wlr_data_device.h>
#include <wlr/types/wlr_seat.h>

static inline int64_t timespec_to_msec(const struct timespec *a) {
	return (int64_t)a->tv_sec * 1000 + a->tv_nsec / 1000000;
}
}

void WlrSeat::ensure_wl_global(WaylandDisplay *display) {
	if (wlr_seat) {
		return;
	}
	// TODO: add WlrDataDevice as a user module
	wlr_data_device_manager_create(display->get_wayland_display());
	// TODO: let godot user customize seat names
	wlr_seat = wlr_seat_create(display->get_wayland_display(), "seat0");
	set_capabilities(capabilities);
}

void WlrSeat::destroy_wl_global(WaylandDisplay *display) {
	wlr_seat_destroy(wlr_seat);
	wlr_seat = NULL;
}

WlrSeat::WlrSeat() {
	wlr_seat = NULL;
}

WlrSeat::~WlrSeat() {
	wlr_seat_destroy(wlr_seat);
	wlr_seat = NULL;
}

void WlrSeat::pointer_notify_enter(Variant _surface, double sx, double sy) {
	auto surface = dynamic_cast<WlrSurface *>((Object *)_surface);
	wlr_seat_pointer_notify_enter(wlr_seat,
			surface->get_wlr_surface(), sx, sy);
}

void WlrSeat::pointer_clear_focus() {
	wlr_seat_pointer_clear_focus(wlr_seat);
}

void WlrSeat::pointer_notify_motion(double sx, double sy) {
	struct timespec now;
	clock_gettime(CLOCK_MONOTONIC, &now);
	wlr_seat_pointer_notify_motion(wlr_seat, timespec_to_msec(&now), sx, sy);
}

uint32_t WlrSeat::pointer_notify_button(Variant button, bool pressed) {
	// TODO: translate godot button events into wlroots button events
	return 0;
}

void WlrSeat::pointer_notify_frame() {
	wlr_seat_pointer_notify_frame(wlr_seat);
}

uint32_t WlrSeat::get_capabilities() const {
	return capabilities;
}

void WlrSeat::set_capabilities(uint32_t caps) {
	uint32_t wlr_capabilities = 0;
	if ((caps & SEAT_CAPABILITY_POINTER)) {
		wlr_capabilities |= WL_SEAT_CAPABILITY_POINTER;
	}
	if ((caps & SEAT_CAPABILITY_KEYBOARD)) {
		wlr_capabilities |= WL_SEAT_CAPABILITY_KEYBOARD;
	}
	if ((caps & SEAT_CAPABILITY_TOUCH)) {
		wlr_capabilities |= WL_SEAT_CAPABILITY_TOUCH;
	}
	capabilities = caps;
	if (wlr_seat != NULL) {
		wlr_seat_set_capabilities(wlr_seat, wlr_capabilities);
	}
}

void WlrSeat::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_capabilities", "capabilities"),
			&WlrSeat::set_capabilities);
	ClassDB::bind_method(D_METHOD("get_capabilities"),
			&WlrSeat::get_capabilities);

	ClassDB::bind_method(D_METHOD("pointer_notify_enter", "surface", "sx", "sy"),
			&WlrSeat::pointer_notify_enter);
	ClassDB::bind_method(D_METHOD("pointer_clear_focus"),
			&WlrSeat::pointer_clear_focus);
	ClassDB::bind_method(D_METHOD("pointer_notify_motion", "sx", "sy"),
			&WlrSeat::pointer_notify_motion);
	ClassDB::bind_method(D_METHOD("pointer_notify_button", "button", "pressed"),
			&WlrSeat::pointer_notify_button);
	ClassDB::bind_method(D_METHOD("pointer_notify_frame"),
			&WlrSeat::pointer_notify_frame);

	ADD_PROPERTY(PropertyInfo(Variant::INT, "capabilities", PROPERTY_HINT_FLAGS,
				"Pointer, Keyboard, Touch"),
			"set_capabilities", "get_capabilities");

	BIND_ENUM_CONSTANT(SEAT_CAPABILITY_POINTER);
	BIND_ENUM_CONSTANT(SEAT_CAPABILITY_KEYBOARD);
	BIND_ENUM_CONSTANT(SEAT_CAPABILITY_TOUCH);
}

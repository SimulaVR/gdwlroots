#include <assert.h>
#include "core/os/input_event.h"
#include "scene/main/node.h"
#include "wayland_display.h"
#include "wlr_seat.h"
#include "wlr_surface.h"
#include <iostream>
using namespace std;
extern "C" {
#include <linux/input-event-codes.h>
#include <wayland-server.h>
#include <wlr/types/wlr_data_device.h>
#include <wlr/types/wlr_seat.h>
#include <wlr/types/wlr_xdg_shell.h>

static inline int64_t timespec_to_msec(const struct timespec *a) {
	return (int64_t)a->tv_sec * 1000 + a->tv_nsec / 1000000;
}
}

void WlrSeat::ensure_wl_global(WaylandDisplay *display) {
	if (wlr_seat) {
		return;
	}
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
  //cout << "WlrSeat::pointer_notify_enter" << endl;
	auto surface = dynamic_cast<WlrSurface *>((Object *)_surface);
	wlr_seat_pointer_notify_enter(wlr_seat,
			surface->get_wlr_surface(), sx, sy);
}

void WlrSeat::pointer_clear_focus() {
  //cout << "pointer_clear_focus (wlr_seat):" << wlr_seat << endl;
	wlr_seat_pointer_clear_focus(wlr_seat);
}

void WlrSeat::pointer_notify_motion(double sx, double sy) {
	struct timespec now;
	clock_gettime(CLOCK_MONOTONIC, &now);
	wlr_seat_pointer_notify_motion(wlr_seat, timespec_to_msec(&now), sx, sy);
}

uint32_t WlrSeat::_pointer_notify_button(uint32_t time,
		ButtonList godot_button, bool pressed) {
	uint32_t wlr_button;
	switch (godot_button) {
	case BUTTON_LEFT:
		wlr_button = BTN_LEFT;
		break;
	case BUTTON_RIGHT:
		wlr_button = BTN_RIGHT;
		break;
	case BUTTON_MIDDLE:
		wlr_button = BTN_MIDDLE;
		break;
	default:
		assert(0);
	}
	return wlr_seat_pointer_notify_button(wlr_seat, time, wlr_button,
			pressed ? WLR_BUTTON_PRESSED : WLR_BUTTON_RELEASED);
}

uint32_t WlrSeat::_pointer_notify_axis(uint32_t time, ButtonList godot_button) {
	enum wlr_axis_orientation axis;
	int32_t value_discrete;
	switch (godot_button) {
	case BUTTON_WHEEL_UP:
		axis = WLR_AXIS_ORIENTATION_VERTICAL;
		value_discrete = -1;
		break;
	case BUTTON_WHEEL_DOWN:
		axis = WLR_AXIS_ORIENTATION_VERTICAL;
		value_discrete = 1;
		break;
	case BUTTON_WHEEL_LEFT:
		axis = WLR_AXIS_ORIENTATION_HORIZONTAL;
		value_discrete = 1;
		break;
	case BUTTON_WHEEL_RIGHT:
		axis = WLR_AXIS_ORIENTATION_HORIZONTAL;
		value_discrete = -1;
		break;
	default:
		assert(0);
	}
	wlr_seat_pointer_notify_axis(wlr_seat, time, axis,
			value_discrete * 5, value_discrete, WLR_AXIS_SOURCE_WHEEL);
	return 0;
}

void WlrSeat::pointer_notify_axis_continuous(double x, double y) {
  //cout << "WlrSeat::pointer_notify_axis_continuous: (" << x << "," << y << ")" << endl;
	struct timespec now;
	clock_gettime(CLOCK_MONOTONIC, &now);

	wlr_seat_pointer_notify_axis(wlr_seat,
                               timespec_to_msec(&now),
                               WLR_AXIS_ORIENTATION_VERTICAL,
                               (-(y * 650)),//value
                               0,           //discrete value
                               WLR_AXIS_SOURCE_CONTINUOUS //WLR_AXIS_SOURCE_WHEEL
                               );

  //For now we omit:
  /*
	/ wlr_seat_pointer_notify_axis(wlr_seat,
  /                              timespec_to_msec(&now),
  /                              WLR_AXIS_ORIENTATION_VERTICAL,
  /                              (-(x * 650)),//value
  /                              0,           //discrete value
  /                              WLR_AXIS_SOURCE_CONTINUOUS //WLR_AXIS_SOURCE_WHEEL
  /                              );
  */
  return;
}

uint32_t WlrSeat::pointer_notify_button(Variant button, bool pressed) {
	struct timespec now;
	clock_gettime(CLOCK_MONOTONIC, &now);
	auto godot_button = (ButtonList)(uint32_t)button;
	switch (godot_button) {
	case BUTTON_LEFT:
	case BUTTON_RIGHT:
	case BUTTON_MIDDLE:
		return _pointer_notify_button(timespec_to_msec(&now),
				godot_button, pressed);
	case BUTTON_WHEEL_UP:
	case BUTTON_WHEEL_DOWN:
	case BUTTON_WHEEL_LEFT:
	case BUTTON_WHEEL_RIGHT:
		return _pointer_notify_axis(
				timespec_to_msec(&now), godot_button);
	default:
		return 0;
	}
}

void WlrSeat::pointer_notify_frame() {
	wlr_seat_pointer_notify_frame(wlr_seat);
}

uint32_t WlrSeat::get_capabilities() const {
	return capabilities;
}


struct wlr_seat * WlrSeat::get_wlr_seat() const {
	return wlr_seat;
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

bool WlrSeat::validate_grab_serial(uint32_t serial) {
	return wlr_seat_validate_grab_serial(wlr_seat, serial);
}

void WlrSeat::set_keyboard(Variant _keyboard) {
  //cout << "WlrSeat::set_keyboard" << endl;
	auto keyboard = dynamic_cast<WlrKeyboard *>((Object *)_keyboard);
	wlr_seat_set_keyboard(wlr_seat, keyboard->get_wlr_input_device());
}

void WlrSeat::keyboard_notify_enter(Variant _surface) {
  //cout << "WlrSeat::keyboard_notify_enter" << endl;
	auto surface = dynamic_cast<WlrSurface *>((Object *)_surface);
	struct wlr_keyboard *keyboard = wlr_seat_get_keyboard(wlr_seat);
	wlr_seat_keyboard_notify_enter(wlr_seat, surface->get_wlr_surface(),
		keyboard->keycodes, keyboard->num_keycodes, &keyboard->modifiers);
}

void WlrSeat::keyboard_notify_key(Variant _key_event) {
	struct timespec now;
	clock_gettime(CLOCK_MONOTONIC, &now);
	auto key_event = dynamic_cast<WlrEventKeyboardKey *>((Object *)_key_event);
	auto event = key_event->get_wlr_event();
  //auto event_keycode = event->keycode;
  //auto event_state = event->state;
  //cout << "WlrSeat::keyboard_notify_key(..) (event->keycode, event->state): "
       // << "("
       // << event_keycode
       // << ", "
       // << event_state
       // << ")"
       // << endl;
	wlr_seat_keyboard_notify_key(wlr_seat, timespec_to_msec(&now),
			event->keycode, event->state);
}

void WlrSeat::keyboard_notify_modifiers() {
  //cout << "WlrSeat::keyboard_notify_modifiers" << endl;
	struct timespec now;
	clock_gettime(CLOCK_MONOTONIC, &now);
	struct wlr_keyboard *keyboard = wlr_seat_get_keyboard(wlr_seat);
	wlr_seat_keyboard_notify_modifiers(wlr_seat, &keyboard->modifiers);
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


	ClassDB::bind_method(D_METHOD("pointer_notify_axis_continuous", "sx", "sy"),
                       &WlrSeat::pointer_notify_axis_continuous);

	ClassDB::bind_method(D_METHOD("pointer_notify_frame"),
			&WlrSeat::pointer_notify_frame);

	ClassDB::bind_method(D_METHOD("set_keyboard"), &WlrSeat::set_keyboard);
	ClassDB::bind_method(D_METHOD("keyboard_notify_enter", "surface"),
			&WlrSeat::keyboard_notify_enter);
	ClassDB::bind_method(D_METHOD("keyboard_notify_key", "key_event"),
			&WlrSeat::keyboard_notify_key);
	ClassDB::bind_method(D_METHOD("keyboard_notify_modifiers"),
			&WlrSeat::keyboard_notify_modifiers);

	ClassDB::bind_method(D_METHOD("validate_grab_serial", "serial"),
			&WlrSeat::validate_grab_serial);

	ADD_PROPERTY(PropertyInfo(Variant::INT, "capabilities", PROPERTY_HINT_FLAGS,
				"Pointer, Keyboard, Touch"),
			"set_capabilities", "get_capabilities");

	BIND_ENUM_CONSTANT(SEAT_CAPABILITY_POINTER);
	BIND_ENUM_CONSTANT(SEAT_CAPABILITY_KEYBOARD);
	BIND_ENUM_CONSTANT(SEAT_CAPABILITY_TOUCH);
}

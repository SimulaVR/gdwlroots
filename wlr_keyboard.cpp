#include <time.h>
#include "core/object.h"
#include "core/os/input_event.h"
#include "keycode_map.h"
#include "scene/main/node.h"
#include "wlr_keyboard.h"
extern "C" {
#include <wlr/interfaces/wlr_keyboard.h>
#include <wlr/types/wlr_keyboard.h>

static void keyboard_destroy(struct wlr_keyboard *kb) {
	// This space deliberately left blank
}

static const struct wlr_keyboard_impl keyboard_impl = {
	.destroy = keyboard_destroy,
};

}

void WlrKeyboard::_bind_methods() {
	ClassDB::bind_method(D_METHOD("_input", "event"),
			&WlrKeyboard::_input);
}

void WlrKeyboard::_notification(int p_what) {
	switch (p_what) {
	case NOTIFICATION_ENTER_TREE:
		wlr_keyboard_init(&wlr_keyboard, &keyboard_impl);
		keyboard_init = true;
		set_process_input(true);
		break;
	case NOTIFICATION_EXIT_TREE:
		if (keyboard_init) {
			wlr_keyboard_destroy(&wlr_keyboard);
		}
		set_process_input(false);
		break;
	}
}

static inline int64_t timespec_to_msec(const struct timespec *a) {
	return (int64_t)a->tv_sec * 1000 + a->tv_nsec / 1000000;
}

void WlrKeyboard::_input(const Ref<InputEvent> &p_event) {
	struct timespec now;
	clock_gettime(CLOCK_MONOTONIC, &now);
	Ref<InputEventKey> k = p_event;
	if (k.is_valid()) {
		struct wlr_event_keyboard_key event = { 0 };
		event.time_msec = timespec_to_msec(&now);
		event.keycode = eudev_from_godot(k->get_scancode()) - 8;
		event.state = k->is_pressed() ? WLR_KEY_PRESSED : WLR_KEY_RELEASED;
		event.update_state = true;
		wlr_keyboard_notify_key(&wlr_keyboard, &event);
	}
}

WlrKeyboard::WlrKeyboard() {
	keyboard_init = false;
	wlr_keyboard = { 0 };
}

WlrKeyboard::~WlrKeyboard() {
	if (keyboard_init) {
		wlr_keyboard_destroy(&wlr_keyboard);
	}
}

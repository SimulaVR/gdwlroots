#include <assert.h>
#include <time.h>
#include "core/object/object.h"
#include "core/input/input_event.h"
#include "core/os/keyboard.h"
#include "keycode_map.h"
#include "scene/main/node.h"
#include "wlr_keyboard.h"
#include <iostream>

namespace wlr {
extern "C" {
#include <xkbcommon/xkbcommon.h>
#include <wlr/interfaces/wlr_keyboard.h>
#include <wlr/interfaces/wlr_input_device.h>
#include <wlr/types/wlr_keyboard.h>

static void keyboard_destroy(struct wlr_keyboard *kb) {
  //cout << "keyboard_destroy" << endl;
	// This space deliberately left blank
}

static const struct wlr_keyboard_impl keyboard_impl = {
	.destroy = keyboard_destroy,
};

}
}

using namespace wlr;

void WlrKeyboard::_bind_methods() {
	ClassDB::bind_method(D_METHOD("_input", "event"),
			&WlrKeyboard::_input);

	ClassDB::bind_method(D_METHOD("send_wlr_event_keyboard_key", "scancode_without_modifiers", "is_pressed"),
											 &WlrKeyboard::send_wlr_event_keyboard_key);

	ADD_SIGNAL(MethodInfo("key", PropertyInfo(Variant::OBJECT,
				"keyboard", PROPERTY_HINT_RESOURCE_TYPE, "WlrKeyboard"),
			PropertyInfo(Variant::OBJECT,
				"key_event", PROPERTY_HINT_RESOURCE_TYPE, "WlrEventKeyboardKey")));
	ADD_SIGNAL(MethodInfo("modifiers", PropertyInfo(Variant::OBJECT,
			"keyboard", PROPERTY_HINT_RESOURCE_TYPE, "WlrKeyboard")));
  ADD_SIGNAL(MethodInfo("shortcut", PropertyInfo(Variant::INT,
                                                 "godot_scancode"), PropertyInfo(Variant::BOOL, "is_pressed")));
}

extern "C" {

void WlrKeyboard::handle_key(struct wl_listener *listener, void *data) {
	WlrKeyboard *keyboard = wl_container_of(listener, keyboard, key);
	struct wlr_event_keyboard_key *event =
		(struct wlr_event_keyboard_key *)data;
	auto gdevent = new WlrEventKeyboardKey(event);
	keyboard->emit_signal("key", keyboard, gdevent);
}

void WlrKeyboard::handle_modifiers(struct wl_listener *listener, void *data) {
	WlrKeyboard *keyboard = wl_container_of(listener, keyboard, modifiers);
	keyboard->emit_signal("modifiers", keyboard);
}

}

void WlrKeyboard::ensure_keyboard() {
  //cout << "WlrKeyboard::ensure_keyboard" << endl;
	struct xkb_rule_names rules = { 0 };

	wlr_keyboard_init(&wlr_keyboard, &keyboard_impl);
	// TODO: configure xkb with godot properties?
	rules.rules = getenv("XKB_DEFAULT_RULES");
	rules.model = getenv("XKB_DEFAULT_MODEL");
	rules.layout = getenv("XKB_DEFAULT_LAYOUT");
	rules.variant = getenv("XKB_DEFAULT_VARIANT");
	rules.options = getenv("XKB_DEFAULT_OPTIONS");
	struct xkb_context *context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
	assert(context);
	struct xkb_keymap *keymap = xkb_map_new_from_names(context, &rules,
			XKB_KEYMAP_COMPILE_NO_FLAGS);
	assert(keymap);
	wlr_keyboard_set_keymap(&wlr_keyboard, keymap);
	xkb_keymap_unref(keymap);
	xkb_context_unref(context);

	wlr_input_device_init(&wlr_input_device, WLR_INPUT_DEVICE_KEYBOARD,
			NULL, "Godot", 1, 1);
	wlr_input_device.keyboard = &wlr_keyboard;

	key.notify = handle_key;
	wl_signal_add(&wlr_keyboard.events.key, &key);
	modifiers.notify = handle_modifiers;
	wl_signal_add(&wlr_keyboard.events.modifiers, &modifiers);

	keyboard_init = true;
}

void WlrKeyboard::_notification(int p_what) {
	switch (p_what) {
	case NOTIFICATION_ENTER_TREE:
		ensure_keyboard();
		set_process_input(true);
    //cout << "The keybhoard should be in the tree now." << endl;
		break;
	case NOTIFICATION_EXIT_TREE:
		if (keyboard_init) {
			wlr_keyboard_destroy(&wlr_keyboard);
		}
		set_process_input(false);
    //cout << "The keybhoard should be out of the tree now." << endl;
		break;
	}
}

static inline int64_t timespec_to_msec(const struct timespec *a) {
	return (int64_t)a->tv_sec * 1000 + a->tv_nsec / 1000000;
}

void WlrKeyboard::_input(const Ref<InputEvent> &p_event) {

	Ref<InputEventKey> k = p_event;
	if (k.is_valid()) {
		int scancode_with_modifiers = k->get_keycode_with_modifiers();
		this->emit_signal("shortcut", scancode_with_modifiers, k->is_pressed());
	}
}

void WlrKeyboard::send_wlr_event_keyboard_key(int scancode_without_modifiers, bool is_pressed) {
  struct wlr_event_keyboard_key event = { 0 };
	struct timespec now;
	clock_gettime(CLOCK_MONOTONIC, &now);

  event.time_msec = timespec_to_msec(&now);
  event.keycode = eudev_from_godot(scancode_without_modifiers);
  event.state = is_pressed ? WLR_KEY_PRESSED : WLR_KEY_RELEASED;
  event.update_state = true;
  wlr_keyboard_notify_key(&wlr_keyboard, &event);
}


struct wlr_input_device *WlrKeyboard::get_wlr_input_device() {
  //cout << "*WlrKeyboard::get_wlr_input_device" << endl;
	return &wlr_input_device;
}

WlrKeyboard::WlrKeyboard() {
  //cout << "WlrKeyboard::WlrKeyboard" << endl;
	keyboard_init = false;
	wlr_keyboard = { 0 };
}

WlrKeyboard::~WlrKeyboard() {
  //cout << "WlrKeyboard::~WlrKeyboard" << endl;
	if (keyboard_init) {
		wlr_keyboard_destroy(&wlr_keyboard);
	}
}


WlrEventKeyboardKey::WlrEventKeyboardKey() {
  //cout << "WlrEventKeyboardKey::WlrEventKeyboardKey" << endl;
	/* Not used */
}

WlrEventKeyboardKey::WlrEventKeyboardKey(struct wlr_event_keyboard_key *event) {
  //cout << "WlrEventKeyboardKey::WlrEventKeyboardKey" << endl;
	this->event = event;
}

struct wlr_event_keyboard_key *WlrEventKeyboardKey::get_wlr_event() {
  //cout << "*WlrEventKeyboardKey::get_wlr_event" << endl;
	return event;
}

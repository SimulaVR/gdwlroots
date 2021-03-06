#ifndef GDWLR_WLR_KEYBOARD_H
#define GDWLR_WLR_KEYBOARD_H
#include <stdint.h>
#include "core/input/input_event.h"
#include "scene/main/node.h"
#include "wayland_display.h"

namespace wlr {
extern "C" {
#include <wayland-server.h>
#include <wlr/types/wlr_input_device.h>
#include <wlr/types/wlr_keyboard.h>
}
}

// TODO: gdscript users may want to create synthetic key events with this
class WlrEventKeyboardKey : public Resource {
	GDCLASS(WlrEventKeyboardKey, Resource);

	struct wlr::wlr_event_keyboard_key *event;

protected:
	WlrEventKeyboardKey(); // Required by Object

public:
	WlrEventKeyboardKey(struct wlr::wlr_event_keyboard_key *event);

	struct wlr::wlr_event_keyboard_key *get_wlr_event();
};

class WlrKeyboard : public Node {
	GDCLASS(WlrKeyboard, Node);

	bool keyboard_init;
	struct wlr::wlr_input_device wlr_input_device;
	struct wlr::wlr_keyboard wlr_keyboard;

	struct wlr::wl_listener key;
	struct wlr::wl_listener modifiers;

	static void handle_key(struct wlr::wl_listener *listener, void *data);
	static void handle_modifiers(struct wlr::wl_listener *listener, void *data);

	void ensure_keyboard();

protected:
	static void _bind_methods();
	virtual void _notification(int p_what);
	virtual void _input(const Ref<InputEvent> &p_event);

public:
	WlrKeyboard();
	~WlrKeyboard();

	struct wlr::wlr_input_device *get_wlr_input_device();
	void send_wlr_event_keyboard_key(int scancode_without_modifiers, bool is_pressed);
};

#endif

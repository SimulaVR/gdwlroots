#ifndef GDWLR_WLR_KEYBOARD_H
#define GDWLR_WLR_KEYBOARD_H
#include <stdint.h>
#include "core/os/input_event.h"
#include "scene/main/node.h"
#include "wayland_display.h"
extern "C" {
#include <wayland-server.h>
#include <wlr/types/wlr_input_device.h>
#include <wlr/types/wlr_keyboard.h>
}

// TODO: gdscript users may want to create synthetic key events with this
class WlrEventKeyboardKey : public Object {
	GDCLASS(WlrEventKeyboardKey, Object);

	struct wlr_event_keyboard_key *event;

protected:
	WlrEventKeyboardKey(); // Required by Object

public:
	WlrEventKeyboardKey(struct wlr_event_keyboard_key *event);

	struct wlr_event_keyboard_key *get_wlr_event();
};

class WlrKeyboard : public Node {
	GDCLASS(WlrKeyboard, Node);

	bool keyboard_init;
	struct wlr_input_device wlr_input_device;
	struct wlr_keyboard wlr_keyboard;

	struct wl_listener key;
	struct wl_listener modifiers;

	static void handle_key(struct wl_listener *listener, void *data);
	static void handle_modifiers(struct wl_listener *listener, void *data);

	void ensure_keyboard();

protected:
	static void _bind_methods();
	virtual void _notification(int p_what);
	virtual void _input(const Ref<InputEvent> &p_event);

public:
	WlrKeyboard();
	~WlrKeyboard();

	struct wlr_input_device *get_wlr_input_device();
};

#endif

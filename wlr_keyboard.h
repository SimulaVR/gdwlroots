#ifndef GDWLR_WLR_KEYBOARD_H
#define GDWLR_WLR_KEYBOARD_H
#include <stdint.h>
#include "core/os/input_event.h"
#include "scene/main/node.h"
#include "wayland_display.h"
extern "C" {
#include <wayland-server.h>
#include <wlr/types/wlr_keyboard.h>
}

class WlrKeyboard : public Node {
	GDCLASS(WlrKeyboard, Node);

	bool keyboard_init;
	struct wlr_keyboard wlr_keyboard;

protected:
	static void _bind_methods();
	virtual void _notification(int p_what);
	virtual void _input(const Ref<InputEvent> &p_event);

public:
	WlrKeyboard();
	~WlrKeyboard();
};

#endif

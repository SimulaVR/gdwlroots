#ifndef GDWLR_WLR_SEAT_H
#define GDWLR_WLR_SEAT_H
#include <stdint.h>
#include "core/os/input_event.h"
#include "renderer.h"
#include "scene/main/node.h"
#include "wayland_display.h"
#include "wayland_global.h"
#include "wlr_keyboard.h"
#include "wlr_surface.h"
extern "C" {
#include <wayland-server.h>
#include <wlr/types/wlr_seat.h>
}

class WlrSeat : public WaylandGlobal {
	GDCLASS(WlrSeat, Node);

	uint32_t capabilities = 0;
	struct wlr_seat *wlr_seat;

	void ensure_wl_global(WaylandDisplay *display);
	void destroy_wl_global(WaylandDisplay *display);

	uint32_t _pointer_notify_button(
			uint32_t time, ButtonList godot_button, bool pressed);
	uint32_t _pointer_notify_axis(uint32_t time, ButtonList godot_button);

  void pointer_notify_axis_continuous(double x, double y);

protected:
	static void _bind_methods();

public:
	enum WlrSeatCapability {
		SEAT_CAPABILITY_POINTER = 1,
		SEAT_CAPABILITY_KEYBOARD = 2,
		SEAT_CAPABILITY_TOUCH = 4,
	};

	uint32_t get_capabilities() const;
	void set_capabilities(uint32_t capabilities);

	void pointer_notify_enter(Variant surface, double sx, double sy);
	void pointer_notify_motion(double sx, double sy);
	uint32_t pointer_notify_button(Variant button, bool pressed);
	void pointer_notify_frame();
	void pointer_clear_focus();

	void set_keyboard(Variant _keyboard);
	void keyboard_notify_enter(Variant _surface);
	void keyboard_notify_key(Variant key_event);
	void keyboard_notify_modifiers();

	bool validate_grab_serial(uint32_t serial);

	WlrSeat();
	~WlrSeat();
};

VARIANT_ENUM_CAST(WlrSeat::WlrSeatCapability);

#endif

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
	struct wlr_seat *wlr_seat;

	struct wl_listener request_cursor;
	static void seat_request_cursor(struct wl_listener *listener, void *data);

	void ensure_wl_global(WaylandDisplay *display);
	void destroy_wl_global(WaylandDisplay *display);

	uint32_t _pointer_notify_button(
			uint32_t time, ButtonList godot_button, bool pressed);
	uint32_t _pointer_notify_axis(uint32_t time, ButtonList godot_button);

  void pointer_notify_axis_continuous(double x, double y);

  uint32_t capabilities = 0;

  protected:
  static void _bind_methods();
  void _notification(int p_what);

public:
	enum WlrSeatCapability {
		SEAT_CAPABILITY_POINTER = 1,
		SEAT_CAPABILITY_KEYBOARD = 2,
		SEAT_CAPABILITY_TOUCH = 4,
	};

	uint32_t get_capabilities() const;
	void set_capabilities(uint32_t capabilities);

	void pointer_notify_enter(Ref<WlrSurface> surface, double sx, double sy);
	void pointer_notify_motion(double sx, double sy);
	uint32_t pointer_notify_button(uint32_t button, bool pressed);
	void pointer_notify_frame();
	void pointer_clear_focus();

	void set_keyboard(Object* _keyboard);
	void keyboard_notify_enter(Ref<WlrSurface> _surface);
	void keyboard_notify_key(Ref<WlrEventKeyboardKey> key_event);
	void keyboard_notify_modifiers();

	bool validate_grab_serial(uint32_t serial);
	WlrSurface * get_pointer_focused_surface();

  struct wlr_seat * get_wlr_seat() const;

	WlrSeat();
	~WlrSeat();
};

VARIANT_ENUM_CAST(WlrSeat::WlrSeatCapability);

#endif

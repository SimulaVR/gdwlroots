#include "core/print_string.h"
#include "wayland_display.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

struct wl_display *WaylandDisplay::get_wayland_display() {
	return wl_display;
}

String WaylandDisplay::get_socket_name() const {
	return socket_name;
}

void WaylandDisplay::set_socket_name(const String &name) {
	socket_name = name;
}

void WaylandDisplay::_bind_methods() {
	ClassDB::bind_method(D_METHOD("run"), &WaylandDisplay::run);
	ClassDB::bind_method(D_METHOD("get_socket_name"),
			&WaylandDisplay::get_socket_name);
	ClassDB::bind_method(D_METHOD("set_socket_name", "name"),
			&WaylandDisplay::set_socket_name);

	ADD_PROPERTY(PropertyInfo(Variant::STRING, "socket_name",
				PROPERTY_HINT_PLACEHOLDER_TEXT, "e.g. 'wayland-0'",
				PROPERTY_USAGE_DEFAULT_INTL),
			"set_socket_name", "get_socket_name");
}

void WaylandDisplay::run() {
	if (socket_name.empty()) {
		socket_name = String(wl_display_add_socket_auto(wl_display));
	} else {
		wl_display_add_socket(wl_display, socket_name.utf8().ptr());
	}
	print_line("Running Wayland server on display " + socket_name);
	setenv("WAYLAND_DISPLAY", socket_name.utf8().ptr(), 1);
	set_process(true);
}

void WaylandDisplay::_notification(int p_what) {
	switch (p_what) {
	case NOTIFICATION_PROCESS:
		wl_display_flush_clients(wl_display);
		wl_event_loop_dispatch(wl_event_loop, 0);
		break;
	}
}

WaylandDisplay::WaylandDisplay() {
	wl_display = wl_display_create();
	wl_event_loop = wl_display_get_event_loop(wl_display);
	socket_name = String();
}

WaylandDisplay::~WaylandDisplay() {
	wl_display_destroy(wl_display);
}

#include "core/class_db.h"
#include "register_types.h"
#include "wayland_display.h"
#include "wlr_output.h"

void register_gdwlroots_types() {
	ClassDB::register_class<WaylandDisplay>();
	ClassDB::register_class<WlrOutput>();
}

void unregister_gdwlroots_types() {
	/* this space deliberately left blank */
}

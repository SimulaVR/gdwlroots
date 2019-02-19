#include "core/class_db.h"
#include "register_types.h"
#include "wayland_display.h"

void register_gdwlroots_types() {
	ClassDB::register_class<WaylandDisplay>();
}

void unregister_gdwlroots_types() {
	/* this space deliberately left blank */
}

#include "wayland_global.h"
#include "wayland_display.h"

using namespace wlr;

WaylandDisplay *WaylandGlobal::get_wayland_display() {
	Node *parent = get_parent();
	WaylandDisplay *display = dynamic_cast<WaylandDisplay *>(parent);
	while (parent && !display) {
		parent = parent->get_parent();
		display = dynamic_cast<WaylandDisplay *>(parent);
	}
	return display;
}

void WaylandGlobal::_notification(int p_what) {
	switch (p_what) {
	case NOTIFICATION_ENTER_TREE:
		ensure_wl_global(get_wayland_display());
		break;
	case NOTIFICATION_EXIT_TREE:
		destroy_wl_global(get_wayland_display());
		break;
	}
}

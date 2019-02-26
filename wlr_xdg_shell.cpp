#include "core/object.h"
#include "scene/main/node.h"
#include "wayland_display.h"
#include "wlr_xdg_shell.h"
extern "C" {
#include <wlr/types/wlr_xdg_shell.h>
}

void WlrXdgShell::_bind_methods() {
	/* This space deliberately left blank */
	/* TODO: signals */
}

/* TODO: Consider abstracting this with a generic wlr node base class */
WaylandDisplay *WlrXdgShell::get_wayland_display() {
	Node *parent = get_parent();
	WaylandDisplay *display = dynamic_cast<WaylandDisplay *>(parent);
	while (parent && !display) {
		parent = parent->get_parent();
		display = dynamic_cast<WaylandDisplay *>(parent);
	}
	return display;
}

void WlrXdgShell::ensure_wlr_xdg_shell() {
	if (wlr_xdg_shell) {
		return;
	}
	auto display = get_wayland_display();
	wlr_xdg_shell = wlr_xdg_shell_create(display->get_wayland_display());
}

void WlrXdgShell::_notification(int p_what) {
	switch (p_what) {
	case NOTIFICATION_ENTER_TREE:
		ensure_wlr_xdg_shell();
		break;
	case NOTIFICATION_EXIT_TREE:
		wlr_xdg_shell_destroy(wlr_xdg_shell);
		wlr_xdg_shell = NULL;
		break;
	}
}

WlrXdgShell::WlrXdgShell() {
	wlr_xdg_shell = NULL;
}

WlrXdgShell::~WlrXdgShell() {
	wlr_xdg_shell_destroy(wlr_xdg_shell);
	wlr_xdg_shell = NULL;
}

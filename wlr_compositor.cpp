#include "core/object.h"
#include "scene/main/node.h"
#include "wayland_display.h"
#include "wlr_compositor.h"
#include "wlr_backend.h"
extern "C" {
#include <wlr/types/wlr_compositor.h>
}

void WlrCompositor::_bind_methods() {
	/* This space deliberately left blank */
	/* TODO: new surface signal */
}

WaylandDisplay *WlrCompositor::get_wayland_display() {
	Node *parent = get_parent();
	WaylandDisplay *display = dynamic_cast<WaylandDisplay *>(parent);
	while (parent && !display) {
		parent = parent->get_parent();
		display = dynamic_cast<WaylandDisplay *>(parent);
	}
	return display;
}

WlrBackend *WlrCompositor::get_wlr_backend() {
	Node *parent = get_parent();
	WlrBackend *backend = dynamic_cast<WlrBackend *>(parent);
	while (parent && !backend) {
		parent = parent->get_parent();
		backend = dynamic_cast<WlrBackend *>(parent);
	}
	return backend;
}

void WlrCompositor::ensure_wlr_compositor() {
	if (wlr_compositor) {
		return;
	}
	auto display = get_wayland_display();
	auto backend = get_wlr_backend();
	auto renderer = backend->get_renderer();
	wlr_compositor = wlr_compositor_create(
			display->get_wayland_display(), renderer->get_wlr_renderer());
}

void WlrCompositor::_notification(int p_what) {
	switch (p_what) {
	case NOTIFICATION_ENTER_TREE:
		ensure_wlr_compositor();
		break;
	case NOTIFICATION_EXIT_TREE:
		wlr_compositor_destroy(wlr_compositor);
		wlr_compositor = NULL;
		break;
	}
}

WlrCompositor::WlrCompositor() {
	wlr_compositor = NULL;
}

WlrCompositor::~WlrCompositor() {
	wlr_compositor_destroy(wlr_compositor);
	wlr_compositor = NULL;
}

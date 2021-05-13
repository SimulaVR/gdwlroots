#include "drivers/gles3/rasterizer_gles3.h"
#include "gles3_renderer.h"
#include "servers/visual/visual_server_globals.h"
#include "wayland_display.h"
#include "wlr_backend.h"
#include <assert.h>
#include <stdlib.h>
extern "C" {
#include <wlr/backend.h>
#include <wlr/backend/interface.h>
#include <wlr/render/wlr_renderer.h>
#include <wlr/util/log.h>

bool backend_start(struct wlr_backend *backend) {
	/* This space deliberately left blank */
	return false;
}

void backend_destroy(struct wlr_backend *backend) {
	/* This space deliberately left blank */
}

struct wlr_renderer *backend_get_renderer(struct wlr_backend *_backend) {
	WlrBackend *backend = (WlrBackend *)_backend;
	return backend->get_renderer()->get_wlr_renderer();
}

static const struct wlr_backend_impl backend_impl = {
	.start = backend_start,
	.destroy = backend_destroy,
	.get_renderer = backend_get_renderer,
};

}

void WlrBackend::_bind_methods() {
	/* This space deliberately left blank */
}

struct WlrRenderer *WlrBackend::get_renderer() {
	return renderer;
}

struct wlr_backend *WlrBackend::get_wlr_backend() {
	return (struct wlr_backend *)this;
}

WaylandDisplay *WlrBackend::get_wayland_display() {
	Node *parent = get_parent();
	WaylandDisplay *display = dynamic_cast<WaylandDisplay *>(parent);
	while (parent && !display) {
		parent = parent->get_parent();
		display = dynamic_cast<WaylandDisplay *>(parent);
	}
	return display;
}

void WlrBackend::_notification(int p_what) {
	WaylandDisplay *display = get_wayland_display();
	switch (p_what) {
	case NOTIFICATION_ENTER_TREE:
		if (display != initialized_display) {
			wlr_renderer_init_wl_display(
				get_renderer()->get_wlr_renderer(),
				display->get_wayland_display());
			initialized_display = display;
		}
		break;
	}
}

WlrBackend::WlrBackend() {
	wlr_log_init(WLR_ERROR, NULL);
	auto gles3_rasterizer = dynamic_cast<RasterizerGLES3 *>(VSG::rasterizer);
	if (auto gles3_rasterizer = dynamic_cast<RasterizerGLES3 *>(VSG::rasterizer)) {
		renderer = new WlrGLES3Renderer(gles3_rasterizer);
       	} else {
		print_line("Unsupported rasterizer backend");
		assert(0);
	}
	wlr_backend_init(&backend, &backend_impl);
}

WlrBackend::~WlrBackend() {
	wlr_backend_destroy(&backend);
}

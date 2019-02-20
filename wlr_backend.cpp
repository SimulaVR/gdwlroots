#include "drivers/gles2/rasterizer_gles2.h"
#include "gles2_renderer.h"
#include "servers/visual/visual_server_globals.h"
#include "wlr_backend.h"
#include <assert.h>
#include <stdlib.h>
extern "C" {
#include <wlr/backend.h>
#include <wlr/backend/interface.h>

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

struct wlr_backend_impl backend_impl = {
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

WlrBackend::WlrBackend() {
	auto gles2_rasterizer = dynamic_cast<RasterizerGLES2 *>(VSG::rasterizer);
	if (gles2_rasterizer != NULL) {
		renderer = new WlrGLES2Renderer(gles2_rasterizer);
	} else {
		print_line("Unsupported rasterizer backend");
		assert(0);
	}
	// TODO: Create appropriate renderer for selected rasterizer
	wlr_backend_init(&backend, &backend_impl);
}

WlrBackend::~WlrBackend() {
	wlr_backend_destroy(&backend);
}

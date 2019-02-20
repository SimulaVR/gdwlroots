#include <GLES2/gl2.h>
#include "drivers/gles2/rasterizer_gles2.h"
#include "gles2_renderer.h"
extern "C" {
#define static
#include <wlr/render/wlr_renderer.h>
#include <wlr/render/interface.h>
#undef static
}

struct wlr_renderer *WlrGLES2Renderer::get_wlr_renderer() {
	return renderer;
}

WlrGLES2Renderer::WlrGLES2Renderer(RasterizerGLES2 *p_rasterizer) {
	rasterizer = p_rasterizer;
	renderer = NULL; // TODO
}

WlrGLES2Renderer::~WlrGLES2Renderer() {
	// TODO: Free wlroots stuff
}

#ifndef GDWLR_GLES2_RENDERER_H
#define GDWLR_GLES2_RENDERER_H
#include "drivers/gles2/rasterizer_gles2.h"
#include "renderer.h"
extern "C" {
#define static
#include <wlr/render/wlr_renderer.h>
#undef static
}

class WlrGLES2Renderer : public WlrRenderer {
	struct wlr_renderer wlr_renderer;
	RasterizerGLES2 *rasterizer;
	RasterizerStorageGLES2 *storage;

public:
	virtual struct wlr_renderer *get_wlr_renderer();

	WlrGLES2Renderer(RasterizerGLES2 *rasterizer);
	~WlrGLES2Renderer();
};

#endif

#ifndef GDWLR_RENDERER_H
#define GDWLR_RENDERER_H
#include "scene/resources/texture.h"

namespace wlr {
extern "C" {
#define static
#include <wlr/render/wlr_renderer.h>
#include <wlr/render/wlr_texture.h>
#undef static
}
}

class WlrRenderer {
protected:
	static WlrRenderer *singleton;

public:
	static inline WlrRenderer *get_singleton() {
		return WlrRenderer::singleton;
	}

	virtual struct wlr::wlr_renderer *get_wlr_renderer() = 0;
	virtual Texture *texture_from_wlr(struct wlr::wlr_texture *texture) = 0;
};

#endif

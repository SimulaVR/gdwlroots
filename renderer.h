#ifndef GDWLR_RENDERER_H
#define GDWLR_RENDERER_H
extern "C" {
#define static
#include <wlr/render/wlr_renderer.h>
#undef static
}

class WlrRenderer {
public:
	virtual struct wlr_renderer *get_wlr_renderer() = 0;
};

#endif

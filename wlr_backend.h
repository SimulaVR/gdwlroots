#ifndef GDWLR_WLR_BACKEND_H
#define GDWLR_WLR_BACKEND_H
#include "scene/main/node.h"
#include "renderer.h"
extern "C" {
#include <wlr/backend.h>
}

class WlrBackend : public Node {
	struct wlr_backend backend;

	GDCLASS(WlrBackend, Node);

	WlrRenderer *renderer;

protected:
	static void _bind_methods();

public:
	struct wlr_backend *get_wlr_backend();
	struct WlrRenderer *get_renderer();

	WlrBackend();
	~WlrBackend();
};

#endif

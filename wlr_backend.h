#ifndef GDWLR_WLR_BACKEND_H
#define GDWLR_WLR_BACKEND_H
#include "core/dictionary.h"
#include "scene/main/node.h"
#include "wayland_display.h"
#include "renderer.h"
extern "C" {
#include <wlr/backend.h>
#include <wlr/types/wlr_linux_dmabuf_v1.h>
}

class WlrBackend : public Node {
	struct wlr_backend backend;

	GDCLASS(WlrBackend, Node);

	WlrRenderer *renderer;
	WaylandDisplay *get_wayland_display();
	WaylandDisplay *initialized_display;
	struct wlr_linux_dmabuf_v1 *linux_dmabuf;

protected:
	static void _bind_methods();
	void _notification(int p_what);

public:
	struct wlr_backend *get_wlr_backend();
	struct WlrRenderer *get_renderer();
	Dictionary get_live_object_dictionary();

	WlrBackend();
	~WlrBackend();
};

#endif

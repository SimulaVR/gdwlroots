#ifndef GDWLR_WLR_COMPOSITOR_H
#define GDWLR_WLR_COMPOSITOR_H
#include "renderer.h"
#include "scene/main/node.h"
#include "wayland_display.h"
#include "wlr_backend.h"
extern "C" {
#include <wlr/backend.h>
}

class WlrCompositor : public Node {
	GDCLASS(WlrCompositor, Node);

	struct wlr_compositor *wlr_compositor;

	WlrRenderer *renderer;
	void ensure_wlr_compositor();
	WaylandDisplay *get_wayland_display();
	WlrBackend *get_wlr_backend();

	struct wl_listener new_surface;

	static void handle_new_surface(
		struct wl_listener *listener, void *data);

protected:
	void _notification(int p_what);

	static void _bind_methods();

public:
	WlrCompositor();
	~WlrCompositor();
};

#endif

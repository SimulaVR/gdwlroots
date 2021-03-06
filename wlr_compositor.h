#ifndef GDWLR_WLR_COMPOSITOR_H
#define GDWLR_WLR_COMPOSITOR_H
#include "renderer.h"
#include "scene/main/node.h"
#include "wayland_display.h"
#include "wayland_global.h"
#include "wlr_backend.h"

namespace wlr {
struct wlr_compositor;
}

class WlrCompositor : public WaylandGlobal {
	GDCLASS(WlrCompositor, Node);

	struct wlr::wlr_compositor *wlr_compositor;

	WlrRenderer *renderer;

	WlrBackend *get_wlr_backend();
	void ensure_wl_global(WaylandDisplay *display);
	void destroy_wl_global(WaylandDisplay *display);

	struct wlr::wl_listener new_surface;

	static void handle_new_surface(
		struct wlr::wl_listener *listener, void *data);

protected:
	static void _bind_methods();

public:
	WlrCompositor();
	~WlrCompositor();
  struct wlr::wlr_compositor * get_wlr_compositor();
};

#endif

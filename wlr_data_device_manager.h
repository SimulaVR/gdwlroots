#ifndef GDWLR_WLR_DATA_DEVICE_MANAGER_H
#define GDWLR_WLR_DATA_DEVICE_MANAGER_H
#include <stdint.h>
#include "wayland_display.h"
#include "wayland_global.h"
extern "C" {
#include <wayland-server.h>
#include <wlr/types/wlr_data_device.h>
}

class WlrDataDeviceManager : public WaylandGlobal {
	GDCLASS(WlrDataDeviceManager, Node);

	struct wlr_data_device_manager *wlr_data_device_manager;

	void ensure_wl_global(WaylandDisplay *display);
	void destroy_wl_global(WaylandDisplay *display);

protected:
	static void _bind_methods();

public:
	WlrDataDeviceManager();
	~WlrDataDeviceManager();
};

#endif

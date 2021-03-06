#include "scene/main/node.h"
#include "wayland_display.h"
#include "wlr_data_device_manager.h"

namespace wlr {
extern "C" {
#include <wayland-server.h>
#include <wlr/types/wlr_data_device.h>
}
}

using namespace wlr;

void WlrDataDeviceManager::ensure_wl_global(WaylandDisplay *display) {
	if (wlr_data_device_manager) {
		return;
	}
	wlr_data_device_manager_create(display->get_wayland_display());
}

void WlrDataDeviceManager::destroy_wl_global(WaylandDisplay *display) {
	wlr_data_device_manager_destroy(wlr_data_device_manager);
	wlr_data_device_manager = NULL;
}

WlrDataDeviceManager::WlrDataDeviceManager() {
	wlr_data_device_manager = NULL;
}

WlrDataDeviceManager::~WlrDataDeviceManager() {
	wlr_data_device_manager_destroy(wlr_data_device_manager);
	wlr_data_device_manager = NULL;
}

void WlrDataDeviceManager::_bind_methods() {
	// TODO:
}

#include "core/object.h"
#include "scene/main/viewport.h"
#include "wlr_output.h"

int WlrOutput::get_viewport() {
	return viewport->get_instance_id();
}

void WlrOutput::set_viewport(int vp_id) {
	viewport = static_cast<Viewport*>(ObjectDB::get_instance(vp_id));
}

void WlrOutput::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_viewport"),
			&WlrOutput::get_viewport);
	ClassDB::bind_method(D_METHOD("set_viewport", "viewport"),
			&WlrOutput::set_viewport);

	ADD_PROPERTY(PropertyInfo(Variant::INT, "viewport",
				PROPERTY_HINT_OBJECT_ID, "Viewport"),
			"set_viewport", "get_viewport");
}

WlrOutput::WlrOutput() {
	/* This space deliberately left blank */
}

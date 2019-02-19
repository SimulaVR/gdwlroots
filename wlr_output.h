#ifndef GDWLR_WLR_OUTPUT_H
#define GDWLR_WLR_OUTPUT_H
#include "scene/main/node.h"
#include "scene/main/viewport.h"
#include <wlr/types/wlr_output.h>

class WlrOutput : public Node {
	GDCLASS(WlrOutput, Node);

	Viewport *viewport;

protected:
	static void _bind_methods();

public:
	int get_viewport();
	void set_viewport(int vp_id);

	WlrOutput();
};

#endif

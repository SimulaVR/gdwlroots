extends Node

var Surface = load("res://Surface.tscn")
var XWaylandSurface = load("res://XWaylandSurface.tscn")

func _on_WaylandDisplay_ready():
	var display = get_node("WaylandDisplay")
	display.run()

func handle_map_surface(surface):
	print("Main.gd:handle_map_surface()")
	var vp = get_viewport().size
	surface.position = Vector2(vp.x / 2, -surface.get_size().y)
	surface.focus()
	add_child(surface)

# New
func handle_map_xwayland_surface(xwayland_surface):
	print("Main.gd:handle_map_xwayland_surface()")
	var vp = get_viewport().size
	xwayland_surface.position = Vector2(vp.x / 2, -xwayland_surface.get_size().y)
	xwayland_surface.focus()
	add_child(xwayland_surface)

func handle_unmap_surface(surface):
	remove_child(surface)

# New
func handle_unmap_xwayland_surface(xwayland_surface):
	remove_child(xwayland_surface)

func _on_WlrXdgShell_new_surface(xdg_surface):
	print("Main.gd:_on_WlrXdgShell_new_surface()")
	if xdg_surface.get_role() != WlrXdgSurface.XDG_SURFACE_ROLE_TOPLEVEL:
		return
	var surface = Surface.instance()
	surface.xdg_surface = xdg_surface
	surface.set_seat(get_node("WaylandDisplay/WlrSeat"))
	surface.connect("map", self, "handle_map_surface")
	surface.connect("unmap", self, "handle_unmap_surface")

# New
func _on_WlrXWayland_new_surface(xwayland_surface):
	print("_on_WlrXWayland_new_surface")
	var surface = XWaylandSurface.instance()
	surface.xwayland_surface = xwayland_surface
	surface.set_seat(get_node("WaylandDisplay/WlrSeat"))
	surface.connect("map", self, "handle_map_surface")
	surface.connect("unmap", self, "handle_unmap_surface")

func _on_viewport_change():
	var vp = get_viewport().size
	get_node("ViewportBounds/Bottom").shape.d = -vp.y
	get_node("ViewportBounds/Right").shape.d = -vp.x

func _on_wlr_key(keyboard, event):
	var seat = get_node("WaylandDisplay/WlrSeat")
	seat.keyboard_notify_key(event)

func _on_wlr_modifiers(keyboard):
	var seat = get_node("WaylandDisplay/WlrSeat")
	seat.keyboard_notify_modifiers()

func _ready():
	get_viewport().connect("size_changed", self, "_on_viewport_change")
	var seat = get_node("WaylandDisplay/WlrSeat")
	var keyboard = get_node("WaylandDisplay/WlrKeyboard")
	seat.set_keyboard(keyboard)
	keyboard.connect("key", self, "_on_wlr_key")
	keyboard.connect("modifiers", self, "_on_wlr_modifiers")
	var compositor = get_node("WaylandDisplay/WlrBackend/WlrCompositor")
	print_tree()
	var xwayland = get_node("WaylandDisplay/WlrXWayland")
	xwayland.start_xwayland(compositor, seat)
	xwayland.connect ("new_surface", self, "_on_WlrXWayland_new_surface")
	print("After start_xwayland")

	# [node name="WlrXWayland" type="WlrXWayland" parent="WaylandDisplay"]
	# [connection signal="new_surface" from="WaylandDisplay/WlrXWayland" to="." method="_on_WlrXWayland_new_surface"]
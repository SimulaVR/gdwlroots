extends Node

var Surface = load("res://Surface.tscn")

func _on_WaylandDisplay_ready():
	var display = get_node("WaylandDisplay")
	display.run()

func handle_map_surface(surface):
	var vp = get_viewport().size
	surface.position = Vector2(vp.x / 2, -surface.get_size().y)
	add_child(surface)

func handle_unmap_surface(surface):
	remove_child(surface)

func _on_WlrXdgShell_new_surface(xdg_surface):
	var surface = Surface.instance()
	surface.xdg_surface = xdg_surface
	surface.connect("map", self, "handle_map_surface")
	surface.connect("unmap", self, "handle_unmap_surface")

func _on_viewport_change():
	var vp = get_viewport().size
	get_node("ViewportBounds/Bottom").shape.d = -vp.y
	get_node("ViewportBounds/Right").shape.d = -vp.x

func _ready():
	get_viewport().connect("size_changed", self, "_on_viewport_change")
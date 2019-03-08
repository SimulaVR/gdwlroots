extends RigidBody2D

export var xdg_surface: WlrXdgSurface = null setget xdg_surface_set
var surface: WlrSurface
var geometry: Rect2

signal map(surface)
signal unmap(surface)

func get_size():
	return xdg_surface.get_geometry().size

func handle_destroy(xdg_surface):
	queue_free()
	set_process(false)
	
func handle_map(xdg_surface):
	set_process(true)
	emit_signal("map", self)

func handle_unmap(xdg_surface):
	set_process(false)
	emit_signal("unmap", self)

func xdg_surface_set(val):
	xdg_surface = val
	surface = xdg_surface.get_wlr_surface()
	xdg_surface.connect("destroy", self, "handle_destroy")
	xdg_surface.connect("map", self, "handle_map")
	xdg_surface.connect("unmap", self, "handle_unmap")

func _draw():
	if surface == null:
		return
	var texture = surface.get_texture()
	if texture == null:
		return
	var state = surface.get_current_state()
	draw_texture(texture, Vector2(-state.get_width() / 2, -state.get_height() / 2))
	surface.send_frame_done()

func _process(delta):
	var collisionShape = get_node("CollisionShape2D")
	var state = surface.get_current_state()
	geometry = xdg_surface.get_geometry()
	if geometry.size.x != 0 and geometry.size.y != 0:
		collisionShape.shape.set_extents(geometry.size / Vector2(2, 2))
	update()
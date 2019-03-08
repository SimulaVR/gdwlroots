extends RigidBody2D

export var xdg_surface: WlrXdgSurface = null setget _xdg_surface_set
var toplevel: WlrXdgToplevel
var surface: WlrSurface
var geometry: Rect2
var process_input: bool
var seat: WlrSeat

signal map(surface)
signal unmap(surface)

func get_size():
	return xdg_surface.get_geometry().size

func set_seat(_seat):
	seat = _seat

func focus():
	if toplevel != null:
		process_input = true
		toplevel.set_activated(true)

func _handle_destroy(xdg_surface):
	queue_free()
	set_process(false)
	
func _handle_map(xdg_surface):
	set_process(true)
	emit_signal("map", self)

func _handle_unmap(xdg_surface):
	set_process(false)
	emit_signal("unmap", self)

func _xdg_surface_set(val):
	xdg_surface = val
	surface = xdg_surface.get_wlr_surface()
	xdg_surface.connect("destroy", self, "_handle_destroy")
	xdg_surface.connect("map", self, "_handle_map")
	xdg_surface.connect("unmap", self, "_handle_unmap")
	if xdg_surface.get_role() == WlrXdgSurface.XDG_SURFACE_ROLE_TOPLEVEL:
		toplevel = xdg_surface.get_xdg_toplevel()

func _draw():
	if surface == null:
		return
	var texture = surface.get_texture()
	if texture == null:
		return
	var state = surface.get_current_state()
	# TODO: Draw all subsurfaces/popups/etc
	draw_texture(texture, Vector2(-state.get_width() / 2, -state.get_height() / 2))
	surface.send_frame_done()

func _process(delta):
	var collisionShape = get_node("CollisionShape2D")
	var state = surface.get_current_state()
	geometry = xdg_surface.get_geometry()
	if geometry.size.x != 0 and geometry.size.y != 0:
		collisionShape.shape.set_extents(geometry.size / Vector2(2, 2))
	update()

func get_surface_coords(position):
	return position + geometry.size / Vector2(2, 2) + geometry.position

func _on_RigidBody2D_input_event(viewport, event, shape_idx):
	if event is InputEventMouseMotion:
		var position = get_surface_coords(to_local(event.position))
		seat.pointer_notify_motion(position.x, position.y)
		seat.pointer_notify_frame()

func _on_RigidBody2D_mouse_entered():
	var position = get_surface_coords(to_local(get_viewport().get_mouse_position()))
	seat.pointer_notify_enter(surface, position.x, position.y)
extends RigidBody2D

export var xdg_surface: WlrXdgSurface = null setget _xdg_surface_set
var toplevel: WlrXdgToplevel
var geometry: Rect2
var seat: WlrSeat

enum {
	INTERACTIVE_PASSTHROUGH,
	INTERACTIVE_MOVE,
	INTERACTIVE_RESIZE,
}

var input_mode = INTERACTIVE_PASSTHROUGH
var interactive_offset: Vector2

signal map(surface)
signal unmap(surface)

func get_size():
	return xdg_surface.get_geometry().size

func set_seat(_seat):
	seat = _seat

func focus():
	if toplevel != null:
		toplevel.set_activated(true)

func _handle_destroy(xdg_surface):
	queue_free()
	set_process(false)
	
func _handle_map(xdg_surface):
	set_process(true)
	set_process_input(true)
	emit_signal("map", self)

func _handle_unmap(xdg_surface):
	set_process(false)
	set_process_input(false)
	emit_signal("unmap", self)

func _handle_request_move(xdg_toplevel, serial):
	if not seat.validate_grab_serial(serial):
		return
	var position = to_local(get_viewport().get_mouse_position())
	interactive_offset = position
	input_mode = INTERACTIVE_MOVE

func _xdg_surface_set(val):
	xdg_surface = val
	xdg_surface.connect("destroy", self, "_handle_destroy")
	xdg_surface.connect("map", self, "_handle_map")
	xdg_surface.connect("unmap", self, "_handle_unmap")
	toplevel = xdg_surface.get_xdg_toplevel()
	toplevel.connect("request_move", self, "_handle_request_move")

func _draw_surface(surface, sx, sy):
	var texture = surface.get_texture()
	if texture == null:
		return
	var state = xdg_surface.get_wlr_surface().get_current_state()
	var position = Vector2(
		(-state.get_buffer_width() / 2) + sx,
		(-state.get_buffer_height() / 2) + sy)
	draw_texture(texture, position)
	surface.send_frame_done()

func _draw():
	if xdg_surface != null:
		var fn = funcref(self, "_draw_surface")
		xdg_surface.for_each_surface(fn)

func _process(delta):
	var collisionShape = get_node("CollisionShape2D")
	var surface = xdg_surface.get_wlr_surface()
	if surface == null:
		update()
		return
	var state = surface.get_current_state()
	geometry = xdg_surface.get_geometry()
	var extents = collisionShape.shape.get_extents()
	var desiredExtents = geometry.size / Vector2(2, 2)
	if geometry.size.x == 0 or geometry.size.y == 0:
		desiredExtents = Vector2(state.get_width() / 2, state.get_height() / 2)
	if desiredExtents.x != 0 and desiredExtents.y != 0 \
			and extents != desiredExtents:
		collisionShape.shape = RectangleShape2D.new()
		collisionShape.shape.set_extents(desiredExtents)
		print("Set surface extents to ", desiredExtents)
	update()

func get_surface_coords(position):
	return position + geometry.size / Vector2(2, 2) + geometry.position

func input_event_passthrough(event):
	var notify_frame = false
	if event is InputEventMouseMotion:
		var position = get_surface_coords(to_local(event.position))
		seat.pointer_notify_motion(position.x, position.y)
		notify_frame = true
	if event is InputEventMouseButton:
		seat.pointer_notify_button(event.button_index, event.pressed)
		notify_frame = true
	if notify_frame:
		seat.pointer_notify_frame()

func _on_RigidBody2D_input_event(viewport, event, shape_idx):
	match input_mode:
		INTERACTIVE_PASSTHROUGH:
			input_event_passthrough(event)

func _input(event):
	if input_mode == INTERACTIVE_MOVE or input_mode == INTERACTIVE_RESIZE:
		if event is InputEventMouseButton and not event.pressed:
			input_mode = INTERACTIVE_PASSTHROUGH

func _integrate_forces(state):
	var lv = state.get_linear_velocity()
	if input_mode == INTERACTIVE_MOVE:
		lv = (get_viewport().get_mouse_position() -
			to_global(interactive_offset)) * 16
	state.set_linear_velocity(lv)

func _on_RigidBody2D_mouse_entered():
	var position = get_surface_coords(to_local(get_viewport().get_mouse_position()))
	# TODO: Find subsurface
	var surface = xdg_surface.get_wlr_surface()
	if surface != null:
		seat.pointer_notify_enter(surface, position.x, position.y)

extends RigidBody2D

export var xwayland_surface: WlrXWaylandSurface = null setget _xwayland_surface_set
# var toplevel: WlrXdgToplevel
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
	return xwayland_surface.get_geometry().size

func set_seat(_seat):
	seat = _seat

func focus():
	xwayland_surface.set_activated(true)
	if seat != null:
		seat.keyboard_notify_enter(xwayland_surface.get_wlr_surface())

func _handle_destroy(xwayland_surface):
	queue_free()
	set_process(false)
	
func _handle_map(xwayland_surface):
	set_process(true)
	set_process_input(true)
	emit_signal("map", self)

func _handle_unmap(xwayland_surface):
	set_process(false)
	set_process_input(false)
	emit_signal("unmap", self)

func _handle_request_move(xwayland_surface):
	var position = to_local(get_viewport().get_mouse_position())
	interactive_offset = position
	input_mode = INTERACTIVE_MOVE

func _xwayland_surface_set(val):
	xwayland_surface = val
	xwayland_surface.connect("destroy", self, "_handle_destroy")
	xwayland_surface.connect("map", self, "_handle_map")
	xwayland_surface.connect("unmap", self, "_handle_unmap")
	# xwayland_surface = xwayland_surface.get_xwayland_surface()
	xwayland_surface.connect("request_move", self, "_handle_request_move")

func _draw():
  if xwayland_surface != null:
	  var texture = xwayland_surface.get_texture()
	  if texture == null:
		  return
	  var position = Vector2(0,0)
	  # var position = Vector2(
	  #   (-xwayland_surface.get_buffer_width() / 2) + sx,
	  #   (-xwayland_surface.get_buffer_height() / 2) + sy)
	  draw_texture(texture, position)
	  xwayland_surface.send_frame_done()

func _process(delta):
	var collisionShape = get_node("CollisionShape2D")
	var surface = xwayland_surface.get_wlr_surface()
	if surface == null:
		update()
		return
	#var state = surface.get_current_state()
	geometry = xwayland_surface.get_geometry()
	var extents = collisionShape.shape.get_extents()
	var desiredExtents = geometry.size / Vector2(2, 2)
	if geometry.size.x == 0 or geometry.size.y == 0:
		desiredExtents = Vector2(xwayland_surface.get_width() / 2, xwayland_surface.get_height() / 2)
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
		var surface = xwayland_surface.surface_at(position.x, position.y)
		if surface != null:
			seat.pointer_notify_enter(surface.get_surface(),
					surface.get_sub_x(), surface.get_sub_y())
			seat.pointer_notify_motion(surface.get_sub_x(), surface.get_sub_y())
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
	var position = get_surface_coords(
			to_local(get_viewport().get_mouse_position()))
	var surface = xwayland_surface.surface_at(position.x, position.y)
	if surface != null:
		seat.pointer_notify_enter(surface.get_surface(),
				surface.get_sub_x(), surface.get_sub_y())

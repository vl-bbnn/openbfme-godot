## Headless regression for Godot arrival times across the HUD's deferred input.
## Run: godot --headless --path godot --script res://tests/input_queue_timing_test.gd
## Requires ROTWK_INSTALL / BFME2_INSTALL; exit 77 when absent.
##
## The existing hold_input QA hook forces the same queue used by a busy logic
## worker. Gestures enter through Viewport.push_input; no desktop input is used.
## Setup uses explicit selection/build-button hooks. This is a queue behavior
## test, not an end-to-end menu, rendering, retail parity or FPS test.
## MouseSettings::isRightClick / PlaceEventTranslator RW 0x83E66C preserve a
## dragged placement when the arrival interval reaches Mouse.ini's 150 ms.
extends SceneTree

var _world: Node3D
var _hud: Node
var _porter: int
var _failures := 0
var _cases := 0

func _initialize() -> void:
	call_deferred("_run")

func _check(ok: bool, message: String) -> void:
	print("INPUT QUEUE ", "ok " if ok else "FAIL ", message)
	if not ok:
		_failures += 1

func _frames() -> void:
	for i in 4:
		await process_frame

func _move(position: Vector2, right_held: bool) -> void:
	var event := InputEventMouseMotion.new()
	event.position = position
	event.global_position = position
	event.button_mask = MOUSE_BUTTON_MASK_RIGHT if right_held else 0
	root.push_input(event)

func _right(pressed: bool, position: Vector2) -> void:
	var event := InputEventMouseButton.new()
	event.position = position
	event.global_position = position
	event.button_index = MOUSE_BUTTON_RIGHT
	event.button_mask = MOUSE_BUTTON_MASK_RIGHT if pressed else 0
	event.pressed = pressed
	root.push_input(event)

func _case(scheme: String, mode: String, long_drag: bool) -> void:
	var tag := "%s %s %s" % [scheme, mode, "long" if long_drag else "quick"]
	if _hud.get_placement().placing:
		_hud.inject_key(1, true) # DIK_ESCAPE: reset the previous case's placement.
		_hud.inject_key(1, false)
	_check(_hud.select_object(_porter), tag + ": select test porter")
	await _frames()
	_check(_hud.press_command_button("GondorFarm"), tag + ": arm placement")
	await _frames()
	var start := Vector2(500, 250)
	var finish := Vector2(750, 250)
	_move(start, false)
	await _frames()
	_check(_hud.get_placement().placing and not _hud.get_state().get("over_gui", true), tag + ": armed outside GUI")
	_check(_hud.get_state().keys.pending == 0, tag + ": empty queue before gesture")
	var frame: int = _world.get_frame()
	var state_hash: int = _world.get_state_hash()
	_hud.hold_input(mode == "full" or mode == "press")
	var down_time := Time.get_ticks_msec()
	_right(true, start)
	if mode == "release":
		_hud.hold_input(true)
	_move(finish, true)
	if long_drag:
		await create_timer(0.45).timeout
	var pending := 0
	if mode == "press":
		pending = _hud.get_state().keys.pending
		_hud.hold_input(false)
		await _frames()
	var up_time := Time.get_ticks_msec()
	_right(false, finish)
	if mode != "press":
		pending = _hud.get_state().keys.pending
	_hud.hold_input(false)
	await _frames()
	var expected_pending: int = {"immediate": 0, "full": 3, "press": 2, "release": 2}[mode]
	_check(pending == expected_pending, tag + ": exact queued event count")
	_check(_hud.get_state().keys.pending == 0, tag + ": queue drained")
	# tolerance: retail Mouse.ini DragToleranceMS is 150; real receipt intervals
	# must fall on the intended side, rather than assuming a timer ran on time.
	_check((up_time - down_time >= 150) == long_drag, tag + ": actual interval %d ms qualifies" % (up_time - down_time))
	_check(_hud.get_placement().placing == long_drag, tag + ": long drag retains placement, quick gesture cancels")
	_check(_world.get_frame() == frame and _world.get_state_hash() == state_hash, tag + ": input never advances simulation")
	_cases += 1

func _run() -> void:
	if OS.get_environment("ROTWK_INSTALL").is_empty() or OS.get_environment("BFME2_INSTALL").is_empty():
		print("INPUT QUEUE TIMING SKIP: set ROTWK_INSTALL / BFME2_INSTALL")
		quit(77)
		return
	root.size = Vector2i(1280, 720)
	var fs = ClassDB.instantiate("RetailFileSystem")
	var mounted: Dictionary = fs.mount_retail()
	if not mounted.ok:
		print("INPUT QUEUE TIMING FAIL mount: ", mounted)
		quit(1)
		return
	_world = ClassDB.instantiate("GameWorld")
	root.add_child(_world)
	var setup: Dictionary = _world.setup(fs)
	if not setup.ok:
		print("INPUT QUEUE TIMING FAIL setup: ", setup)
		quit(1)
		return
	var loaded: Dictionary = _world.load_map("map mp fall back 4p", {"slots": [{"player": "Player_1", "faction": "FactionMen", "human": true}], "seed": 4711})
	if not loaded.ok:
		print("INPUT QUEUE TIMING FAIL load: ", loaded)
		quit(1)
		return
	_world.set_logic_thread(false)
	var owner := -1
	for i in loaded.players.size():
		if String(loaded.players[i]).contains("'Player_1'"):
			owner = i
	if owner == -1:
		print("INPUT QUEUE TIMING FAIL: player not found")
		quit(1)
		return
	_porter = _world.create_object("MenPorter", owner, 1700, 1700, 0)
	if _porter == 0:
		print("INPUT QUEUE TIMING FAIL: porter not created")
		quit(1)
		return
	var camera := Camera3D.new()
	root.add_child(camera)
	for alternate in [false, true]:
		var layer := CanvasLayer.new()
		root.add_child(layer)
		_hud = ClassDB.instantiate("InGameHudNode")
		layer.add_child(_hud)
		await process_frame
		var hud_setup: Dictionary = _hud.setup(fs, _world, camera, "Player_1", {"camera_start": Vector3(1975, 0, -1800), "edge_scroll": false, "alternate_mouse": alternate})
		if not hud_setup.ok:
			print("INPUT QUEUE TIMING FAIL HUD: ", hud_setup)
			quit(1)
			return
		for mode in ["immediate", "full", "press", "release"]:
			for long_drag in [false, true]:
				await _case("alternate" if alternate else "classic", mode, long_drag)
		layer.queue_free()
		await _frames()
	_check(_cases == 16, "all sixteen gestures ran")
	_world.queue_free()
	camera.queue_free()
	await _frames()
	print("INPUT QUEUE TIMING %s: %d cases, %d failures" % ["PASS" if _failures == 0 else "FAIL", _cases, _failures])
	quit(0 if _failures == 0 else 1)

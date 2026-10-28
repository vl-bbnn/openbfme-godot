## GameWorld must join its logic worker before Node PREDELETE deletes the FX playback child.
## Exercises attached and detached ownership, each with queued natural logic work.
## Requires mounted retail data; no rule/actor/outcome hooks. The child probe uses only public worker-state accessors.
extends Node
var failures: int=0
func _ready() -> void:
	call_deferred("_run")
func _run() -> void:
	var folder: String=OS.get_environment("OPENBFME_TEARDOWN_DIR")
	assert(not folder.is_empty())
	var fs: RefCounted=ClassDB.instantiate("RetailFileSystem")
	assert(fs.mount_retail().ok)
	for attached in [true,false]:
		var world: Node3D=ClassDB.instantiate("GameWorld")
		if attached:get_tree().root.add_child(world)
		assert(world.setup(fs).ok)
		var rep: Dictionary=world.load_map("map mp brown lands",{"slots":[{"player":"Player_1","faction":"FactionMen","human":true,"team":0,"start_index":0},{"player":"Player_2","faction":"FactionMordor","human":false,"team":1,"start_index":1}],"seed":4711,"logic_thread":true})
		assert(rep.ok)
		world.set_auto_advance(false)
		assert(world.get_logic_thread())
		var probe: Node=load("res://tests/teardown_probe.gd").new()
		probe.world=world
		probe.case_name="attached" if attached else "detached"
		probe.folder=folder
		world.add_child(probe)
		world.advance(2.0)
		# No hold/wait accessor here: deliberately tear down immediately after queueing.
		world.free()
		var row: Dictionary=JSON.parse_string(FileAccess.get_file_as_string(folder.path_join(("attached" if attached else "detached")+"-child.json")))
		print("WORKER CHILD ",JSON.stringify(row))
		if row.logic_thread or not row.logic_held:failures+=1
	print("WORKER TEARDOWN ","PASS" if failures==0 else "FAIL", " failures=",failures)
	get_tree().quit(0 if failures==0 else 1)

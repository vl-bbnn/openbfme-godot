## S-5161: pin the missing authored W3D normal-map stop, not a guessed replacement.
## Requires pure RotWK 2.01 / BFME2 1.06. The two ERROR lines from D1 are expected.
## Passing this test proves the diagnostic contract; it does not accept rendering.
extends SceneTree

func _initialize() -> void:
	call_deferred("_run")

func _run() -> void:
	if OS.get_environment("ROTWK_INSTALL").is_empty() or OS.get_environment("BFME2_INSTALL").is_empty():
		quit(77)
		return
	var fs = ClassDB.instantiate("RetailFileSystem")
	var mounted: Dictionary = fs.mount_retail()
	assert(mounted.ok, JSON.stringify(mounted))
	var surface_errors := PackedStringArray([
		"NBELVNBARX_D1.PLANE01: texture not found: NBElvnBarx_D_NRM.tga",
		"NBELVNBARX_D1.NBELVNBARX: texture not found: NBElvnBarx_D_NRM.tga"])
	var all_errors := surface_errors.duplicate()
	all_errors.append("texture not found: NBElvnBarx_D_NRM.tga")
	all_errors.append("texture not found: NBElvnBarx_D_NRM.tga")
	var rows: Array = []
	for name in ["NBElvnBarx_SKN", "NBElvnBarx_D1L", "NBElvnBarx_D1"]:
		var renderer: Node3D = ClassDB.instantiate("W3DInstancer")
		root.add_child(renderer)
		var setup: Dictionary = renderer.setup(fs)
		assert(setup.ok, JSON.stringify(setup))
		# Each fresh instancer's first allocated model has handle zero, even on this reported stop.
		var handle: int = renderer.add_model(name)
		assert(handle == 0, str(renderer.get_errors()))
		var report: Dictionary = renderer.get_model_report(handle)
		var expected := surface_errors if name == "NBElvnBarx_D1" else PackedStringArray()
		assert(report.errors == expected, JSON.stringify(report))
		assert(report.ok == (name != "NBElvnBarx_D1"), JSON.stringify(report))
		assert(renderer.get_errors() == (all_errors if name == "NBElvnBarx_D1" else PackedStringArray()), str(renderer.get_errors()))
		rows.append({"name": name, "report": report, "errors": renderer.get_errors(), "warnings": renderer.get_warnings()})
		renderer.free()
	var release = ClassDB.instantiate("ReleaseInfo")
	var result: Dictionary = {"diagnostic_contract_pass": true, "graphics_accepted": false, "stop": "S-5161", "pid": OS.get_process_id(), "record": release.build_record(), "mount": mounted, "models": rows}
	var output: String = OS.get_environment("OPENBFME_MISSING_NORMAL_RESULT")
	if not output.is_empty():
		var file := FileAccess.open(output, FileAccess.WRITE)
		assert(file != null)
		file.store_string(JSON.stringify(result))
		file.close()
	print("W3D MISSING NORMAL STOP PASS; GRAPHICS ACCEPTANCE REMAINS BLOCKED S-5161")
	quit(0)

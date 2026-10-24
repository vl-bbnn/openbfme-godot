## Movie audio ownership regression. Needs pure ROTWK_INSTALL/BFME2_INSTALL.
## The first allocated handle aliases an unresolved legacy music command (S-240).
## Exercise actual movie playback/cleanup with real EALogo data, not a mock handle.
extends SceneTree

var failures := 0

func _check(ok: bool, message: String) -> void:
	print("MOVIE AUDIO %s %s" % ["ok" if ok else "FAIL", message])
	if not ok:
		failures += 1

func _initialize() -> void:
	if OS.get_environment("ROTWK_INSTALL").is_empty() or OS.get_environment("BFME2_INSTALL").is_empty():
		print("MOVIE AUDIO SKIP: original game installs are required")
		quit(77)
		return
	var fs: RefCounted = ClassDB.instantiate("RetailFileSystem")
	var mount: Dictionary = fs.mount_retail()
	_check(mount.get("ok", false), "pure retail mount: " + JSON.stringify(mount))
	if failures:
		quit(1)
		return
	var world: Node = ClassDB.instantiate("GameWorld")
	root.add_child(world)
	var setup: Dictionary = world.setup(fs)
	_check(setup.get("ok", false), "world video registry loaded")
	if failures:
		quit(1)
		return
	for mode in ["pending-first", "playing-first", "playing-later", "unrelated-voice"]:
		var audio: Node = ClassDB.instantiate("GameAudio")
		root.add_child(audio)
		var booted: Dictionary = audio.boot(fs, {"seed":1})
		_check(booted.get("ok", false), mode + ": audio boot")
		if not booted.get("ok", false):
			break
		if mode == "playing-later":
			var warm_handle: int = audio.play_sound("EALogo")
			audio.kill_event(warm_handle)
		var unrelated: int = 0
		if mode == "unrelated-voice":
			unrelated = audio.play_sound("EALogo")
			await create_timer(0.5).timeout
		var player: Node = load("res://scripts/movie_player.gd").new()
		root.add_child(player)
		player.skip_after_ms = 0 if mode == "pending-first" else 1000
		var played: bool = await player.play(world, audio, "EALogoMovie")
		_check(played and player.errors.is_empty(), mode + ": movie opened without errors")
		_check(player.last.get("audio_started", 0) == 1, mode + ": one movie audio request")
		var stopped: Dictionary = audio.get_stats()
		var playing: PackedStringArray = audio.get_playing()
		_check(stopped.get("pending_requests", -1) == 0, mode + ": no pending movie request after stop")
		if unrelated == 0:
			_check(not playing.has("EALogo"), mode + ": movie voice absent immediately after stop")
		else:
			_check(audio.is_playing(unrelated) and stopped.get("playing_streams", -1) == 1, mode + ": exactly the unrelated voice survives")
		await create_timer(0.5).timeout
		var delayed: PackedStringArray = audio.get_playing()
		if unrelated == 0:
			_check(not delayed.has("EALogo"), mode + ": no revived movie voice after stop")
		else:
			_check(audio.is_playing(unrelated) and audio.get_stats().get("playing_streams", -1) == 1, mode + ": unrelated voice remains after a frame")
		print("MOVIE AUDIO RECORD ", JSON.stringify({"mode":mode,"movie":player.last,"stopped":stopped,"playing":playing,"delayed":delayed}))
		player.queue_free()
		audio.shutdown()
		audio.queue_free()
		await process_frame
	world.queue_free()
	await process_frame
	print("MOVIE AUDIO RESULT: %d failures" % failures)
	quit(0 if failures == 0 else 1)

extends Node
var world: Node
var case_name: String
var folder: String
func _notification(what: int) -> void:
	if what==NOTIFICATION_PREDELETE:
		var f:=FileAccess.open(folder.path_join(case_name+"-child.json"),FileAccess.WRITE)
		if f!=null:
			f.store_string(JSON.stringify({"case":case_name,"world_valid":is_instance_valid(world),"logic_thread":world.get_logic_thread(),"logic_held":world.is_logic_held()}))
			f.close()

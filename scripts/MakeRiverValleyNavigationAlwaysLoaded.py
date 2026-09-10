import unreal


MAP_PATH = "/Game/CityStateRTS/Maps/LVL_RiverValleySkirmish"

level_subsystem = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

level_subsystem.load_level(MAP_PATH)
chunk_count = 0
for actor in actor_subsystem.get_all_level_actors():
    if actor.get_class().get_name() == "NavigationDataChunkActor":
        actor.set_editor_property("is_spatially_loaded", False)
        actor.modify()
        chunk_count += 1

if chunk_count != 4:
    raise RuntimeError(f"Expected 4 navigation chunks, found {chunk_count}")

if not level_subsystem.save_current_level():
    raise RuntimeError("Failed to save the river valley map")

unreal.log(f"NAV_CHUNKS_ALWAYS_LOADED count={chunk_count}")

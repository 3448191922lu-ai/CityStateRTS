import unreal


MAP_PATH = "/Game/CityStateRTS/Maps/LVL_RiverValleySkirmish"

level_subsystem = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

level_subsystem.load_level(MAP_PATH)
recast_nav_mesh = next(
    actor
    for actor in actor_subsystem.get_all_level_actors()
    if actor.get_class().get_name() == "RecastNavMesh"
)
recast_nav_mesh.set_editor_property("is_world_partitioned", True)
recast_nav_mesh.modify()

if not level_subsystem.save_current_level():
    raise RuntimeError("无法保存河谷遭遇战关卡")

unreal.log("RIVER_RECAST_WORLD_PARTITIONED enabled=true")

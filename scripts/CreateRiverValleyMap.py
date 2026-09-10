import unreal


source_path = "/Game/CityStateRTS/Maps/LVL_CityStateSkirmish"
target_path = "/Game/CityStateRTS/Maps/LVL_RiverValleySkirmish"

if unreal.EditorAssetLibrary.does_asset_exist(target_path):
    unreal.log("LVL_RiverValleySkirmish 已存在，无需重复创建。")
else:
    source_world = unreal.EditorAssetLibrary.load_asset(source_path)
    target_world = unreal.AssetToolsHelpers.get_asset_tools().duplicate_asset(
        "LVL_RiverValleySkirmish",
        "/Game/CityStateRTS/Maps",
        source_world,
    )
    if target_world is None:
        raise RuntimeError("无法复制河谷遭遇战关卡。")
    unreal.EditorAssetLibrary.save_loaded_asset(target_world, only_if_is_dirty=False)
    unreal.log("已创建并保存 LVL_RiverValleySkirmish。")

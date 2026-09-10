import json
from pathlib import Path

import unreal


PROJECT_ROOT = Path(r"D:\ue project\RTS")
MANIFEST_PATH = PROJECT_ROOT / "Art" / "M4ImportManifest.json"


def load_manifest():
    return json.loads(MANIFEST_PATH.read_text(encoding="utf-8"))


def create_import_task(source_root, entry):
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", str(source_root / Path(entry["sourceFile"])))
    task.set_editor_property("destination_path", entry["destination"])
    task.set_editor_property("destination_name", entry["assetName"])
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("save", True)

    if entry["assetType"] == "skeletalMesh":
        options = unreal.FbxImportUI()
        options.set_editor_property("import_mesh", True)
        options.set_editor_property("import_animations", True)
        options.set_editor_property("import_as_skeletal", True)
        options.set_editor_property("mesh_type_to_import", unreal.FBXImportType.FBXIT_SKELETAL_MESH)
        options.skeletal_mesh_import_data.set_editor_property("convert_scene", True)
        options.anim_sequence_import_data.set_editor_property("import_custom_attribute", False)
        task.set_editor_property("options", options)
    elif entry["assetType"] == "staticMesh":
        options = unreal.FbxImportUI()
        options.set_editor_property("import_mesh", True)
        options.set_editor_property("import_animations", False)
        options.set_editor_property("import_as_skeletal", False)
        options.set_editor_property("mesh_type_to_import", unreal.FBXImportType.FBXIT_STATIC_MESH)
        options.static_mesh_import_data.set_editor_property("combine_meshes", True)
        options.static_mesh_import_data.set_editor_property("generate_lightmap_u_vs", True)
        task.set_editor_property("options", options)
    return task


def normalized(value):
    return "".join(character.lower() for character in value if character.isalnum())


def find_animation(folder, source_animation, source_prefix):
    wanted = normalized(source_animation)
    prefix = normalized(source_prefix)
    candidates = []
    for asset_path in unreal.EditorAssetLibrary.list_assets(folder, recursive=False, include_folder=False):
        asset = unreal.EditorAssetLibrary.load_asset(asset_path)
        asset_name = normalized(asset.get_name())
        if isinstance(asset, unreal.AnimSequence) and prefix in asset_name and wanted in asset_name:
            candidates.append(asset_path)
    if not candidates:
        raise RuntimeError(f"未找到动画片段：{source_animation} ({folder})")
    return min(candidates, key=lambda path: (not normalized(path.rsplit("/", 1)[-1]).endswith(wanted), len(path)))


def main():
    # 传统 FBX 导入器会稳定导入 Quaternius 文件中的多个动画 Take。
    unreal.SystemLibrary.execute_console_command(None, "Interchange.FeatureFlags.Import.FBX 0")
    manifest = load_manifest()
    source_root = Path(manifest["sourceRoot"])
    # 切换导入器后必须全新创建骨骼，避免与旧导入器生成的骨骼发生合并冲突。
    unreal.EditorAssetLibrary.delete_directory("/Game/CityStateRTS/Art/Characters")
    import_entries = [entry for entry in manifest["assets"] if entry["assetType"] != "animation"]
    tasks = [create_import_task(source_root, entry) for entry in import_entries]
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)

    mesh_entries = {entry["role"]: entry for entry in manifest["assets"] if entry["assetType"] == "skeletalMesh"}
    for entry in [item for item in manifest["assets"] if item["assetType"] == "animation"]:
        source_folder = mesh_entries[entry["skeletonRole"]]["destination"]
        source_prefix = mesh_entries[entry["skeletonRole"]]["assetName"]
        source_path = find_animation(source_folder, entry["sourceAnimation"], source_prefix)
        target_path = f'{entry["destination"]}/{entry["assetName"]}'
        if source_path != target_path:
            if unreal.EditorAssetLibrary.does_asset_exist(target_path):
                unreal.EditorAssetLibrary.delete_asset(target_path)
            if not unreal.EditorAssetLibrary.rename_asset(source_path, target_path):
                raise RuntimeError(f"动画重命名失败：{source_path} -> {target_path}")

    unreal.EditorAssetLibrary.save_directory("/Game/CityStateRTS/Art", only_if_is_dirty=False, recursive=True)
    unreal.log(f"M4 导入完成：{len(import_entries)} 个源文件，16 个动画角色。")


main()

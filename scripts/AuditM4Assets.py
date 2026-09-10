import json
from pathlib import Path

import unreal


PROJECT_ROOT = Path(r"D:\ue project\RTS")
MANIFEST = json.loads((PROJECT_ROOT / "Art" / "M4ImportManifest.json").read_text(encoding="utf-8"))


def load(path):
    return unreal.EditorAssetLibrary.load_asset(path)


def asset_path(entry):
    return f'{entry["destination"]}/{entry["assetName"]}'


def main():
    result = {
        "missingRoles": [],
        "missingAssets": [],
        "classMismatches": [],
        "skeletonMismatches": [],
        "missingReferences": [],
        "unrecordedSources": [],
        "success": False,
    }
    expected_classes = {
        "skeletalMesh": unreal.SkeletalMesh,
        "staticMesh": unreal.StaticMesh,
        "animation": unreal.AnimSequence,
        "texture": unreal.Texture2D,
        "sound": unreal.SoundWave,
    }
    by_role = {entry["role"]: entry for entry in MANIFEST["assets"]}
    known_urls = {source["url"] for source in MANIFEST["sources"]}
    for entry in MANIFEST["assets"]:
        path = asset_path(entry)
        asset = load(path)
        if not asset:
            result["missingAssets"].append(path)
            continue
        expected = expected_classes[entry["assetType"]]
        if not isinstance(asset, expected):
            result["classMismatches"].append({"path": path, "expected": expected.__name__, "actual": asset.get_class().get_name()})
        if entry["sourceUrl"] not in known_urls:
            result["unrecordedSources"].append(entry["sourceUrl"])

    for entry in [item for item in MANIFEST["assets"] if item["assetType"] == "animation"]:
        animation = load(asset_path(entry))
        mesh_entry = by_role[entry["skeletonRole"]]
        mesh = load(asset_path(mesh_entry))
        if animation and mesh and animation.get_editor_property("skeleton") != mesh.get_editor_property("skeleton"):
            result["skeletonMismatches"].append({
                "animation": asset_path(entry),
                "animationSkeleton": animation.get_editor_property("skeleton").get_path_name(),
                "mesh": asset_path(mesh_entry),
                "meshSkeleton": mesh.get_editor_property("skeleton").get_path_name(),
            })

    unit_fields = ("skeletal_mesh", "idle_animation", "move_animation", "attack_animation", "death_animation")
    for unit_name in ("Infantry", "Archer", "Cavalry"):
        data_path = f"/Game/CityStateRTS/Data/DA_Unit_{unit_name}"
        data = load(data_path)
        for field in unit_fields:
            value = data.get_editor_property(field) if data else None
            if not value:
                result["missingReferences"].append(f"{data_path}.{field}")
            elif field == "skeletal_mesh" and value.get_path_name().startswith("/Engine/BasicShapes"):
                result["missingReferences"].append(f"{data_path}.{field}=BasicShapes")
        if unit_name == "Cavalry":
            for field in ("rider_skeletal_mesh", "rider_idle_animation", "rider_move_animation", "rider_attack_animation", "rider_death_animation"):
                if not data or not data.get_editor_property(field):
                    result["missingReferences"].append(f"{data_path}.{field}")

    for building_name in ("Barracks", "ArcheryRange", "Stable", "House", "Tower", "Wall", "Gate"):
        data_path = f"/Game/CityStateRTS/Data/DA_Building_{building_name}"
        data = load(data_path)
        mesh = data.get_editor_property("visual_mesh") if data else None
        if not mesh or mesh.get_path_name().startswith("/Engine/BasicShapes"):
            result["missingReferences"].append(f"{data_path}.visual_mesh")

    presentation_path = "/Game/CityStateRTS/Data/DA_Presentation"
    presentation = load(presentation_path)
    presentation_fields = (
        "player_faction_material", "enemy_faction_material", "neutral_faction_material", "construction_material", "hit_flash_material", "capture_ring_material",
        "capital_mesh", "town_mesh", "flag_mesh", "tree_mesh_a", "tree_mesh_b", "rock_mesh_a", "bridge_mesh",
        "move_command_effect", "attack_move_command_effect", "attack_target_effect", "hit_effect", "construction_effect", "construction_complete_effect",
        "destruction_effect", "capture_effect", "projectile_trail_effect", "select_sound", "move_sound", "attack_order_sound", "invalid_sound",
        "construction_start_sound", "construction_complete_sound", "training_complete_sound", "capture_contested_sound", "capture_complete_sound",
        "melee_hit_sound", "arrow_shot_sound", "arrow_hit_sound", "hoof_sound", "tower_shot_sound", "building_hit_sound", "building_destroyed_sound",
        "victory_sound", "defeat_sound", "world_attenuation", "combat_concurrency",
    )
    for field in presentation_fields:
        if not presentation or not presentation.get_editor_property(field):
            result["missingReferences"].append(f"{presentation_path}.{field}")

    result["unrecordedSources"] = sorted(set(result["unrecordedSources"]))
    result["success"] = not any(result[key] for key in result if key != "success")
    output_path = PROJECT_ROOT / "docs" / "verification" / "M4-assets.json"
    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_text(json.dumps(result, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    if not result["success"]:
        raise RuntimeError(f"M4 资产审计失败，详见 {output_path}")
    unreal.log(f"M4 资产审计通过：{len(MANIFEST['assets'])} 个角色。")


main()

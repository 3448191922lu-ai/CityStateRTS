import re

import unreal


ASSET_TOOLS = unreal.AssetToolsHelpers.get_asset_tools()


def load(path):
    asset = unreal.load_asset(path)
    if not asset:
        raise RuntimeError(f"缺少资产：{path}")
    return asset


def create_asset(name, folder, asset_class, factory):
    path = f"{folder}/{name}"
    existing = unreal.load_asset(path) if unreal.EditorAssetLibrary.does_asset_exist(path) else None
    return existing or ASSET_TOOLS.create_asset(name, folder, asset_class, factory)


def create_surface_master():
    material = create_asset("M_Surface", "/Game/CityStateRTS/Art/Materials", unreal.Material, unreal.MaterialFactoryNew())
    if not unreal.MaterialEditingLibrary.get_material_property_input_node(material, unreal.MaterialProperty.MP_BASE_COLOR):
        white = load("/Engine/EngineResources/WhiteSquareTexture.WhiteSquareTexture")
        normal = load("/Game/CityStateRTS/Art/Materials/Textures/T_Ground_Normal")
        base = unreal.MaterialEditingLibrary.create_material_expression(material, unreal.MaterialExpressionTextureSampleParameter2D, -700, -100)
        base.set_editor_property("parameter_name", "BaseColorTexture")
        base.set_editor_property("texture", white)
        tint = unreal.MaterialEditingLibrary.create_material_expression(material, unreal.MaterialExpressionVectorParameter, -700, 100)
        tint.set_editor_property("parameter_name", "FactionColor")
        tint.set_editor_property("default_value", unreal.LinearColor.WHITE)
        multiply = unreal.MaterialEditingLibrary.create_material_expression(material, unreal.MaterialExpressionMultiply, -350, 0)
        normal_sample = unreal.MaterialEditingLibrary.create_material_expression(material, unreal.MaterialExpressionTextureSampleParameter2D, -350, 220)
        normal_sample.set_editor_property("parameter_name", "NormalTexture")
        normal_sample.set_editor_property("texture", normal)
        roughness = unreal.MaterialEditingLibrary.create_material_expression(material, unreal.MaterialExpressionTextureSampleParameter2D, -350, 420)
        roughness.set_editor_property("parameter_name", "RoughnessTexture")
        roughness.set_editor_property("texture", white)
        unreal.MaterialEditingLibrary.connect_material_expressions(base, "RGB", multiply, "A")
        unreal.MaterialEditingLibrary.connect_material_expressions(tint, "RGB", multiply, "B")
        unreal.MaterialEditingLibrary.connect_material_property(multiply, "", unreal.MaterialProperty.MP_BASE_COLOR)
        unreal.MaterialEditingLibrary.connect_material_property(normal_sample, "RGB", unreal.MaterialProperty.MP_NORMAL)
        unreal.MaterialEditingLibrary.connect_material_property(roughness, "R", unreal.MaterialProperty.MP_ROUGHNESS)
        unreal.MaterialEditingLibrary.recompile_material(material)
    return material


def create_color_material(name, blend_mode, domain, color, opacity, capture_parameters=False):
    material = create_asset(name, "/Game/CityStateRTS/Art/Materials", unreal.Material, unreal.MaterialFactoryNew())
    material.set_editor_property("blend_mode", blend_mode)
    material.set_editor_property("material_domain", domain)
    material.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    if not unreal.MaterialEditingLibrary.get_material_property_input_node(material, unreal.MaterialProperty.MP_EMISSIVE_COLOR):
        color_node = unreal.MaterialEditingLibrary.create_material_expression(material, unreal.MaterialExpressionVectorParameter, -400, 0)
        color_node.set_editor_property("parameter_name", "FactionColor")
        color_node.set_editor_property("default_value", color)
        opacity_node = unreal.MaterialEditingLibrary.create_material_expression(material, unreal.MaterialExpressionScalarParameter, -400, 180)
        opacity_node.set_editor_property("parameter_name", "Opacity")
        opacity_node.set_editor_property("default_value", opacity)
        unreal.MaterialEditingLibrary.connect_material_property(color_node, "RGB", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
        opacity_output = opacity_node
        if capture_parameters:
            progress_node = unreal.MaterialEditingLibrary.create_material_expression(material, unreal.MaterialExpressionScalarParameter, -400, 320)
            progress_node.set_editor_property("parameter_name", "Progress")
            progress_node.set_editor_property("default_value", 1.0)
            contested_node = unreal.MaterialEditingLibrary.create_material_expression(material, unreal.MaterialExpressionScalarParameter, -400, 460)
            contested_node.set_editor_property("parameter_name", "Contested")
            contested_node.set_editor_property("default_value", 0.0)
            visible_amount = unreal.MaterialEditingLibrary.create_material_expression(material, unreal.MaterialExpressionMax, -160, 360)
            opacity_multiply = unreal.MaterialEditingLibrary.create_material_expression(material, unreal.MaterialExpressionMultiply, 40, 220)
            unreal.MaterialEditingLibrary.connect_material_expressions(progress_node, "", visible_amount, "A")
            unreal.MaterialEditingLibrary.connect_material_expressions(contested_node, "", visible_amount, "B")
            unreal.MaterialEditingLibrary.connect_material_expressions(opacity_node, "", opacity_multiply, "A")
            unreal.MaterialEditingLibrary.connect_material_expressions(visible_amount, "", opacity_multiply, "B")
            opacity_output = opacity_multiply
        unreal.MaterialEditingLibrary.connect_material_property(opacity_output, "", unreal.MaterialProperty.MP_OPACITY)
        unreal.MaterialEditingLibrary.recompile_material(material)
    return material


def create_material_instance(name, parent, color=None, textures=None):
    instance = create_asset(name, "/Game/CityStateRTS/Art/Materials", unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    instance.set_editor_property("parent", parent)
    if color:
        unreal.MaterialEditingLibrary.set_material_instance_vector_parameter_value(instance, "FactionColor", color)
    for parameter, texture in (textures or {}).items():
        unreal.MaterialEditingLibrary.set_material_instance_texture_parameter_value(instance, parameter, texture)
    return instance


def first_material_slot(mesh):
    property_name = "materials" if isinstance(mesh, unreal.SkeletalMesh) else "static_materials"
    materials = mesh.get_editor_property(property_name)
    return materials[0].get_editor_property("material_slot_name") if materials else unreal.Name("Faction")


def create_effect(name, template):
    target = f"/Game/CityStateRTS/Art/Effects/{name}"
    existing = unreal.load_asset(target) if unreal.EditorAssetLibrary.does_asset_exist(target) else None
    return existing or ASSET_TOOLS.duplicate_asset(name, "/Game/CityStateRTS/Art/Effects", load(template))


def configure_audio():
    attenuation = create_asset("ATT_WorldFeedback", "/Game/CityStateRTS/Art/Audio", unreal.SoundAttenuation, unreal.SoundAttenuationFactory())
    settings = attenuation.get_editor_property("attenuation")
    settings.set_editor_property("attenuate", True)
    settings.set_editor_property("spatialize", True)
    settings.set_editor_property("distance_algorithm", unreal.AttenuationDistanceModel.LOGARITHMIC)
    settings.set_editor_property("attenuation_shape", unreal.AttenuationShape.SPHERE)
    settings.set_editor_property("attenuation_shape_extents", unreal.Vector(600.0, 0.0, 0.0))
    settings.set_editor_property("falloff_distance", 3000.0)
    attenuation.set_editor_property("attenuation", settings)

    concurrency = create_asset("SC_CombatFeedback", "/Game/CityStateRTS/Art/Audio", unreal.SoundConcurrency, unreal.SoundConcurrencyFactory())
    concurrency_settings = concurrency.get_editor_property("concurrency")
    concurrency_settings.set_editor_property("max_count", 12)
    concurrency_settings.set_editor_property("resolution_rule", unreal.MaxConcurrentResolutionRule.STOP_FARTHEST_THEN_PREVENT_NEW)
    concurrency_settings.set_editor_property("volume_scale_mode", unreal.ConcurrencyVolumeScaleMode.DEFAULT)
    concurrency_settings.set_editor_property("volume_scale_can_release", True)
    concurrency.set_editor_property("concurrency", concurrency_settings)
    return attenuation, concurrency


def main():
    surface = create_surface_master()
    construction = create_color_material("M_Construction", unreal.BlendMode.BLEND_TRANSLUCENT, unreal.MaterialDomain.MD_SURFACE, unreal.LinearColor(0.25, 0.75, 1.0, 1.0), 0.45)
    hit_flash = create_color_material("M_HitFlash", unreal.BlendMode.BLEND_TRANSLUCENT, unreal.MaterialDomain.MD_SURFACE, unreal.LinearColor.WHITE, 0.65)
    capture_ring = create_color_material("M_CaptureRing", unreal.BlendMode.BLEND_TRANSLUCENT, unreal.MaterialDomain.MD_DEFERRED_DECAL, unreal.LinearColor.WHITE, 0.65, True)

    player = create_material_instance("MI_Faction_Player", surface, unreal.LinearColor(0.10, 0.34, 0.58, 1.0))
    enemy = create_material_instance("MI_Faction_Enemy", surface, unreal.LinearColor(0.61, 0.18, 0.14, 1.0))
    neutral = create_material_instance("MI_Faction_Neutral", surface, unreal.LinearColor(0.68, 0.54, 0.32, 1.0))
    for role in ("Stone", "Wood", "Ground", "Road"):
        create_material_instance(f"MI_{role}", surface, unreal.LinearColor.WHITE, {
            "BaseColorTexture": load(f"/Game/CityStateRTS/Art/Materials/Textures/T_{role}_BaseColor"),
            "NormalTexture": load(f"/Game/CityStateRTS/Art/Materials/Textures/T_{role}_Normal"),
            "RoughnessTexture": load(f"/Game/CityStateRTS/Art/Materials/Textures/T_{role}_Roughness"),
        })
        normal_texture = load(f"/Game/CityStateRTS/Art/Materials/Textures/T_{role}_Normal")
        normal_texture.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_NORMALMAP)
        normal_texture.set_editor_property("srgb", False)
        roughness_texture = load(f"/Game/CityStateRTS/Art/Materials/Textures/T_{role}_Roughness")
        roughness_texture.set_editor_property("srgb", False)

    attenuation, concurrency = configure_audio()
    effect_templates = {
        "NS_MoveCommand": "/Niagara/DefaultAssets/Templates/Systems/RadialBurst.RadialBurst",
        "NS_AttackMoveCommand": "/Niagara/DefaultAssets/Templates/Systems/RadialBurst.RadialBurst",
        "NS_AttackTarget": "/Niagara/DefaultAssets/Templates/Systems/DirectionalBurst.DirectionalBurst",
        "NS_Hit": "/Niagara/DefaultAssets/Templates/Systems/DirectionalBurst.DirectionalBurst",
        "NS_Construction": "/Niagara/DefaultAssets/Templates/Systems/FountainLightweight.FountainLightweight",
        "NS_ConstructionComplete": "/Niagara/DefaultAssets/Templates/Systems/RadialBurst.RadialBurst",
        "NS_Destruction": "/Niagara/DefaultAssets/Templates/Systems/SimpleExplosion.SimpleExplosion",
        "NS_Capture": "/Niagara/DefaultAssets/Templates/Systems/RadialBurst.RadialBurst",
        "NS_ProjectileTrail": "/Niagara/DefaultAssets/Templates/Systems/DirectionalBurstLightweight.DirectionalBurstLightweight",
    }
    effects = {name: create_effect(name, template) for name, template in effect_templates.items()}

    unit_paths = {
        "Infantry": ("Infantry", "Infantry", None),
        "Archer": ("Archer", "Archer", None),
        "Cavalry": ("Cavalry", "Horse", "Rider"),
    }
    for unit_name, (folder, animation_prefix, rider_prefix) in unit_paths.items():
        data = load(f"/Game/CityStateRTS/Data/DA_Unit_{unit_name}")
        mesh_name = "SK_Horse" if unit_name == "Cavalry" else f"SK_{unit_name}"
        mesh = load(f"/Game/CityStateRTS/Art/Characters/{folder}/{mesh_name}")
        data.set_editor_property("skeletal_mesh", mesh)
        data.set_editor_property("faction_material_slot", first_material_slot(mesh))
        for state in ("Idle", "Move", "Attack", "Death"):
            data.set_editor_property(f"{state.lower()}_animation", load(f"/Game/CityStateRTS/Art/Characters/Animations/A_{animation_prefix}_{state}"))
        if rider_prefix:
            rider = load("/Game/CityStateRTS/Art/Characters/Cavalry/SK_Rider")
            data.set_editor_property("rider_skeletal_mesh", rider)
            data.set_editor_property("rider_faction_material_slot", first_material_slot(rider))
            for state in ("Idle", "Move", "Attack", "Death"):
                data.set_editor_property(f"rider_{state.lower()}_animation", load(f"/Game/CityStateRTS/Art/Characters/Animations/A_Rider_{state}"))
        if unit_name == "Archer":
            data.set_editor_property("projectile_mesh", load("/Game/CityStateRTS/Art/Effects/SM_Arrow"))
            data.set_editor_property("attack_sound", load("/Game/CityStateRTS/Art/Audio/S_ArrowShot"))
            data.set_editor_property("hit_sound", load("/Game/CityStateRTS/Art/Audio/S_ArrowHit"))
        elif unit_name == "Cavalry":
            data.set_editor_property("attack_sound", load("/Game/CityStateRTS/Art/Audio/S_Hoof"))
            data.set_editor_property("hit_sound", load("/Game/CityStateRTS/Art/Audio/S_MeleeHit"))
        else:
            data.set_editor_property("attack_sound", load("/Game/CityStateRTS/Art/Audio/S_MeleeHit"))
            data.set_editor_property("hit_sound", load("/Game/CityStateRTS/Art/Audio/S_BuildingHit"))

    building_names = ("Barracks", "ArcheryRange", "Stable", "House", "Tower", "Wall", "Gate")
    for building_name in building_names:
        data = load(f"/Game/CityStateRTS/Data/DA_Building_{building_name}")
        mesh = load(f"/Game/CityStateRTS/Art/Buildings/SM_{building_name}")
        data.set_editor_property("visual_mesh", mesh)
        data.set_editor_property("faction_material_slot", first_material_slot(mesh))
        data.set_editor_property("hit_sound", load("/Game/CityStateRTS/Art/Audio/S_BuildingHit"))
        data.set_editor_property("destroyed_sound", load("/Game/CityStateRTS/Art/Audio/S_BuildingDestroyed"))
        if building_name == "Tower":
            data.set_editor_property("projectile_mesh", load("/Game/CityStateRTS/Art/Effects/SM_Arrow"))
            data.set_editor_property("attack_sound", load("/Game/CityStateRTS/Art/Audio/S_TowerShot"))

    factory = unreal.DataAssetFactory()
    factory.set_editor_property("data_asset_class", unreal.StrategyPresentationDataAsset)
    presentation = create_asset("DA_Presentation", "/Game/CityStateRTS/Data", unreal.StrategyPresentationDataAsset, factory)
    references = {
        "player_faction_material": player, "enemy_faction_material": enemy, "neutral_faction_material": neutral,
        "construction_material": construction, "hit_flash_material": hit_flash, "capture_ring_material": capture_ring,
        "capital_mesh": load("/Game/CityStateRTS/Art/Buildings/SM_Capital"), "town_mesh": load("/Game/CityStateRTS/Art/Buildings/SM_Town"),
        "flag_mesh": load("/Game/CityStateRTS/Art/Environment/SM_Flag"), "tree_mesh_a": load("/Game/CityStateRTS/Art/Environment/SM_Tree_Common"),
        "tree_mesh_b": load("/Game/CityStateRTS/Art/Environment/SM_Tree_Pine"), "rock_mesh_a": load("/Game/CityStateRTS/Art/Environment/SM_Rock"),
        "bridge_mesh": load("/Game/CityStateRTS/Art/Environment/SM_Bridge"),
        "move_command_effect": effects["NS_MoveCommand"], "attack_move_command_effect": effects["NS_AttackMoveCommand"],
        "attack_target_effect": effects["NS_AttackTarget"], "hit_effect": effects["NS_Hit"], "construction_effect": effects["NS_Construction"],
        "construction_complete_effect": effects["NS_ConstructionComplete"], "destruction_effect": effects["NS_Destruction"],
        "capture_effect": effects["NS_Capture"], "projectile_trail_effect": effects["NS_ProjectileTrail"],
        "world_attenuation": attenuation, "combat_concurrency": concurrency,
    }
    for role in ("Select", "Move", "AttackOrder", "Invalid", "ConstructionStart", "ConstructionComplete", "TrainingComplete", "CaptureContested", "CaptureComplete", "MeleeHit", "ArrowShot", "ArrowHit", "Hoof", "TowerShot", "BuildingHit", "BuildingDestroyed", "Victory", "Defeat"):
        property_name = re.sub(r"(?<!^)(?=[A-Z])", "_", role).lower() + "_sound"
        sound = load(f"/Game/CityStateRTS/Art/Audio/S_{role}")
        references[property_name] = sound
        if role not in ("Select", "Move", "AttackOrder", "Invalid", "Victory", "Defeat"):
            sound.set_editor_property("attenuation_settings", attenuation)
            sound.set_editor_property("concurrency_set", {concurrency})
    for property_name, value in references.items():
        presentation.set_editor_property(property_name, value)

    ground_material = load("/Game/CityStateRTS/Art/Materials/MI_Ground")
    for map_path in ("/Game/CityStateRTS/Maps/LVL_CityStateSkirmish", "/Game/CityStateRTS/Maps/LVL_RiverValleySkirmish"):
        unreal.EditorLoadingAndSavingUtils.load_map(map_path)
        for actor in unreal.EditorLevelLibrary.get_all_level_actors():
            if isinstance(actor, unreal.StaticMeshActor) and "ground" in actor.get_actor_label().lower():
                actor.static_mesh_component.set_material(0, ground_material)
        unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)

    for path in ("/Game/CityStateRTS/Art", "/Game/CityStateRTS/Data"):
        unreal.EditorAssetLibrary.save_directory(path, only_if_is_dirty=False, recursive=True)
    unreal.log("M4 展示资产配置完成。")


main()

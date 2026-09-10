import json
from pathlib import Path
import unreal

# 读取游戏实际加载的数据资产，不修改资源；供兵种和经济核对使用。
unit_fields = ['gold_cost', 'training_time', 'population_cost', 'member_count',
               'max_health', 'damage', 'attack_interval', 'attack_range', 'move_speed']
building_fields = ['gold_cost', 'construction_time', 'max_health', 'population_bonus']
report = {'units': {}, 'buildings': {}}
for name in ['Infantry', 'Archer', 'Cavalry']:
    asset = unreal.load_asset('/Game/CityStateRTS/Data/DA_Unit_' + name)
    report['units'][name] = {field: asset.get_editor_property(field) for field in unit_fields}
for name in ['Barracks', 'ArcheryRange', 'Stable', 'House', 'Tower', 'Wall', 'Gate']:
    asset = unreal.load_asset('/Game/CityStateRTS/Data/DA_Building_' + name)
    report['buildings'][name] = {field: asset.get_editor_property(field) for field in building_fields}
output = Path(unreal.Paths.project_saved_dir()) / 'Verification' / 'M3-Balance' / 'assets.json'
output.parent.mkdir(parents=True, exist_ok=True)
output.write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
unreal.log('Balance assets exported: ' + str(output))

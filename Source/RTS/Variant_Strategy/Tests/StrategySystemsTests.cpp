#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "StrategySystems.h"
#include "StrategyArtStyle.h"
#include "StrategyGameUserSettings.h"
#include "StrategyMapDefinition.h"
#include "StrategyPauseMenu.h"
#include "StrategyPlayerController.h"
#include "Engine/World.h"
#include "InputActionValue.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStrategyTrainingQueueTest,
	"RTS.Strategy.Systems.TrainingQueue",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStrategyTrainingQueueTest::RunTest(const FString& Parameters)
{
	FStrategyTrainingQueue Queue;
	TestTrue(TEXT("第一项应成功入队"), Queue.Enqueue(EStrategyUnitType::Infantry, 8.0f, 4));
	TestTrue(TEXT("第二项应成功入队"), Queue.Enqueue(EStrategyUnitType::Archer, 10.0f, 4));
	TestTrue(TEXT("第三项应成功入队"), Queue.Enqueue(EStrategyUnitType::Cavalry, 14.0f, 6));
	TestTrue(TEXT("第四项应成功入队"), Queue.Enqueue(EStrategyUnitType::Infantry, 8.0f, 4));
	TestTrue(TEXT("第五项应成功入队"), Queue.Enqueue(EStrategyUnitType::Infantry, 8.0f, 4));
	TestFalse(TEXT("第六项必须被容量限制拒绝"), Queue.Enqueue(EStrategyUnitType::Archer, 10.0f, 4));
	TestEqual(TEXT("销毁建筑时应释放全部预留人口"), Queue.GetReservedPopulation(), 22);

	EStrategyUnitType CompletedType = EStrategyUnitType::Cavalry;
	int32 CompletedPopulation = 0;
	TestFalse(TEXT("训练时间未满时不应完成"), Queue.Update(7.0f, CompletedType, CompletedPopulation));
	TestTrue(TEXT("达到八秒时应完成队首"), Queue.Update(1.0f, CompletedType, CompletedPopulation));
	TestEqual(TEXT("必须按 FIFO 完成步兵"), CompletedType, EStrategyUnitType::Infantry);
	TestEqual(TEXT("完成项应带回人口费用"), CompletedPopulation, 4);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStrategyFogGridTest,
	"RTS.Strategy.Systems.FogGrid",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStrategyFogGridTest::RunTest(const FString& Parameters)
{
	FStrategyFogGrid Grid;
	Grid.Initialize(128, 96, FVector2D(-9000.0f, -7000.0f), FVector2D(9000.0f, 7000.0f));
	Grid.BeginVisibilityUpdate();
	Grid.Reveal(FVector2D::ZeroVector, 1000.0f);
	TestTrue(TEXT("视野圆心应当前可见"), Grid.IsVisible(FVector2D::ZeroVector));
	TestFalse(TEXT("地图角落不应当前可见"), Grid.IsVisible(FVector2D(8900.0f, 6900.0f)));

	Grid.BeginVisibilityUpdate();
	TestFalse(TEXT("下一次更新后旧区域不再当前可见"), Grid.IsVisible(FVector2D::ZeroVector));
	TestTrue(TEXT("旧区域应保持已探索"), Grid.IsExplored(FVector2D::ZeroVector));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStrategyMapDefinitionTest,
	"RTS.Strategy.Systems.MapDefinition",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStrategyMapDefinitionTest::RunTest(const FString& Parameters)
{
	const FStrategySkirmishMapDefinition Original = FStrategyMapDefinitions::Resolve(TEXT("LVL_CityStateSkirmish"));
	TestFalse(TEXT("原地图不得生成河谷地形"), Original.bSpawnRiverValleyTerrain);
	TestEqual(TEXT("原地图必须保留三座城镇"), Original.NeutralTowns.Num(), 3);
	TestTrue(TEXT("原地图北部城镇坐标保持不变"), Original.NeutralTowns[1].Equals(FVector(0.0f, 4000.0f, 0.0f)));
	TestTrue(TEXT("摄像机最小边界必须保持原玩法范围"), Original.CameraMin.Equals(FVector2D(-11000.0f, -9000.0f)));
	TestTrue(TEXT("摄像机最大边界必须保持原玩法范围"), Original.CameraMax.Equals(FVector2D(11000.0f, 9000.0f)));
	TestTrue(TEXT("迷雾最小边界必须覆盖扩大的视觉地面"), Original.FogMin.Equals(FVector2D(-16000.0f, -14000.0f)));
	TestTrue(TEXT("迷雾最大边界必须覆盖扩大的视觉地面"), Original.FogMax.Equals(FVector2D(16000.0f, 14000.0f)));
	TestEqual(TEXT("扩大后迷雾网格必须保持足够精度"), Original.FogGridSize, FIntPoint(192, 160));

	FStrategyFogGrid OuterFog;
	OuterFog.Initialize(Original.FogGridSize.X, Original.FogGridSize.Y, Original.FogMin, Original.FogMax);
	OuterFog.BeginVisibilityUpdate();
	OuterFog.Reveal(FVector2D(-15000.0f, 0.0f), 200.0f);
	TestTrue(TEXT("原边界外的视觉地面必须由迷雾网格覆盖"), OuterFog.IsVisible(FVector2D(-15000.0f, 0.0f)));

	const FStrategySkirmishMapDefinition River = FStrategyMapDefinitions::Resolve(TEXT("UEDPIE_0_LVL_RiverValleySkirmish"));
	TestTrue(TEXT("PIE 前缀不得影响河谷地图识别"), River.bSpawnRiverValleyTerrain);
	TestEqual(TEXT("河谷地图必须有三座城镇"), River.NeutralTowns.Num(), 3);
	TestTrue(TEXT("北部城镇必须略偏玩家侧"), River.NeutralTowns[0].Equals(FVector(-2200.0f, 3900.0f, 0.0f)));
	TestTrue(TEXT("中央城镇必须位于浅滩"), River.NeutralTowns[1].Equals(FVector::ZeroVector));
	TestTrue(TEXT("南部城镇必须略偏 AI 侧"), River.NeutralTowns[2].Equals(FVector(2200.0f, -3900.0f, 0.0f)));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStrategyMapBuildRestrictionTest,
	"RTS.Strategy.Systems.MapBuildRestriction",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStrategyMapBuildRestrictionTest::RunTest(const FString& Parameters)
{
	const FStrategySkirmishMapDefinition River = FStrategyMapDefinitions::Resolve(TEXT("LVL_RiverValleySkirmish"));
	const FVector2D Footprint(200.0f, 200.0f);
	TestFalse(TEXT("北桥必须禁止建造"), FStrategyMapDefinitions::IsBuildingAllowed(River, FVector(0.0f, 3000.0f, 0.0f), Footprint));
	TestFalse(TEXT("中央浅滩必须禁止建造"), FStrategyMapDefinitions::IsBuildingAllowed(River, FVector::ZeroVector, Footprint));
	TestTrue(TEXT("桥头外侧必须允许地图规则建造"), FStrategyMapDefinitions::IsBuildingAllowed(River, FVector(-1800.0f, 3000.0f, 0.0f), Footprint));
	TestTrue(TEXT("主城附近必须允许地图规则建造"), FStrategyMapDefinitions::IsBuildingAllowed(River, FVector(-5000.0f, 0.0f, 0.0f), Footprint));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStrategyAIPlannerTest,
	"RTS.Strategy.Systems.AIPriority",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStrategyAIPlannerTest::RunTest(const FString& Parameters)
{
	FStrategyAIInputs Inputs;
	Inputs.bOwnedPointThreatened = true;
	Inputs.bNeutralPointAvailable = true;
	Inputs.bMissingProductionBuilding = true;
	TestEqual(TEXT("防守必须拥有最高优先级"), FStrategyAIPlanner::ChooseAction(Inputs), EStrategyAIAction::Defend);

	Inputs.bOwnedPointThreatened = false;
	TestEqual(TEXT("无威胁时优先占领中立据点"), FStrategyAIPlanner::ChooseAction(Inputs), EStrategyAIAction::Capture);

	Inputs.bNeutralPointAvailable = false;
	TestEqual(TEXT("没有中立据点时补齐生产设施"), FStrategyAIPlanner::ChooseAction(Inputs), EStrategyAIAction::Build);

	Inputs.bMissingProductionBuilding = false;
	Inputs.AvailableSquads = 3;
	TestEqual(TEXT("三支小队应组成进攻编队"), FStrategyAIPlanner::ChooseAction(Inputs), EStrategyAIAction::Attack);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStrategyCameraMovementTest,
	"RTS.Strategy.Systems.CameraMovement",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStrategyCameraMovementTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("W 必须对应屏幕向上"), FStrategyCameraMovement::ResolveScreenDirection(true, false, false, false), FVector2D(0.0f, 1.0f));
	TestEqual(TEXT("S 必须对应屏幕向下"), FStrategyCameraMovement::ResolveScreenDirection(false, true, false, false), FVector2D(0.0f, -1.0f));
	TestEqual(TEXT("A 必须对应屏幕向左"), FStrategyCameraMovement::ResolveScreenDirection(false, false, true, false), FVector2D(-1.0f, 0.0f));
	TestEqual(TEXT("D 必须对应屏幕向右"), FStrategyCameraMovement::ResolveScreenDirection(false, false, false, true), FVector2D(1.0f, 0.0f));
	TestEqual(TEXT("相反按键同时按下必须互相抵消"), FStrategyCameraMovement::ResolveScreenDirection(true, true, true, true), FVector2D::ZeroVector);

	const FVector WorldUp = FStrategyCameraMovement::ScreenToWorld(FVector2D(0.0f, 1.0f));
	const FVector WorldRight = FStrategyCameraMovement::ScreenToWorld(FVector2D(1.0f, 0.0f));
	TestTrue(TEXT("屏幕向上必须映射到相机画面顶部"), WorldUp.Equals(FVector(UE_SQRT_2 / 2.0f, -UE_SQRT_2 / 2.0f, 0.0f), KINDA_SMALL_NUMBER));
	TestTrue(TEXT("屏幕向右必须映射到相机画面右侧"), WorldRight.Equals(FVector(UE_SQRT_2 / 2.0f, UE_SQRT_2 / 2.0f, 0.0f), KINDA_SMALL_NUMBER));

	const FVector RotatedWorldUp = FStrategyCameraMovement::ScreenToWorld(FVector2D(0.0f, 1.0f), 45.0f);
	const FVector RotatedWorldRight = FStrategyCameraMovement::ScreenToWorld(FVector2D(1.0f, 0.0f), 45.0f);
	TestTrue(TEXT("旋转相机后 W 仍对应屏幕向上"), RotatedWorldUp.Equals(FVector(UE_SQRT_2 / 2.0f, UE_SQRT_2 / 2.0f, 0.0f), KINDA_SMALL_NUMBER));
	TestTrue(TEXT("旋转相机后 D 仍对应屏幕向右"), RotatedWorldRight.Equals(FVector(-UE_SQRT_2 / 2.0f, UE_SQRT_2 / 2.0f, 0.0f), KINDA_SMALL_NUMBER));

	const FVector CameraLocation(-8800.0f, 1800.0f, 4500.0f);
	const float CameraPitch = -53.0f;
	const float OldYaw = -45.0f;
	const float NewYaw = 45.0f;
	const FVector OldForward = FRotator(CameraPitch, OldYaw, 0.0f).Vector();
	const FVector OldFocus = CameraLocation + OldForward * (CameraLocation.Z / -OldForward.Z);
	const FVector OrbitLocation = FStrategyCameraMovement::GetOrbitCameraLocation(
		CameraLocation, CameraPitch, OldYaw, NewYaw);
	const FVector NewForward = FRotator(CameraPitch, NewYaw, 0.0f).Vector();
	const FVector NewFocus = OrbitLocation + NewForward * (OrbitLocation.Z / -NewForward.Z);
	TestTrue(TEXT("旋转后摄像机必须围绕同一地面视野中心"), NewFocus.Equals(OldFocus, 0.1f));
	TestTrue(TEXT("旋转镜头不能改变摄像机高度"),
		FMath::IsNearlyEqual(OrbitLocation.Z, CameraLocation.Z, KINDA_SMALL_NUMBER));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStrategyCameraBoundsTest,
	"RTS.Strategy.Systems.CameraBounds",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStrategyCameraBoundsTest::RunTest(const FString& Parameters)
{
	const FVector2D Extents = FStrategyCameraBoundsRules::GetGroundViewExtents(10000.0f, 16.0f / 9.0f, 53.0f, -45.0f);
	TestTrue(TEXT("最大缩放并旋转四十五度时必须覆盖完整地面视野"),
		Extents.Equals(FVector2D(10712.362f, 10712.362f), 1.0f));

	const FVector2D Clamped = FStrategyCameraBoundsRules::ClampGroundFocus(
		FVector2D(9000.0f, 8000.0f), FVector2D(-11000.0f, -9000.0f), FVector2D(11000.0f, 9000.0f), Extents, 200.0f);
	TestTrue(TEXT("镜头焦点必须退回地图安全范围"),
		Clamped.Equals(FVector2D(87.638f, 0.0f), 1.0f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStrategyCameraUltrawideBoundsTest,
	"RTS.Strategy.Systems.CameraUltrawideBounds",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStrategyCameraUltrawideBoundsTest::RunTest(const FString& Parameters)
{
	const FVector2D Extents = FStrategyCameraBoundsRules::GetGroundViewExtents(7000.0f, 2559.0f / 1089.0f, 53.0f, -45.0f);
	TestTrue(TEXT("超宽屏必须计入 UE 保持纵向视野后扩大的横向范围"),
		Extents.Equals(FVector2D(8914.49f, 8914.49f), 1.0f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStrategyArtStyleTest,
	"RTS.Strategy.ArtStyle.PaletteAndSilhouettes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStrategyArtStyleTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("玩家使用冷蓝阵营色"), StrategyArtStyle::GetFactionColor(EStrategyFaction::Player).B > StrategyArtStyle::GetFactionColor(EStrategyFaction::Player).R);
	TestTrue(TEXT("敌军使用赭红阵营色"), StrategyArtStyle::GetFactionColor(EStrategyFaction::Enemy).R > StrategyArtStyle::GetFactionColor(EStrategyFaction::Enemy).B);
	TestTrue(TEXT("弓兵轮廓高于步兵"), StrategyArtStyle::GetUnitSilhouetteScale(EStrategyUnitType::Archer).Z > StrategyArtStyle::GetUnitSilhouetteScale(EStrategyUnitType::Infantry).Z);
	TestTrue(TEXT("墙段横向轮廓明显"), StrategyArtStyle::GetBuildingSilhouetteScale(EStrategyBuildingType::Wall).X > StrategyArtStyle::GetBuildingSilhouetteScale(EStrategyBuildingType::Wall).Y);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStrategyWallPlannerTest,
	"RTS.Strategy.Systems.WallPlanner",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStrategyWallPlannerTest::RunTest(const FString& Parameters)
{
	const TArray<FStrategyWallSegmentPlan> Line = FStrategyWallPlanner::BuildLine(
		FVector::ZeroVector, FVector(2000.0f, 0.0f, 0.0f), 400.0f, 30);
	TestEqual(TEXT("2000 cm 墙线必须生成五段"), Line.Num(), 5);
	TestTrue(TEXT("第一段必须位于首个墙段中心"), Line[0].Location.Equals(FVector(200.0f, 0.0f, 0.0f)));
	TestTrue(TEXT("最后一段必须位于末个墙段中心"), Line[4].Location.Equals(FVector(1800.0f, 0.0f, 0.0f)));
	TestEqual(TEXT("单次墙线不得超过三十段"), FStrategyWallPlanner::BuildLine(
		FVector::ZeroVector, FVector(20000.0f, 0.0f, 0.0f), 400.0f, 30).Num(), 30);
	TestEqual(TEXT("200 金只能支付两段"), FStrategyWallPlanner::GetAffordableCount(5, 200.0f, 75.0f), 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStrategyWallPlacementRulesTest,
	"RTS.Strategy.Systems.WallPlacementRules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStrategyWallPlacementRulesTest::RunTest(const FString& Parameters)
{
	const TArray<int32> BuildableIndices = FStrategyWallPlacementRules::SelectBuildableIndices(
		{true, false, true}, 150.0f, 75.0f);
	TestEqual(TEXT("无效墙段必须被跳过"), BuildableIndices.Num(), 2);
	TestEqual(TEXT("第一段有效墙必须保留原索引"), BuildableIndices[0], 0);
	TestEqual(TEXT("第三段有效墙必须保留原索引"), BuildableIndices[1], 2);

	const TArray<int32> LimitedByGold = FStrategyWallPlacementRules::SelectBuildableIndices(
		{true, false, true}, 75.0f, 75.0f);
	TestEqual(TEXT("金币不足时只建造起点方向可负担的有效墙段"), LimitedByGold.Num(), 1);
	TestEqual(TEXT("金币限制下仍从起点方向选取"), LimitedByGold[0], 0);

	TestTrue(TEXT("己方完工城墙允许升级城门"), FStrategyWallPlacementRules::CanUpgradeToGate(
		EStrategyFaction::Player, EStrategyFaction::Player, EStrategyBuildingType::Wall, true));
	TestFalse(TEXT("敌方城墙不允许升级"), FStrategyWallPlacementRules::CanUpgradeToGate(
		EStrategyFaction::Player, EStrategyFaction::Enemy, EStrategyBuildingType::Wall, true));
	TestFalse(TEXT("未完工城墙不允许升级"), FStrategyWallPlacementRules::CanUpgradeToGate(
		EStrategyFaction::Player, EStrategyFaction::Player, EStrategyBuildingType::Wall, false));
	TestFalse(TEXT("非城墙建筑不允许升级"), FStrategyWallPlacementRules::CanUpgradeToGate(
		EStrategyFaction::Player, EStrategyFaction::Player, EStrategyBuildingType::Tower, true));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStrategyGateCollisionRulesTest,
	"RTS.Strategy.Systems.GateCollisionRules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStrategyGateCollisionRulesTest::RunTest(const FString& Parameters)
{
	TestFalse(TEXT("玩家城门必须允许玩家单位通过"), FStrategyGateCollisionRules::ShouldBlock(
		EStrategyFaction::Player, EStrategyFaction::Player, true));
	TestTrue(TEXT("玩家城门必须阻挡敌方单位"), FStrategyGateCollisionRules::ShouldBlock(
		EStrategyFaction::Player, EStrategyFaction::Enemy, true));
	TestTrue(TEXT("施工中的城门必须阻挡己方单位"), FStrategyGateCollisionRules::ShouldBlock(
		EStrategyFaction::Player, EStrategyFaction::Player, false));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStrategyBuildingGroundPlacementRulesTest,
	"RTS.Strategy.Systems.BuildingGroundPlacementRules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStrategyBuildingGroundPlacementRulesTest::RunTest(const FString& Parameters)
{
	const FVector RequestedLocation(1200.0f, -800.0f, 500.0f);
	const FVector ProjectedGround(1192.0f, -795.0f, 12.0f);
	TestEqual(TEXT("点击建筑顶部时保留平面坐标并使用导航地面高度"),
		FStrategyBuildingPlacementRules::ResolveGroundLocation(RequestedLocation, ProjectedGround),
		FVector(1200.0f, -800.0f, 12.0f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStrategyBlockedGateTargetTest,
	"RTS.Strategy.Systems.BlockedGateTarget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStrategyBlockedGateTargetTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("被敌方城门阻挡时必须转为攻击城门"), FStrategyGateCollisionRules::CanAttackBlockingBuilding(
		EStrategyFaction::Player, EStrategyFaction::Enemy, EStrategyBuildingType::Gate));
	TestFalse(TEXT("己方城门不得成为攻击目标"), FStrategyGateCollisionRules::CanAttackBlockingBuilding(
		EStrategyFaction::Player, EStrategyFaction::Player, EStrategyBuildingType::Gate));
	TestFalse(TEXT("其他敌方建筑不由城门阻挡规则处理"), FStrategyGateCollisionRules::CanAttackBlockingBuilding(
		EStrategyFaction::Player, EStrategyFaction::Enemy, EStrategyBuildingType::Tower));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStrategyTowerTargetRulesTest,
	"RTS.Strategy.Systems.TowerTargetRules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStrategyTowerTargetRulesTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("箭塔必须攻击范围内可见的存活敌军"), FStrategyTowerTargetRules::CanTarget(
		EStrategyFaction::Player, EStrategyFaction::Enemy, true, true, true, 1499.0f, 1500.0f));
	TestFalse(TEXT("未完工箭塔不得攻击"), FStrategyTowerTargetRules::CanTarget(
		EStrategyFaction::Player, EStrategyFaction::Enemy, false, true, true, 1000.0f, 1500.0f));
	TestFalse(TEXT("箭塔不得攻击迷雾中的敌军"), FStrategyTowerTargetRules::CanTarget(
		EStrategyFaction::Player, EStrategyFaction::Enemy, true, true, false, 1000.0f, 1500.0f));
	TestFalse(TEXT("箭塔不得攻击己方单位"), FStrategyTowerTargetRules::CanTarget(
		EStrategyFaction::Player, EStrategyFaction::Player, true, true, true, 1000.0f, 1500.0f));
	TestFalse(TEXT("箭塔不得攻击射程外敌军"), FStrategyTowerTargetRules::CanTarget(
		EStrategyFaction::Player, EStrategyFaction::Enemy, true, true, true, 1501.0f, 1500.0f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStrategySquadMarkerRulesTest,
	"RTS.Strategy.Systems.SquadMarkerRules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStrategySquadMarkerRulesTest::RunTest(const FString& Parameters)
{
	const TArray<FVector2D> Markers = {FVector2D(100.0f, 100.0f), FVector2D(120.0f, 100.0f)};
	TestEqual(TEXT("重叠命中时返回离光标最近的徽记"),
		FStrategySquadMarkerRules::FindHoveredMarker(Markers, FVector2D(116.0f, 100.0f), 17.0f), 1);
	TestEqual(TEXT("命中半径外不返回徽记"),
		FStrategySquadMarkerRules::FindHoveredMarker(Markers, FVector2D(200.0f, 200.0f), 17.0f), INDEX_NONE);
	TestTrue(TEXT("拖动已选小队时命令全部已选小队"),
		FStrategySquadMarkerRules::ShouldCommandSelectedSquads(true));
	TestFalse(TEXT("拖动未选小队时只命令源小队"),
		FStrategySquadMarkerRules::ShouldCommandSelectedSquads(false));
	TestEqual(TEXT("敌方目标转换为集火"),
		FStrategySquadMarkerRules::ResolveOrderType(true), EStrategyOrderType::AttackTarget);
	TestEqual(TEXT("地面目标转换为移动"),
		FStrategySquadMarkerRules::ResolveOrderType(false), EStrategyOrderType::Move);
	TestEqual(TEXT("一半生命与一次减员都保留初始生命上限"),
		FStrategySquadMarkerRules::CalculateHealthPercent(300.0f, 600.0f), 0.5f);
	TestFalse(TEXT("普通建筑放置时禁用徽记输入"), FStrategySquadMarkerRules::CanInteract(true, false));
	TestFalse(TEXT("城墙拖拽时禁用徽记输入"), FStrategySquadMarkerRules::CanInteract(false, true));
	TestTrue(TEXT("普通状态允许徽记输入"), FStrategySquadMarkerRules::CanInteract(false, false));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStrategySquadMarkerClickSequenceTest,
	"RTS.Strategy.Systems.SquadMarkerClickSequence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStrategySquadMarkerClickSequenceTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	AStrategyPlayerController* Controller = World->SpawnActor<AStrategyPlayerController>();
	// 长拖拽不会触发 Tap，完成拖拽后遗留的标记不能吞掉下一次建造点击。
	Controller->bConsumeNextSelectClick = true;
	Controller->ClearSquadMarkerInput();
	Controller->bBuildingPlacementActive = true;
	Controller->SelectHoldStarted(FInputActionValue(true));
	TestFalse(TEXT("拖拽后的下一次按下应恢复普通点击"), Controller->bConsumeNextSelectClick);
	World->DestroyWorld(false);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStrategyIntroPromptRulesTest,
	"RTS.Strategy.Systems.IntroPromptRules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStrategyIntroPromptRulesTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("开局七秒内提示必须完全可见"), FStrategyIntroPromptRules::GetOpacity(6.9f), 1.0f);
	TestEqual(TEXT("最后一秒提示必须线性淡出"), FStrategyIntroPromptRules::GetOpacity(7.5f), 0.5f);
	TestEqual(TEXT("八秒后提示必须隐藏"), FStrategyIntroPromptRules::GetOpacity(8.0f), 0.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStrategyOrderTargetRulesTest,
	"RTS.Strategy.Systems.OrderTargetRules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStrategyOrderTargetRulesTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("可见存活敌军允许成为攻击目标"), FStrategyOrderTargetRules::CanAttack(
		EStrategyFaction::Player, EStrategyFaction::Enemy, true, true));
	TestFalse(TEXT("迷雾中的敌军不得成为攻击目标"), FStrategyOrderTargetRules::CanAttack(
		EStrategyFaction::Player, EStrategyFaction::Enemy, true, false));
	TestFalse(TEXT("己方单位不得成为攻击目标"), FStrategyOrderTargetRules::CanAttack(
		EStrategyFaction::Player, EStrategyFaction::Player, true, true));
	TestFalse(TEXT("死亡敌军不得成为攻击目标"), FStrategyOrderTargetRules::CanAttack(
		EStrategyFaction::Player, EStrategyFaction::Enemy, false, true));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStrategySettingsRulesTest,
	"RTS.Strategy.Systems.SettingsRules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStrategySettingsRulesTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("负音量限制为零"), FStrategySettingsRules::SnapMasterVolume(-0.3f), 0.0f);
	TestEqual(TEXT("音量按百分之五取整"), FStrategySettingsRules::SnapMasterVolume(0.53f), 0.55f);
	TestEqual(TEXT("过高音量限制为一"), FStrategySettingsRules::SnapMasterVolume(1.4f), 1.0f);
	TestEqual(TEXT("低画质映射"), FStrategySettingsRules::QualityIndexToLevel(0), 0);
	TestEqual(TEXT("史诗画质映射"), FStrategySettingsRules::QualityIndexToLevel(3), 3);
	TestEqual(TEXT("窗口模式映射"), FStrategySettingsRules::WindowModeIndexToValue(0), EWindowMode::Windowed);
	TestEqual(TEXT("无边框模式映射"), FStrategySettingsRules::WindowModeIndexToValue(1), EWindowMode::WindowedFullscreen);
	TestEqual(TEXT("独占全屏映射"), FStrategySettingsRules::WindowModeIndexToValue(2), EWindowMode::Fullscreen);
	TestEqual(TEXT("分辨率显示"), FStrategySettingsRules::FormatResolution(FIntPoint(1920, 1080)), FString(TEXT("1920 x 1080")));
	TestEqual(TEXT("固定北向设置索引"), FStrategySettingsRules::MinimapOrientationToIndex(
		EStrategyMinimapOrientation::NorthUp), 0);
	TestEqual(TEXT("跟随摄像机设置索引"), FStrategySettingsRules::MinimapOrientationToIndex(
		EStrategyMinimapOrientation::FollowCamera), 1);
	TestEqual(TEXT("固定北向索引解析"), FStrategySettingsRules::MinimapOrientationFromIndex(0),
		EStrategyMinimapOrientation::NorthUp);
	TestEqual(TEXT("跟随摄像机索引解析"), FStrategySettingsRules::MinimapOrientationFromIndex(1),
		EStrategyMinimapOrientation::FollowCamera);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStrategySettingsMenuFlowTest,
	"RTS.Strategy.Systems.SettingsMenuFlow",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStrategySettingsMenuFlowTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("暂停页 Esc 关闭菜单"),
		FStrategyPauseMenuRules::ResolveEscape(EStrategyPauseMenuPage::Pause),
		EStrategyPauseMenuEscapeAction::CloseMenu);
	TestEqual(TEXT("设置页 Esc 放弃并返回"),
		FStrategyPauseMenuRules::ResolveEscape(EStrategyPauseMenuPage::Settings),
		EStrategyPauseMenuEscapeAction::DiscardAndReturn);

	TArray<FIntPoint> Resolutions = { FIntPoint(1920, 1080), FIntPoint(1280, 720), FIntPoint(1920, 1080) };
	FStrategyPauseMenuRules::NormalizeResolutions(Resolutions, FIntPoint(1600, 900));
	TestEqual(TEXT("分辨率去重并保留当前项"), Resolutions.Num(), 3);
	TestEqual(TEXT("分辨率按面积排序"), Resolutions[0], FIntPoint(1280, 720));
	TestTrue(TEXT("当前分辨率存在"), Resolutions.Contains(FIntPoint(1600, 900)));
	const UStrategyPauseMenu* Menu = NewObject<UStrategyPauseMenu>();
	TestTrue(TEXT("暂停菜单必须能获取键盘焦点以处理 Esc"), Menu->IsFocusable());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStrategyPresentationRulesTest,
	"RTS.Strategy.Systems.PresentationRules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStrategyPresentationRulesTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("死亡优先"), FStrategyPresentationRules::ResolveUnitVisualState(false, true, 100.0f), EStrategyUnitVisualState::Dead);
	TestEqual(TEXT("攻击优先于移动"), FStrategyPresentationRules::ResolveUnitVisualState(true, true, 100.0f), EStrategyUnitVisualState::Attack);
	TestEqual(TEXT("有速度时移动"), FStrategyPresentationRules::ResolveUnitVisualState(true, false, 1.0f), EStrategyUnitVisualState::Move);
	TestEqual(TEXT("静止时待机"), FStrategyPresentationRules::ResolveUnitVisualState(true, false, 0.0f), EStrategyUnitVisualState::Idle);

	TestTrue(TEXT("玩家反馈不受玩家视野限制"), FStrategyPresentationRules::CanPlayWorldFeedback(EStrategyFaction::Player, false));
	TestTrue(TEXT("可见敌军允许播放反馈"), FStrategyPresentationRules::CanPlayWorldFeedback(EStrategyFaction::Enemy, true));
	TestFalse(TEXT("不可见敌军不得泄露反馈"), FStrategyPresentationRules::CanPlayWorldFeedback(EStrategyFaction::Enemy, false));
	TestTrue(TEXT("可见中立目标允许播放反馈"), FStrategyPresentationRules::CanPlayWorldFeedback(EStrategyFaction::Neutral, true));
	TestFalse(TEXT("不可见中立目标不得播放反馈"), FStrategyPresentationRules::CanPlayWorldFeedback(EStrategyFaction::Neutral, false));

	const FVector ImportedForward(0.0f, 1.0f, 0.0f);
	const FVector CorrectedForward = FStrategyPresentationRules::GetImportedCharacterMeshRotation().RotateVector(ImportedForward);
	TestTrue(TEXT("导入角色的 +Y 前向必须校正到单位 +X 移动方向"),
		CorrectedForward.Equals(FVector::ForwardVector, KINDA_SMALL_NUMBER));
	return true;
}

#endif

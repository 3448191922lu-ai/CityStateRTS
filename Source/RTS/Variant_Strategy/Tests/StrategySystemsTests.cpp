#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "StrategySystems.h"
#include "StrategyArtStyle.h"

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

#endif

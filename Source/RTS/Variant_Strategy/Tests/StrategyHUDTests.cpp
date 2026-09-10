#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "StrategyHUDModel.h"
#include "StrategyMinimapModel.h"
#include "StrategySystems.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStrategyMinimapProjectionTest,
	"RTS.Strategy.UI.MinimapProjection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStrategyMinimapProjectionTest::RunTest(const FString& Parameters)
{
	const FVector2D MapMin(-16000.0f, -14000.0f);
	const FVector2D MapMax(16000.0f, 14000.0f);
	const FVector2D Size(300.0f, 240.0f);
	const float InitialCameraYaw = -45.0f;
	const FVector2D InitialViewUp(1000.0f, -1000.0f);
	const FVector2D NorthLocal = FStrategyMinimapProjection::WorldToLocal(
		InitialViewUp, MapMin, MapMax, Size, EStrategyMinimapOrientation::NorthUp, InitialCameraYaw, InitialCameraYaw);
	TestTrue(TEXT("玩家初始视角上方必须映射到小地图正上方"),
		FMath::IsNearlyEqual(NorthLocal.X, Size.X * 0.5f, 0.01f) && NorthLocal.Y < Size.Y * 0.5f);
	TestTrue(TEXT("开局时固定北向与跟随摄像机方向必须一致"), NorthLocal.Equals(
		FStrategyMinimapProjection::WorldToLocal(InitialViewUp, MapMin, MapMax, Size,
			EStrategyMinimapOrientation::FollowCamera, InitialCameraYaw, InitialCameraYaw), 0.01f));
	TestTrue(TEXT("地图中心映射到控件中心"), FStrategyMinimapProjection::WorldToLocal(
		FVector2D::ZeroVector, MapMin, MapMax, Size,
		EStrategyMinimapOrientation::NorthUp, InitialCameraYaw, InitialCameraYaw).Equals(Size * 0.5f, 0.01f));

	const FVector2D World(4200.0f, -3100.0f);
	const FVector2D FollowLocal = FStrategyMinimapProjection::WorldToLocal(
		World, MapMin, MapMax, Size, EStrategyMinimapOrientation::FollowCamera, -45.0f, InitialCameraYaw);
	TestTrue(TEXT("旋转模式必须可逆"), FStrategyMinimapProjection::LocalToWorld(
		FollowLocal, MapMin, MapMax, Size,
		EStrategyMinimapOrientation::FollowCamera, -45.0f, InitialCameraYaw).Equals(World, 1.0f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStrategyMinimapRulesTest,
	"RTS.Strategy.UI.MinimapRules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStrategyMinimapRulesTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("己方对象始终显示"), FStrategyMinimapVisibilityRules::ShouldDraw(
		EStrategyMinimapEntityKind::Squad, EStrategyFaction::Player, false, false));
	TestFalse(TEXT("不可见敌军不能显示"), FStrategyMinimapVisibilityRules::ShouldDraw(
		EStrategyMinimapEntityKind::Squad, EStrategyFaction::Enemy, false, true));
	TestTrue(TEXT("当前可见敌军可以显示"), FStrategyMinimapVisibilityRules::ShouldDraw(
		EStrategyMinimapEntityKind::Building, EStrategyFaction::Enemy, true, true));
	TestTrue(TEXT("已探索城镇保留图标"), FStrategyMinimapVisibilityRules::ShouldDraw(
		EStrategyMinimapEntityKind::Town, EStrategyFaction::Enemy, false, true));
	TestTrue(TEXT("固定中立城镇允许显示无归属图标"), FStrategyMinimapVisibilityRules::ShouldDraw(
		EStrategyMinimapEntityKind::Town, EStrategyFaction::Neutral, false, false));
	TestEqual(TEXT("无选择不下令"), FStrategyMinimapCommandRules::ResolveIntent(false, true),
		EStrategyMinimapCommandIntent::None);
	TestEqual(TEXT("可见敌军触发攻击"), FStrategyMinimapCommandRules::ResolveIntent(true, true),
		EStrategyMinimapCommandIntent::Attack);
	TestEqual(TEXT("其余位置触发移动"), FStrategyMinimapCommandRules::ResolveIntent(true, false),
		EStrategyMinimapCommandIntent::Move);
	TestTrue(TEXT("720p 小地图按比例缩小"),
		FStrategyMinimapLayoutRules::ResolveSize(720.0f).Equals(FVector2D(250.0f, 200.0f)));
	TestTrue(TEXT("1080p 使用默认尺寸"),
		FStrategyMinimapLayoutRules::ResolveSize(1080.0f).Equals(FVector2D(300.0f, 240.0f)));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStrategyHUDContextRulesTest,
	"RTS.Strategy.UI.ContextRules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStrategyHUDContextRulesTest::RunTest(const FString& Parameters)
{
	FStrategyHUDContextInputs Inputs;
	TestEqual(TEXT("无选择显示基础操作"), FStrategyHUDLayoutRules::ResolveContext(Inputs), EStrategyHUDContext::Idle);
	Inputs.SelectedSquadCount = 2;
	TestEqual(TEXT("小队上下文"), FStrategyHUDLayoutRules::ResolveContext(Inputs), EStrategyHUDContext::Squad);
	Inputs.bHasBuilding = true;
	TestEqual(TEXT("建筑优先于小队"), FStrategyHUDLayoutRules::ResolveContext(Inputs), EStrategyHUDContext::Building);
	Inputs.bHasTown = true;
	TestEqual(TEXT("城镇优先于建筑"), FStrategyHUDLayoutRules::ResolveContext(Inputs), EStrategyHUDContext::Town);
	Inputs.bBuildMode = true;
	TestEqual(TEXT("建造模式优先级最高"), FStrategyHUDLayoutRules::ResolveContext(Inputs), EStrategyHUDContext::Build);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStrategyHUDScaleRulesTest,
	"RTS.Strategy.UI.ScaleRules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStrategyHUDScaleRulesTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("720 高度保持整像素文字"), FStrategyHUDLayoutRules::ResolveScale(720.0f), 1.0f);
	TestEqual(TEXT("窗口标题栏不产生小数缩放"), FStrategyHUDLayoutRules::ResolveScale(1032.0f), 1.0f);
	TestEqual(TEXT("1080 高度使用基准缩放"), FStrategyHUDLayoutRules::ResolveScale(1080.0f), 1.0f);
	TestEqual(TEXT("1440 高度不无限放大"), FStrategyHUDLayoutRules::ResolveScale(1440.0f), 1.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStrategyGarrisonCommandRulesTest,
	"RTS.Strategy.UI.GarrisonCommandRules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStrategyGarrisonCommandRulesTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("右键己方据点解析为驻防"), FStrategyGarrisonCommandRules::ShouldEnterPoint(
		EStrategyFaction::Player, EStrategyFaction::Player, true));
	TestFalse(TEXT("右键敌方据点不解析为驻防"), FStrategyGarrisonCommandRules::ShouldEnterPoint(
		EStrategyFaction::Player, EStrategyFaction::Enemy, true));
	TestFalse(TEXT("驻军在范围内移动时不出城"), FStrategyGarrisonCommandRules::ShouldExitForOrder(
		EStrategyOrderType::Move, 650.0f));
	TestTrue(TEXT("驻军向范围外移动时出城"), FStrategyGarrisonCommandRules::ShouldExitForOrder(
		EStrategyOrderType::Move, 651.0f));
	TestTrue(TEXT("驻军向范围外攻击移动时出城"), FStrategyGarrisonCommandRules::ShouldExitForOrder(
		EStrategyOrderType::AttackMove, 651.0f));
	TestTrue(TEXT("驻军集火范围外目标时出城"), FStrategyGarrisonCommandRules::ShouldExitForOrder(
		EStrategyOrderType::AttackTarget, 651.0f));
	TestFalse(TEXT("停止命令保持驻防"), FStrategyGarrisonCommandRules::ShouldExitForOrder(
		EStrategyOrderType::Stop, 1000.0f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStrategyHUDAffordabilityStateTest,
	"RTS.Strategy.UI.AffordabilityState",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStrategyHUDAffordabilityStateTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("同一可购买状态不随金币个位变化"),
		FStrategyHUDLayoutRules::ResolveAffordabilityState(251.0f, 250.0f),
		FStrategyHUDLayoutRules::ResolveAffordabilityState(252.0f, 250.0f));
	TestNotEqual(TEXT("跨过费用阈值才改变按钮状态"),
		FStrategyHUDLayoutRules::ResolveAffordabilityState(249.0f, 250.0f),
		FStrategyHUDLayoutRules::ResolveAffordabilityState(250.0f, 250.0f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStrategyHUDTrainingSnapshotTest,
	"RTS.Strategy.UI.TrainingSnapshot",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStrategyHUDTrainingSnapshotTest::RunTest(const FString& Parameters)
{
	FStrategyTrainingQueue Queue;
	Queue.Enqueue(EStrategyUnitType::Infantry, 8.0f, 4);
	EStrategyUnitType CompletedType;
	int32 PopulationCost = 0;
	Queue.Update(2.0f, CompletedType, PopulationCost);
	TestEqual(TEXT("队列公开首项类型"), Queue.GetItems()[0].UnitType, EStrategyUnitType::Infantry);
	TestEqual(TEXT("首项训练进度"), Queue.GetFrontProgress(), 0.25f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStrategyHUDActionRulesTest,
	"RTS.Strategy.UI.ActionRules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStrategyHUDActionRulesTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("未建成建筑不可训练"), FStrategyHUDActionRules::GetTrainingUnavailableReason(
		false, true, 0, 500.0f, 0, 20, 100.0f, 4), EStrategyHUDUnavailableReason::UnderConstruction);
	TestEqual(TEXT("队列满优先提示"), FStrategyHUDActionRules::GetTrainingUnavailableReason(
		true, true, 5, 500.0f, 0, 20, 100.0f, 4), EStrategyHUDUnavailableReason::QueueFull);
	TestEqual(TEXT("金币不足"), FStrategyHUDActionRules::GetTrainingUnavailableReason(
		true, true, 0, 99.0f, 0, 20, 100.0f, 4), EStrategyHUDUnavailableReason::NotEnoughGold);
	TestEqual(TEXT("人口不足"), FStrategyHUDActionRules::GetTrainingUnavailableReason(
		true, true, 0, 500.0f, 17, 20, 100.0f, 4), EStrategyHUDUnavailableReason::PopulationFull);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStrategyHUDVisibilityTest,
	"RTS.Strategy.UI.Visibility",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStrategyHUDVisibilityTest::RunTest(const FString& Parameters)
{
	const FStrategyHUDVisibility Town = FStrategyHUDVisibility::ForContext(EStrategyHUDContext::Town);
	TestTrue(TEXT("城镇显示对象信息"), Town.bObjectPanel);
	TestTrue(TEXT("城镇显示详情"), Town.bDetailPanel);
	TestTrue(TEXT("城镇显示命令"), Town.bCommandPanel);
	TestFalse(TEXT("城镇不显示基础提示"), Town.bIdleHelp);
	const FStrategyHUDVisibility Idle = FStrategyHUDVisibility::ForContext(EStrategyHUDContext::Idle);
	TestTrue(TEXT("空选择显示基础提示"), Idle.bIdleHelp);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStrategyHUDPlacementIssueTest,
	"RTS.Strategy.UI.PlacementIssue",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStrategyHUDPlacementIssueTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("地图禁区优先"), FStrategyPlacementIssueRules::Resolve(false, false, false, true),
		EStrategyBuildingPlacementIssue::MapRestricted);
	TestEqual(TEXT("领地不足"), FStrategyPlacementIssueRules::Resolve(true, false, false, true),
		EStrategyBuildingPlacementIssue::OutsideTerritory);
	TestEqual(TEXT("不可导航"), FStrategyPlacementIssueRules::Resolve(true, true, false, true),
		EStrategyBuildingPlacementIssue::NotNavigable);
	TestEqual(TEXT("占地重叠"), FStrategyPlacementIssueRules::Resolve(true, true, true, true),
		EStrategyBuildingPlacementIssue::Overlap);
	TestEqual(TEXT("合法位置"), FStrategyPlacementIssueRules::Resolve(true, true, true, false),
		EStrategyBuildingPlacementIssue::None);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStrategyHUDNotificationQueueTest,
	"RTS.Strategy.UI.NotificationQueue",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStrategyHUDNotificationQueueTest::RunTest(const FString& Parameters)
{
	FStrategyHUDNotificationQueue Queue;
	Queue.Push(TEXT("一"), FLinearColor::White);
	Queue.Push(TEXT("二"), FLinearColor::White);
	Queue.Push(TEXT("三"), FLinearColor::White);
	Queue.Push(TEXT("四"), FLinearColor::White);
	TestEqual(TEXT("最多三条"), Queue.GetItems().Num(), 3);
	TestEqual(TEXT("移除最旧通知"), Queue.GetItems()[0].Message, FString(TEXT("二")));
	Queue.Update(3.1f);
	TestTrue(TEXT("三秒后清空"), Queue.GetItems().IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStrategyHUDUpdateCacheTest,
	"RTS.Strategy.UI.UpdateCache",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStrategyHUDUpdateCacheTest::RunTest(const FString& Parameters)
{
	FStrategyHUDUpdateCache Cache;
	TestTrue(TEXT("首次文字需要刷新"), Cache.AcceptText(TEXT("Resource"), TEXT("金币 500")));
	TestFalse(TEXT("相同文字不重复刷新"), Cache.AcceptText(TEXT("Resource"), TEXT("金币 500")));
	TestTrue(TEXT("文字变化时刷新"), Cache.AcceptText(TEXT("Resource"), TEXT("金币 501")));
	TestTrue(TEXT("首次缩放需要刷新"), Cache.AcceptScalar(TEXT("Scale"), 0.85f));
	TestFalse(TEXT("相同缩放不重复刷新"), Cache.AcceptScalar(TEXT("Scale"), 0.85f));
	TestTrue(TEXT("缩放变化时刷新"), Cache.AcceptScalar(TEXT("Scale"), 1.0f));
	return true;
}

#endif

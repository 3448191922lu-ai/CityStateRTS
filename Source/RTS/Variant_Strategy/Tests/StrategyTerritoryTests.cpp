#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "StrategyGameState.h"
#include "StrategySystems.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStrategyTownDevelopmentTest,
	"RTS.Strategy.Territory.DevelopmentRules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStrategyTownDevelopmentTest::RunTest(const FString& Parameters)
{
	FStrategyTownDevelopment Town;
	TestFalse(TEXT("249 金币不能开始专精"),
		FStrategyTownDevelopmentRules::CanStartSpecialization(Town, EStrategyTownSpecialization::Trade, 249.0f));
	TestTrue(TEXT("250 金币可以开始专精"),
		FStrategyTownDevelopmentRules::CanStartSpecialization(Town, EStrategyTownSpecialization::Trade, 250.0f));

	FStrategyTownDevelopmentRules::StartSpecialization(Town, EStrategyTownSpecialization::Trade);
	TestEqual(TEXT("开始后进入建设中"), Town.State, EStrategyTownDevelopmentState::Building);
	FStrategyTownDevelopmentRules::Update(Town, 19.0f, false, false);
	TestEqual(TEXT("十九秒尚未完成"), Town.State, EStrategyTownDevelopmentState::Building);
	FStrategyTownDevelopmentRules::Update(Town, 2.0f, true, false);
	TestEqual(TEXT("争夺时暂停"), Town.ProgressSeconds, 19.0f);
	TestEqual(TEXT("二十秒完成事件"),
		FStrategyTownDevelopmentRules::Update(Town, 1.0f, false, false),
		EStrategyTownUpdateResult::BuildCompleted);
	TestEqual(TEXT("完成后激活"), Town.State, EStrategyTownDevelopmentState::Active);

	FStrategyTownDevelopmentRules::StartDowngrade(Town);
	TestEqual(TEXT("降级立即停用"), Town.State, EStrategyTownDevelopmentState::Downgrading);
	FStrategyTownDevelopmentRules::Update(Town, 5.0f, true, false);
	TestEqual(TEXT("降级争夺时暂停"), Town.ProgressSeconds, 0.0f);
	TestEqual(TEXT("降级完成事件"),
		FStrategyTownDevelopmentRules::Update(Town, 10.0f, false, false),
		EStrategyTownUpdateResult::DowngradeCompleted);
	TestEqual(TEXT("降级后清除专精"), Town.Specialization, EStrategyTownSpecialization::None);
	TestEqual(TEXT("降级后返回未专精"), Town.State, EStrategyTownDevelopmentState::Unspecialized);
	TestEqual(TEXT("退款为四成"), FStrategyTownDevelopmentRules::GetDowngradeRefund(), 100.0f);

	FStrategyTownDevelopment CapturedTown;
	CapturedTown.Specialization = EStrategyTownSpecialization::Fortress;
	CapturedTown.State = EStrategyTownDevelopmentState::Active;
	FStrategyTownDevelopmentRules::HandleOwnershipChanged(CapturedTown);
	TestEqual(TEXT("成熟专精易手后瘫痪"), CapturedTown.State, EStrategyTownDevelopmentState::DisabledAfterCapture);
	TestEqual(TEXT("易手后保留已完成专精"), CapturedTown.Specialization, EStrategyTownSpecialization::Fortress);
	FStrategyTownDevelopmentRules::Update(CapturedTown, 9.0f, false, true);
	FStrategyTownDevelopmentRules::Update(CapturedTown, 2.0f, true, true);
	TestEqual(TEXT("重新启用争夺时暂停"), CapturedTown.ProgressSeconds, 9.0f);
	FStrategyTownDevelopmentRules::Update(CapturedTown, 5.0f, false, false);
	TestEqual(TEXT("无驻军五秒恢复到零"), CapturedTown.ProgressSeconds, 0.0f);
	TestEqual(TEXT("连续驻军十秒完成重新启用"),
		FStrategyTownDevelopmentRules::Update(CapturedTown, 10.0f, false, true),
		EStrategyTownUpdateResult::ReactivationCompleted);
	TestEqual(TEXT("重新启用后激活"), CapturedTown.State, EStrategyTownDevelopmentState::Active);

	FStrategyTownDevelopment UnfinishedTown;
	FStrategyTownDevelopmentRules::StartSpecialization(UnfinishedTown, EStrategyTownSpecialization::Recruitment);
	FStrategyTownDevelopmentRules::HandleOwnershipChanged(UnfinishedTown);
	TestEqual(TEXT("未完成专精易手后取消"), UnfinishedTown.Specialization, EStrategyTownSpecialization::None);
	TestEqual(TEXT("取消工程后未专精"), UnfinishedTown.State, EStrategyTownDevelopmentState::Unspecialized);

	FStrategyTownDevelopment DowngradingTown;
	DowngradingTown.Specialization = EStrategyTownSpecialization::Trade;
	DowngradingTown.State = EStrategyTownDevelopmentState::Active;
	FStrategyTownDevelopmentRules::StartDowngrade(DowngradingTown);
	FStrategyTownDevelopmentRules::HandleOwnershipChanged(DowngradingTown);
	TestEqual(TEXT("降级中易手保留已完成专精"), DowngradingTown.Specialization, EStrategyTownSpecialization::Trade);
	TestEqual(TEXT("降级中易手转为瘫痪"), DowngradingTown.State, EStrategyTownDevelopmentState::DisabledAfterCapture);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStrategyGarrisonRulesTest,
	"RTS.Strategy.Territory.GarrisonRules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStrategyGarrisonRulesTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("普通城镇容量为二"), FStrategyGarrisonRules::GetCapacity(false,
		EStrategyTownSpecialization::None, EStrategyTownDevelopmentState::Unspecialized), 2);
	TestEqual(TEXT("主城容量为二"), FStrategyGarrisonRules::GetCapacity(true,
		EStrategyTownSpecialization::None, EStrategyTownDevelopmentState::Unspecialized), 2);
	TestEqual(TEXT("激活要塞容量为三"), FStrategyGarrisonRules::GetCapacity(false,
		EStrategyTownSpecialization::Fortress, EStrategyTownDevelopmentState::Active), 3);
	TestEqual(TEXT("瘫痪要塞退回普通容量"), FStrategyGarrisonRules::GetCapacity(false,
		EStrategyTownSpecialization::Fortress, EStrategyTownDevelopmentState::DisabledAfterCapture), 2);

	TestEqual(TEXT("普通据点等待三秒"), FStrategyGarrisonRules::GetRecoveryDelay(
		EStrategyTownSpecialization::None, EStrategyTownDevelopmentState::Unspecialized), 3.0f);
	TestEqual(TEXT("普通据点每秒恢复百分之三"), FStrategyGarrisonRules::GetRecoveryRate(
		EStrategyTownSpecialization::None, EStrategyTownDevelopmentState::Unspecialized), 0.03f);
	TestEqual(TEXT("普通据点八秒补员"), FStrategyGarrisonRules::GetReinforcementInterval(
		EStrategyTownSpecialization::None, EStrategyTownDevelopmentState::Unspecialized), 8.0f);
	TestEqual(TEXT("激活要塞立即恢复"), FStrategyGarrisonRules::GetRecoveryDelay(
		EStrategyTownSpecialization::Fortress, EStrategyTownDevelopmentState::Active), 0.0f);
	TestEqual(TEXT("激活要塞每秒恢复百分之五"), FStrategyGarrisonRules::GetRecoveryRate(
		EStrategyTownSpecialization::Fortress, EStrategyTownDevelopmentState::Active), 0.05f);
	TestEqual(TEXT("激活要塞六秒补员"), FStrategyGarrisonRules::GetReinforcementInterval(
		EStrategyTownSpecialization::Fortress, EStrategyTownDevelopmentState::Active), 6.0f);

	TestFalse(TEXT("六百五十厘米内保持驻防"), FStrategyGarrisonRules::ShouldExitForDestination(650.0f));
	TestTrue(TEXT("超过六百五十厘米出城"), FStrategyGarrisonRules::ShouldExitForDestination(651.0f));
	TestTrue(TEXT("生命低于六成时 AI 回城"), FStrategyGarrisonRules::ShouldAIRetreat(0.59f));
	TestFalse(TEXT("生命达到六成时 AI 不回城"), FStrategyGarrisonRules::ShouldAIRetreat(0.60f));
	TestFalse(TEXT("生命不足九成时 AI 留城"), FStrategyGarrisonRules::ShouldAILeave(0.89f));
	TestTrue(TEXT("生命达到九成时 AI 出城"), FStrategyGarrisonRules::ShouldAILeave(0.90f));
	TestEqual(TEXT("按初始总生命计算恢复量"),
		FStrategyGarrisonRules::GetRecoveryAmount(600.0f, 0.03f, 2.0f), 36.0f);
	TestEqual(TEXT("恢复生命不超过成员上限"),
		FStrategyGarrisonRules::ClampRecoveredHealth(140.0f, 150.0f, 20.0f), 150.0f);
	TestFalse(TEXT("满编小队不补员"), FStrategyGarrisonRules::ShouldReinforce(4, 4, 8.0f, 8.0f));
	TestFalse(TEXT("补员计时未到不补员"), FStrategyGarrisonRules::ShouldReinforce(3, 4, 7.9f, 8.0f));
	TestTrue(TEXT("缺员且计时达到时补员"), FStrategyGarrisonRules::ShouldReinforce(3, 4, 8.0f, 8.0f));
	TestTrue(TEXT("同阵营存活小队且有空位可驻防"), FStrategyGarrisonRules::CanEnter(
		EStrategyFaction::Player, EStrategyFaction::Player, true, false, 1, 2));
	TestFalse(TEXT("敌方小队不可驻防"), FStrategyGarrisonRules::CanEnter(
		EStrategyFaction::Enemy, EStrategyFaction::Player, true, false, 0, 2));
	TestFalse(TEXT("满员据点不可驻防"), FStrategyGarrisonRules::CanEnter(
		EStrategyFaction::Player, EStrategyFaction::Player, true, false, 2, 2));
	TestFalse(TEXT("已驻防小队不可重复进入"), FStrategyGarrisonRules::CanEnter(
		EStrategyFaction::Player, EStrategyFaction::Player, true, true, 0, 2));
	TestTrue(TEXT("所属方据点有驻军且敌军进入时反击"), FStrategyGarrisonRules::ShouldSortie(
		EStrategyFaction::Player, true, false, 2));
	TestFalse(TEXT("只有己方单位时不反击"), FStrategyGarrisonRules::ShouldSortie(
		EStrategyFaction::Player, false, true, 2));
	const TArray<FStrategyGarrisonDestination> Destinations = {
		{FVector2D(1000.0f, 0.0f), true, false, true},
		{FVector2D(-1000.0f, 0.0f), true, true, false},
		{FVector2D(200.0f, 0.0f), false, false, false}
	};
	TestEqual(TEXT("等距时要塞优先于主城"), FStrategyGarrisonRules::FindBestDestination(
		FVector2D::ZeroVector, Destinations), 1);
	TestEqual(TEXT("不可用近点会被跳过"), FStrategyGarrisonRules::FindBestDestination(
		FVector2D(250.0f, 0.0f), Destinations), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStrategySupplyNetworkTest,
	"RTS.Strategy.Territory.SupplyNetwork",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStrategySupplyNetworkTest::RunTest(const FString& Parameters)
{
	TArray<FStrategySupplyNode> Plains = {
		{FVector2D(-7000.0f, 0.0f), EStrategyFaction::Player, true},
		{FVector2D(0.0f, 0.0f), EStrategyFaction::Player, false},
		{FVector2D(0.0f, 4000.0f), EStrategyFaction::Player, false},
		{FVector2D(0.0f, -4000.0f), EStrategyFaction::Enemy, false}
	};
	const TSet<int32> PlainsConnected = FStrategySupplyRules::FindConnectedTownIndices(Plains, EStrategyFaction::Player);
	TestTrue(TEXT("中央城镇连接主城"), PlainsConnected.Contains(1));
	TestTrue(TEXT("同阵营翼城通过中央城镇连接"), PlainsConnected.Contains(2));
	TestFalse(TEXT("敌方城镇不连接"), PlainsConnected.Contains(3));

	TArray<FStrategySupplyNode> River = {
		{FVector2D(-7000.0f, 0.0f), EStrategyFaction::Player, true},
		{FVector2D(-2200.0f, 3900.0f), EStrategyFaction::Player, false},
		{FVector2D(0.0f, 0.0f), EStrategyFaction::Player, false},
		{FVector2D(2200.0f, -3900.0f), EStrategyFaction::Player, false}
	};
	TSet<int32> RiverConnected = FStrategySupplyRules::FindConnectedTownIndices(River, EStrategyFaction::Player);
	TestTrue(TEXT("河谷临近侧翼连接"), RiverConnected.Contains(1));
	TestTrue(TEXT("河谷中央城镇连接"), RiverConnected.Contains(2));
	TestTrue(TEXT("河谷远端侧翼通过中央连接"), RiverConnected.Contains(3));

	River[2].Owner = EStrategyFaction::Enemy;
	RiverConnected = FStrategySupplyRules::FindConnectedTownIndices(River, EStrategyFaction::Player);
	TestTrue(TEXT("中央易手后临近侧翼仍连接"), RiverConnected.Contains(1));
	TestFalse(TEXT("中央易手后远端侧翼断线"), RiverConnected.Contains(3));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStrategyTownSpecializationEffectsTest,
	"RTS.Strategy.Territory.SpecializationEffects",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStrategyTownSpecializationEffectsTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("贸易主体收入"), FStrategyTownSpecializationRules::GetIncomeBonus(
		EStrategyTownSpecialization::Trade, EStrategyTownDevelopmentState::Active, false), 4.0f);
	TestEqual(TEXT("贸易连接收入"), FStrategyTownSpecializationRules::GetIncomeBonus(
		EStrategyTownSpecialization::Trade, EStrategyTownDevelopmentState::Active, true), 6.0f);
	TestEqual(TEXT("征募增加人口"), FStrategyTownSpecializationRules::GetPopulationBonus(
		EStrategyTownSpecialization::Recruitment, EStrategyTownDevelopmentState::Active), 10);
	TestEqual(TEXT("征募主体倍率"), FStrategyTownSpecializationRules::GetTrainingTimeMultiplier(
		EStrategyTownSpecialization::Recruitment, EStrategyTownDevelopmentState::Active, false), 0.8f);
	TestEqual(TEXT("征募连接倍率"), FStrategyTownSpecializationRules::GetTrainingTimeMultiplier(
		EStrategyTownSpecialization::Recruitment, EStrategyTownDevelopmentState::Active, true), 0.7f);
	TestEqual(TEXT("要塞占领秒数"), FStrategyTownSpecializationRules::GetCaptureDuration(
		EStrategyTownSpecialization::Fortress, EStrategyTownDevelopmentState::Active), 15.0f);
	TestEqual(TEXT("要塞主体射程"), FStrategyTownSpecializationRules::GetFortressRange(
		EStrategyTownSpecialization::Fortress, EStrategyTownDevelopmentState::Active, false), 1200.0f);
	TestEqual(TEXT("要塞连接射程"), FStrategyTownSpecializationRules::GetFortressRange(
		EStrategyTownSpecialization::Fortress, EStrategyTownDevelopmentState::Active, true), 1500.0f);
	TestEqual(TEXT("要塞主体伤害"), FStrategyTownSpecializationRules::GetFortressDamage(
		EStrategyTownSpecialization::Fortress, EStrategyTownDevelopmentState::Active, false), 20.0f);
	TestEqual(TEXT("要塞连接伤害"), FStrategyTownSpecializationRules::GetFortressDamage(
		EStrategyTownSpecialization::Fortress, EStrategyTownDevelopmentState::Active, true), 25.0f);

	for (const EStrategyTownDevelopmentState State : {
		EStrategyTownDevelopmentState::Building,
		EStrategyTownDevelopmentState::Downgrading,
		EStrategyTownDevelopmentState::DisabledAfterCapture})
	{
		TestEqual(TEXT("非激活贸易没有收入加成"),
			FStrategyTownSpecializationRules::GetIncomeBonus(EStrategyTownSpecialization::Trade, State, true), 0.0f);
		TestEqual(TEXT("非激活征募没有人口加成"),
			FStrategyTownSpecializationRules::GetPopulationBonus(EStrategyTownSpecialization::Recruitment, State), 0);
		TestEqual(TEXT("非激活征募不缩短训练"),
			FStrategyTownSpecializationRules::GetTrainingTimeMultiplier(EStrategyTownSpecialization::Recruitment, State, true), 1.0f);
		TestEqual(TEXT("非激活要塞不延长占领"),
			FStrategyTownSpecializationRules::GetCaptureDuration(EStrategyTownSpecialization::Fortress, State), 10.0f);
		TestEqual(TEXT("非激活要塞没有射程"),
			FStrategyTownSpecializationRules::GetFortressRange(EStrategyTownSpecialization::Fortress, State, true), 0.0f);
		TestEqual(TEXT("非激活要塞没有伤害"),
			FStrategyTownSpecializationRules::GetFortressDamage(EStrategyTownSpecialization::Fortress, State, true), 0.0f);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStrategyFactionEconomyTotalsTest,
	"RTS.Strategy.Territory.FactionEconomyTotals",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStrategyFactionEconomyTotalsTest::RunTest(const FString& Parameters)
{
	const TArray<FStrategyTownContribution> Contributions = {
		{EStrategyFaction::Player, 5.0f, 20},
		{EStrategyFaction::Player, 3.0f, 5},
		{EStrategyFaction::Player, 7.0f, 15},
		{EStrategyFaction::Enemy, 9.0f, 5}
	};
	const FStrategyFactionEconomyTotals Totals = FStrategyFactionEconomyRules::Calculate(
		EStrategyFaction::Player, Contributions, 10);
	TestEqual(TEXT("只汇总己方据点"), Totals.OwnedPoints, 3);
	TestEqual(TEXT("基础与专精收入汇总"), Totals.IncomePerSecond, 15.0f);
	TestEqual(TEXT("据点与一座民居人口汇总"), Totals.PopulationCap, 50);

	const TArray<FStrategyTownContribution> ExcessPopulation = {
		{EStrategyFaction::Player, 5.0f, 20},
		{EStrategyFaction::Player, 3.0f, 45}
	};
	TestEqual(TEXT("总人口硬上限为六十"), FStrategyFactionEconomyRules::Calculate(
		EStrategyFaction::Player, ExcessPopulation, 20).PopulationCap, 60);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStrategyTownActionAvailabilityTest,
	"RTS.Strategy.Territory.TownActionAvailability",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStrategyTownActionAvailabilityTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("己方未专精城镇可管理"), FStrategyTownActionRules::CanChooseSpecialization(
		EStrategyFaction::Player, EStrategyFaction::Player, false, EStrategyTownDevelopmentState::Unspecialized));
	TestFalse(TEXT("敌方城镇不可管理"), FStrategyTownActionRules::CanChooseSpecialization(
		EStrategyFaction::Player, EStrategyFaction::Enemy, false, EStrategyTownDevelopmentState::Unspecialized));
	TestFalse(TEXT("主城不可专精"), FStrategyTownActionRules::CanChooseSpecialization(
		EStrategyFaction::Player, EStrategyFaction::Player, true, EStrategyTownDevelopmentState::Unspecialized));
	TestTrue(TEXT("己方激活城镇可降级"), FStrategyTownActionRules::CanDowngrade(
		EStrategyFaction::Player, EStrategyFaction::Player, false, EStrategyTownDevelopmentState::Active));
	TestFalse(TEXT("瘫痪城镇不可直接降级"), FStrategyTownActionRules::CanDowngrade(
		EStrategyFaction::Player, EStrategyFaction::Player, false, EStrategyTownDevelopmentState::DisabledAfterCapture));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStrategyTownVisibilityPolicyTest,
	"RTS.Strategy.Territory.TownVisibilityPolicy",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStrategyTownVisibilityPolicyTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("己方城镇始终显示管理详情"), FStrategyTownVisibilityRules::CanShowLiveDetails(
		EStrategyFaction::Player, EStrategyFaction::Player, false));
	TestTrue(TEXT("可见敌镇显示公开详情"), FStrategyTownVisibilityRules::CanShowPublicDetails(
		EStrategyFaction::Player, EStrategyFaction::Enemy, true));
	TestFalse(TEXT("迷雾外敌镇不显示公开详情"), FStrategyTownVisibilityRules::CanShowPublicDetails(
		EStrategyFaction::Player, EStrategyFaction::Enemy, false));
	TestFalse(TEXT("迷雾外敌镇不显示实时详情"), FStrategyTownVisibilityRules::CanShowLiveDetails(
		EStrategyFaction::Player, EStrategyFaction::Enemy, false));
	TestFalse(TEXT("绝不显示敌方补给"), FStrategyTownVisibilityRules::CanShowSupplyConnection(
		EStrategyFaction::Player, EStrategyFaction::Enemy));
	TestEqual(TEXT("敌方建设中的选择不得提前公开"), FStrategyTownVisibilityRules::GetPublicSpecialization(
		EStrategyFaction::Player, EStrategyFaction::Enemy, true, EStrategyTownSpecialization::Fortress,
		EStrategyTownDevelopmentState::Building), EStrategyTownSpecialization::None);
	TestEqual(TEXT("可见敌方已完成专精可以公开"), FStrategyTownVisibilityRules::GetPublicSpecialization(
		EStrategyFaction::Player, EStrategyFaction::Enemy, true, EStrategyTownSpecialization::Fortress,
		EStrategyTownDevelopmentState::Active), EStrategyTownSpecialization::Fortress);
	TestEqual(TEXT("己方建设选择始终可见"), FStrategyTownVisibilityRules::GetPublicSpecialization(
		EStrategyFaction::Player, EStrategyFaction::Player, false, EStrategyTownSpecialization::Recruitment,
		EStrategyTownDevelopmentState::Building), EStrategyTownSpecialization::Recruitment);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStrategyTownPanelDesiredSizeTest,
	"RTS.Strategy.Territory.TownPanelDesiredSize",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStrategyTownPanelDesiredSizeTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("城镇面板画布槽必须采用子控件期望尺寸"),
		FStrategyTownPanelLayoutRules::ShouldUseDesiredSize());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStrategyTownAIChoiceTest,
	"RTS.Strategy.Territory.TownAIChoice",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStrategyTownAIChoiceTest::RunTest(const FString& Parameters)
{
	FStrategyTownAIInputs Inputs;
	TestEqual(TEXT("第一座优先贸易"), FStrategyTownAIPlanner::ChooseSpecialization(Inputs),
		EStrategyTownSpecialization::Trade);

	Inputs.bHasTradeTown = true;
	TestEqual(TEXT("第二座优先征募"), FStrategyTownAIPlanner::ChooseSpecialization(Inputs),
		EStrategyTownSpecialization::Recruitment);

	Inputs.bHasRecruitmentTown = true;
	TestEqual(TEXT("已有经济与征募后选择要塞"), FStrategyTownAIPlanner::ChooseSpecialization(Inputs),
		EStrategyTownSpecialization::Fortress);

	Inputs.bTownThreatened = true;
	TestEqual(TEXT("受威胁城镇优先要塞"), FStrategyTownAIPlanner::ChooseSpecialization(Inputs),
		EStrategyTownSpecialization::Fortress);

	TestFalse(TEXT("金币不足时不重新专精"), FStrategyTownAIPlanner::ShouldRespecialize(
		EStrategyTownSpecialization::Trade, EStrategyTownSpecialization::Recruitment, 499.0f));
	TestFalse(TEXT("角色相同时不重新专精"), FStrategyTownAIPlanner::ShouldRespecialize(
		EStrategyTownSpecialization::Trade, EStrategyTownSpecialization::Trade, 500.0f));
	TestTrue(TEXT("金币充足且角色不同时重新专精"), FStrategyTownAIPlanner::ShouldRespecialize(
		EStrategyTownSpecialization::Trade, EStrategyTownSpecialization::Recruitment, 500.0f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStrategyPresentationDefinitionAvailabilityTest,
	"RTS.Strategy.Territory.PresentationDefinitionAvailability",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStrategyPresentationDefinitionAvailabilityTest::RunTest(const FString& Parameters)
{
	AStrategyGameState* State = NewObject<AStrategyGameState>();
	TestNotNull(TEXT("表现数据首次访问时必须完成加载"), State->GetPresentationDefinition());
	return true;
}

#endif

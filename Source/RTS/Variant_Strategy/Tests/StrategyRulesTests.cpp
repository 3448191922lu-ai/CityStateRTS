#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "StrategyGameState.h"
#include "StrategyRules.h"
#include "StrategyTypes.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStrategyEconomyTest,
	"RTS.Strategy.Rules.Economy",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStrategyEconomyTest::RunTest(const FString& Parameters)
{
	FStrategyFactionState State;
	State.AddIncome(2.0f);
	TestEqual(TEXT("两秒主城收入应为十金币"), State.Gold, 510.0f);

	TestTrue(TEXT("资源足够时应预留训练费用"), State.TrySpendAndReserve(100.0f, 4));
	TestEqual(TEXT("金币应立即扣除"), State.Gold, 410.0f);
	TestEqual(TEXT("人口应先进入预留"), State.ReservedPopulation, 4);

	State.CommitPopulation(4);
	TestEqual(TEXT("训练完成后应占用人口"), State.UsedPopulation, 4);
	TestEqual(TEXT("训练完成后应清除预留"), State.ReservedPopulation, 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStrategyPopulationLimitTest,
	"RTS.Strategy.Rules.PopulationLimit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStrategyPopulationLimitTest::RunTest(const FString& Parameters)
{
	FStrategyFactionState State;
	State.UsedPopulation = 18;
	TestFalse(TEXT("超过人口上限的训练必须被拒绝"), State.TrySpendAndReserve(100.0f, 4));
	TestEqual(TEXT("失败时不得扣除金币"), State.Gold, 500.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStrategyCounterDamageTest,
	"RTS.Strategy.Rules.CounterDamage",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStrategyCounterDamageTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("步兵克制骑兵"), FStrategyRules::GetDamageMultiplier(EStrategyUnitType::Infantry, EStrategyUnitType::Cavalry), 1.5f);
	TestEqual(TEXT("骑兵克制弓兵"), FStrategyRules::GetDamageMultiplier(EStrategyUnitType::Cavalry, EStrategyUnitType::Archer), 1.5f);
	TestEqual(TEXT("弓兵克制步兵"), FStrategyRules::GetDamageMultiplier(EStrategyUnitType::Archer, EStrategyUnitType::Infantry), 1.5f);
	TestEqual(TEXT("非克制关系使用基础伤害"), FStrategyRules::GetDamageMultiplier(EStrategyUnitType::Infantry, EStrategyUnitType::Archer), 1.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStrategyCaptureTest,
	"RTS.Strategy.Rules.Capture",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStrategyCaptureTest::RunTest(const FString& Parameters)
{
	FStrategyCaptureState State;
	State.Update(5.0f, true, false);
	TestEqual(TEXT("单方驻留应推进占领"), State.ProgressSeconds, 5.0f);

	State.Update(2.0f, true, true);
	TestEqual(TEXT("争夺状态应暂停占领"), State.ProgressSeconds, 5.0f);

	State.Update(2.0f, false, false);
	TestEqual(TEXT("无人驻留时占领进度应在五秒内完全回退"), State.ProgressSeconds, 1.0f);

	State.Update(9.0f, true, false);
	TestEqual(TEXT("达到十秒后城镇归玩家"), State.Owner, EStrategyFaction::Player);
	TestEqual(TEXT("完成占领后进度归零"), State.ProgressSeconds, 0.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStrategyVictoryTest,
	"RTS.Strategy.Rules.Victory",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStrategyVictoryTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("双方主城存活时比赛继续"), FStrategyRules::GetWinner(true, true), EStrategyFaction::Neutral);
	TestEqual(TEXT("敌方主城被摧毁时玩家获胜"), FStrategyRules::GetWinner(true, false), EStrategyFaction::Player);
	TestEqual(TEXT("玩家主城被摧毁时敌方获胜"), FStrategyRules::GetWinner(false, true), EStrategyFaction::Enemy);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStrategyEarlyFactionInitializationTest,
	"RTS.Strategy.Rules.EarlyFactionInitialization",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStrategyEarlyFactionInitializationTest::RunTest(const FString& Parameters)
{
	const AStrategyGameState* Defaults = GetDefault<AStrategyGameState>();
	TestTrue(TEXT("GameMode BeginPlay 前玩家阵营状态必须存在"), Defaults->HasFactionState(EStrategyFaction::Player));
	TestTrue(TEXT("GameMode BeginPlay 前敌方阵营状态必须存在"), Defaults->HasFactionState(EStrategyFaction::Enemy));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStrategyBuildingFootprintTest,
	"RTS.Strategy.Rules.BuildingFootprint",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStrategyBuildingFootprintTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("墙段四角均在领地内时必须允许建造"), FStrategyRules::IsFootprintInsideTerritory(
		FVector::ZeroVector, FVector2D(200.0f, 60.0f), 0.0f, FVector::ZeroVector, 1000.0f));
	TestFalse(TEXT("旋转墙段有角点越界时必须拒绝建造"), FStrategyRules::IsFootprintInsideTerritory(
		FVector(850.0f, 0.0f, 0.0f), FVector2D(200.0f, 60.0f), 45.0f, FVector::ZeroVector, 1000.0f));
	return true;
}

#endif

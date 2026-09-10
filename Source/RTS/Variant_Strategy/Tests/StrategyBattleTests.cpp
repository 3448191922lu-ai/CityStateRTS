#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Tests/AutomationEditorCommon.h"
#include "Tests/AutomationCommon.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "StrategyGameState.h"
#include "StrategyUnit.h"
#include "StrategyWorldActors.h"

namespace StrategyBattleProbe
{
struct FSide
{
	TArray<TWeakObjectPtr<AStrategySquad>> Squads;
	float InitialHealth = 0.0f;
	float Gold = 0.0f;
	int32 Population = 0;

	float Health() const
	{
		float Total = 0.0f;
		for (const auto& Squad : Squads)
		{
			if (Squad.IsValid())
			{
				const auto* Definition = Squad->GetWorld()->GetGameState<AStrategyGameState>()->GetUnitDefinition(Squad->GetUnitType());
				Total += Squad->GetHealthPercent() * Definition->MaxHealth * Definition->MemberCount;
			}
		}
		return Total;
	}

	int32 Members() const
	{
		int32 Total = 0;
		for (const auto& Squad : Squads)
		{
			if (Squad.IsValid()) { Total += Squad->GetMembers().Num(); }
		}
		return Total;
	}

	void Order(const FStrategyOrder& Command) const
	{
		for (const auto& Squad : Squads)
		{
			Squad->IssueOrder(Command);
		}
	}
};

class FRunBattle : public IAutomationLatentCommand
{
public:
	FRunBattle(FAutomationTestBase* InTest, FString InMap, int32 InScenario, bool bInMirrored)
		: Test(InTest), Map(MoveTemp(InMap)), Scenario(InScenario), bMirrored(bInMirrored) {}

	virtual bool Update() override
	{
		UWorld* World = nullptr;
		for (const FWorldContext& Context : GEngine->GetWorldContexts())
		{
			if (Context.WorldType == EWorldType::PIE) { World = Context.World(); break; }
		}
		if (!World)
		{
			Test->AddError(TEXT("实战对照未获得 PIE 世界"));
			return true;
		}
		if (!bStarted)
		{
			// 清除上一组单位及战略 AI，只保留地图、导航、迷雾与真实单位战斗。
			TArray<AActor*> Cleanup;
			for (TActorIterator<AActor> It(World); It; ++It)
			{
				if (It->IsA<AStrategyAICommander>() || It->IsA<AStrategyUnit>() || It->IsA<AStrategySquad>())
				{
					Cleanup.Add(*It);
				}
			}
			for (AActor* Actor : Cleanup) { Actor->Destroy(); }
			AStrategyGameState* State = World->GetGameState<AStrategyGameState>();
			const float Sign = bMirrored ? -1.0f : 1.0f;
			const EStrategyFaction LeftFaction = bMirrored ? EStrategyFaction::Enemy : EStrategyFaction::Player;
			const EStrategyFaction RightFaction = bMirrored ? EStrategyFaction::Player : EStrategyFaction::Enemy;
			const bool bChase = Scenario >= 2;
			const TArray<EStrategyUnitType> LeftTypes = Scenario == 0
				? TArray<EStrategyUnitType>{EStrategyUnitType::Infantry, EStrategyUnitType::Infantry}
				: Scenario == 1 ? TArray<EStrategyUnitType>{EStrategyUnitType::Infantry, EStrategyUnitType::Archer}
				: TArray<EStrategyUnitType>{Scenario == 2 ? EStrategyUnitType::Cavalry : EStrategyUnitType::Infantry};
			const TArray<EStrategyUnitType> RightTypes = Scenario == 0
				? TArray<EStrategyUnitType>{EStrategyUnitType::Cavalry}
				: Scenario == 1 ? TArray<EStrategyUnitType>{EStrategyUnitType::Infantry, EStrategyUnitType::Infantry}
				: TArray<EStrategyUnitType>{EStrategyUnitType::Archer};
			auto SpawnSide = [State](FSide& Side, const TArray<EStrategyUnitType>& Types, EStrategyFaction Faction, FVector Start, float Behind)
			{
				for (int32 Index = 0; Index < Types.Num(); ++Index)
				{
					const auto* Definition = State->GetUnitDefinition(Types[Index]);
					Side.Gold += Definition->GoldCost;
					Side.Population += Definition->PopulationCost;
					Side.Squads.Add(State->SpawnSquad(Faction, Types[Index], Start + FVector(Index * Behind, 0, 0), false));
				}
				Side.InitialHealth = Side.Health();
			};
			SpawnSide(Left, LeftTypes, LeftFaction, Sign * FVector(bChase ? -1800 : -1200, bChase ? 3000 : 0, 0), -350 * Sign);
			SpawnSide(Right, RightTypes, RightFaction, Sign * FVector(bChase ? -850 : 1200, bChase ? 3000 : 0, 0), 350 * Sign);
			FStrategyOrder Attack;
			Attack.Type = bChase ? EStrategyOrderType::AttackTarget : EStrategyOrderType::AttackMove;
			Attack.TargetActor = bChase ? Right.Squads[0]->GetMembers()[0].Get() : nullptr;
			Attack.Destination = FVector::ZeroVector;
			Left.Order(Attack);
			FStrategyOrder Other;
			Other.Type = bChase ? EStrategyOrderType::Move : EStrategyOrderType::AttackMove;
			Other.Destination = bChase ? Sign * FVector(5000, 3000, 0) : FVector::ZeroVector;
			Right.Order(Other);
			StartTime = World->GetTimeSeconds();
			bStarted = true;
			return false;
		}
		const float Elapsed = World->GetTimeSeconds() - StartTime;
		if (FirstHit < 0.0f && Right.Health() < Right.InitialHealth) { FirstHit = Elapsed; }
		if (Left.Members() > 0 && Right.Members() > 0 && Elapsed < (Scenario >= 2 ? 10.0f : 30.0f))
		{
			return false;
		}
		const TCHAR* Names[] = {TEXT("InfantryVsCavalry"), TEXT("SupportedArchers"), TEXT("CavalryChase"), TEXT("InfantryChase")};
		const FString Row = FString::Printf(TEXT("%s,%s,%d,%.2f,%.0f,%.0f,%d,%d,%d,%d,%.1f,%.1f,%.2f\n"),
			*Map, Names[Scenario], bMirrored, Elapsed, Left.Gold, Right.Gold, Left.Population, Right.Population,
			Left.Members(), Right.Members(), Left.Health(), Right.Health(), FirstHit);
		const FString Directory = FPaths::ProjectSavedDir() / TEXT("Verification/M3-Battles");
		IFileManager::Get().MakeDirectory(*Directory, true);
		Test->TestTrue(TEXT("保存实际战斗结果"), FFileHelper::SaveStringToFile(Row, *(Directory / TEXT("results.csv")),
			FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM, &IFileManager::Get(), FILEWRITE_Append));
		Test->AddInfo(Row.TrimEnd());
		return true;
	}

private:
	FAutomationTestBase* Test;
	FString Map;
	int32 Scenario;
	bool bMirrored;
	bool bStarted = false;
	float StartTime = 0.0f;
	float FirstHit = -1.0f;
	FSide Left, Right;
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStrategyBattleProbeTest, "RTS.Balance.BattleProbe",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FStrategyBattleProbeTest::RunTest(const FString& Parameters)
{
	const FString Directory = FPaths::ProjectSavedDir() / TEXT("Verification/M3-Battles");
	IFileManager::Get().MakeDirectory(*Directory, true);
	FFileHelper::SaveStringToFile(TEXT("Map,Scenario,Mirrored,Seconds,GoldA,GoldB,PopulationA,PopulationB,SurvivorsA,SurvivorsB,HealthA,HealthB,FirstHitSeconds\n"),
		*(Directory / TEXT("results.csv")), FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
	for (const FString Map : {TEXT("LVL_CityStateSkirmish"), TEXT("LVL_RiverValleySkirmish")})
	{
		ADD_LATENT_AUTOMATION_COMMAND(FEditorLoadMap(TEXT("/Game/CityStateRTS/Maps/") + Map));
		ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
		ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(3.0f));
		for (int32 Scenario = 0; Scenario < 4; ++Scenario)
		{
			for (bool bMirrored : {false, true})
			{
				ADD_LATENT_AUTOMATION_COMMAND(StrategyBattleProbe::FRunBattle(this, Map, Scenario, bMirrored));
			}
		}
		ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	}
	return true;
}

#endif

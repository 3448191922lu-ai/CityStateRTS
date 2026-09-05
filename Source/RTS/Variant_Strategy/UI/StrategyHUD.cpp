// Copyright Epic Games, Inc. All Rights Reserved.


#include "StrategyHUD.h"
#include "StrategyUnit.h"
#include "StrategyPlayerController.h"
#include "StrategyUI.h"
#include "Engine/Canvas.h"
#include "StrategyGameState.h"
#include "StrategyWorldActors.h"

void AStrategyHUD::BeginPlay()
{
	Super::BeginPlay();

	// spawn the UI widget
	if (UIWidgetClass)
	{
		UIWidget = CreateWidget<UStrategyUI>(GetOwningPlayerController(), UIWidgetClass);
		if (UIWidget)
		{
			UIWidget->AddToViewport(0);
		}
	}
}

void AStrategyHUD::DragSelectUpdate(FVector2D Start, FVector2D WidthAndHeight, FVector2D CurrentPosition, bool bDraw)
{
	// copy the selection box data
	bDrawBox = bDraw;
	BoxStart = Start;
	BoxSize = WidthAndHeight;
	BoxCurrentPosition = CurrentPosition;

}

void AStrategyHUD::DrawHUD()
{
	// draw all debug information, etc.
	Super::DrawHUD();

	// ensure we have a valid player controller
	if (AStrategyPlayerController* PC = Cast<AStrategyPlayerController>(GetOwningPlayerController()))
	{
		if (const AStrategyGameState* State = GetWorld()->GetGameState<AStrategyGameState>())
		{
			const FStrategyFactionState& Faction = State->GetFactionState(EStrategyFaction::Player);
			DrawRect(FLinearColor(0.015f, 0.02f, 0.04f, 0.88f), 20.0f, 20.0f, 520.0f, 54.0f);
			DrawText(FString::Printf(TEXT("GOLD  %.0f     POP  %d+%d/%d     POINTS  %d"), Faction.Gold, Faction.UsedPopulation, Faction.ReservedPopulation, Faction.PopulationCap, Faction.OwnedPoints), FColor::White, 38.0f, 36.0f, nullptr, 1.15f);

			const float BottomY = Canvas ? Canvas->SizeY - 112.0f : 600.0f;
			DrawRect(FLinearColor(0.015f, 0.02f, 0.04f, 0.88f), 20.0f, BottomY, Canvas ? Canvas->SizeX - 40.0f : 1200.0f, 92.0f);
			FString Context = TEXT("WASD Camera   LMB Select/Box   RMB Move/Attack   F Attack-Move   X Stop   B Build   Wheel Zoom   Esc Pause");
			if (PC->IsBuildMenuOpen())
			{
				Context = PC->IsWallPlacementActive()
					? TEXT("PLACE WALL: Hold LMB and drag   Green valid / Red invalid   B Cancel")
					: PC->IsBuildingPlacementActive()
						? TEXT("PLACE BUILDING: Left click valid territory   B Cancel")
						: TEXT("BUILD: [1] Barracks [2] Archery [3] Stable [4] House [5] Tower [6] Wall [7] Upgrade selected wall to Gate");
			}
			else if (AStrategyBuilding* Building = PC->GetSelectedBuilding())
			{
				Context = FString::Printf(TEXT("BUILDING  HP %.0f%%  QUEUE %d/5   TRAIN: [1] Infantry [2] Archer [3] Cavalry"), Building->GetHealthPercent() * 100.0f, Building->GetQueueLength());
			}
			else if (!PC->GetSelectedSquads().IsEmpty())
			{
				Context = FString::Printf(TEXT("%d SQUAD(S) SELECTED   RMB Move/Target   F then RMB Attack-Move   X Stop"), PC->GetSelectedSquads().Num());
			}
			DrawText(Context, FColor::White, 38.0f, BottomY + 30.0f, nullptr, 1.0f);

			if (!State->IsMatchRunning())
			{
				const FString Result = State->GetWinner() == EStrategyFaction::Player ? TEXT("VICTORY") : TEXT("DEFEAT");
				DrawRect(FLinearColor(0.0f, 0.0f, 0.0f, 0.82f), Canvas->SizeX * 0.5f - 240.0f, Canvas->SizeY * 0.5f - 80.0f, 480.0f, 160.0f);
				DrawText(Result, State->GetWinner() == EStrategyFaction::Player ? FColor::Green : FColor::Red, Canvas->SizeX * 0.5f - 80.0f, Canvas->SizeY * 0.5f - 45.0f, nullptr, 2.0f);
				DrawText(TEXT("R Restart     Q Quit"), FColor::White, Canvas->SizeX * 0.5f - 105.0f, Canvas->SizeY * 0.5f + 22.0f, nullptr, 1.1f);
			}
		}

		// draw the selection box
		if (bDrawBox)
		{
			DrawRect(SelectionBoxColor, BoxStart.X, BoxStart.Y, BoxSize.X, BoxSize.Y);

			// get all the units in the selection box
			TArray<AStrategyUnit*> BoxedUnits;
			GetActorsInSelectionRectangle(BoxStart, BoxCurrentPosition, BoxedUnits, true);

			// update the unit selection on the player controller
			PC->DragSelectUnits(BoxedUnits);
		}

		// get the currently selected units
		TArray<AStrategyUnit*> SelectedUnits = PC->GetSelectedUnits();

		// update the selection count on the UI widget
		if (UIWidget)
		{
			UIWidget->SetSelectedUnitsCount(PC->GetSelectedSquads().Num());
		}

		// process each selected unit
		for (AStrategyUnit* CurrentUnit : SelectedUnits)
		{
			if (IsValid(CurrentUnit))
			{
				// project the unit's location to screen coordinates
				FVector2D ScreenCoords;

				if (PC->ProjectWorldLocationToScreen(CurrentUnit->GetActorLocation(), ScreenCoords, true))
				{
					// draw a selection string near the unit
					DrawText(TEXT("Selected"), FColor::White, ScreenCoords.X - 25.0f, ScreenCoords.Y + 25.0f, nullptr, 1.0f);
				}
			}
			
		}
	}

}

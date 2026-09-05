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

void AStrategyHUD::DrawCircle(const FVector2D& Center, float Radius, const FLinearColor& Color, float Thickness)
{
	constexpr int32 SegmentCount = 16;
	FVector2D Previous = Center + FVector2D(Radius, 0.0f);
	for (int32 Index = 1; Index <= SegmentCount; ++Index)
	{
		const float Angle = UE_TWO_PI * static_cast<float>(Index) / SegmentCount;
		const FVector2D Current = Center + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * Radius;
		DrawLine(Previous.X, Previous.Y, Current.X, Current.Y, Color, Thickness);
		Previous = Current;
	}
}

void AStrategyHUD::DrawSquadMarker(const FVector2D& Center, float Diameter, EStrategyUnitType UnitType,
	float HealthPercent, bool bHighlighted)
{
	const float Radius = Diameter * 0.5f;
	const FLinearColor Background(0.03f, 0.20f, 0.48f, 0.68f);
	const FLinearColor Border = bHighlighted
		? FLinearColor(0.45f, 0.95f, 1.0f, 1.0f)
		: FLinearColor(0.30f, 0.68f, 0.95f, 0.95f);
	const FLinearColor Symbol(0.88f, 0.96f, 1.0f, 1.0f);

	// 使用水平线填充圆形底板，避免引入纹理资源。
	for (float OffsetY = -Radius + 1.0f; OffsetY < Radius; OffsetY += 2.0f)
	{
		const float HalfWidth = FMath::Sqrt(Radius * Radius - OffsetY * OffsetY);
		DrawLine(Center.X - HalfWidth, Center.Y + OffsetY, Center.X + HalfWidth, Center.Y + OffsetY, Background, 2.0f);
	}
	DrawCircle(Center, Radius, Border, bHighlighted ? 2.5f : 1.5f);

	switch (UnitType)
	{
	case EStrategyUnitType::Infantry:
		DrawRect(FLinearColor(0.22f, 0.48f, 0.72f, 0.95f), Center.X - 6.0f, Center.Y - 7.0f, 12.0f, 11.0f);
		DrawLine(Center.X - 6.0f, Center.Y + 4.0f, Center.X, Center.Y + 9.0f, Symbol, 2.0f);
		DrawLine(Center.X, Center.Y + 9.0f, Center.X + 6.0f, Center.Y + 4.0f, Symbol, 2.0f);
		DrawLine(Center.X - 6.0f, Center.Y - 7.0f, Center.X + 6.0f, Center.Y - 7.0f, Symbol, 2.0f);
		DrawLine(Center.X - 6.0f, Center.Y - 7.0f, Center.X - 6.0f, Center.Y + 4.0f, Symbol, 2.0f);
		DrawLine(Center.X + 6.0f, Center.Y - 7.0f, Center.X + 6.0f, Center.Y + 4.0f, Symbol, 2.0f);
		break;

	case EStrategyUnitType::Archer:
	{
		FVector2D Previous(Center.X, Center.Y - 9.0f);
		for (int32 Index = 1; Index <= 8; ++Index)
		{
			const float Angle = -UE_HALF_PI + UE_PI * static_cast<float>(Index) / 8.0f;
			const FVector2D Current(Center.X - FMath::Cos(Angle) * 8.0f, Center.Y + FMath::Sin(Angle) * 9.0f);
			DrawLine(Previous.X, Previous.Y, Current.X, Current.Y, Symbol, 2.0f);
			Previous = Current;
		}
		DrawLine(Center.X, Center.Y - 9.0f, Center.X, Center.Y + 9.0f, Symbol, 1.5f);
		break;
	}

	case EStrategyUnitType::Cavalry:
		DrawLine(Center.X - 9.0f, Center.Y - 6.0f, Center.X, Center.Y + 1.0f, Symbol, 2.0f);
		DrawLine(Center.X, Center.Y + 1.0f, Center.X + 9.0f, Center.Y - 6.0f, Symbol, 2.0f);
		DrawLine(Center.X - 9.0f, Center.Y, Center.X, Center.Y + 7.0f, Symbol, 2.0f);
		DrawLine(Center.X, Center.Y + 7.0f, Center.X + 9.0f, Center.Y, Symbol, 2.0f);
		break;
	}

	const float HealthBarY = Center.Y + Radius + 4.0f;
	DrawRect(FLinearColor(0.03f, 0.03f, 0.04f, 0.95f), Center.X - 15.0f, HealthBarY, 30.0f, 4.0f);
	DrawRect(FLinearColor(0.12f, 0.90f, 0.28f, 1.0f), Center.X - 15.0f, HealthBarY, 30.0f * HealthPercent, 4.0f);
}

void AStrategyHUD::DrawDragArrow(const FVector2D& Start, const FVector2D& End, const FLinearColor& Color)
{
	const FVector2D Direction = (End - Start).GetSafeNormal();
	const FVector2D Perpendicular(-Direction.Y, Direction.X);
	const FVector2D WingA = End - Direction * 10.392f + Perpendicular * 6.0f;
	const FVector2D WingB = End - Direction * 10.392f - Perpendicular * 6.0f;
	DrawLine(Start.X, Start.Y, End.X, End.Y, Color, 3.0f);
	DrawLine(End.X, End.Y, WingA.X, WingA.Y, Color, 3.0f);
	DrawLine(End.X, End.Y, WingB.X, WingB.Y, Color, 3.0f);
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
			FVector2D MousePosition;
			PC->GetMousePosition(MousePosition.X, MousePosition.Y);
			AStrategySquad* HoveredSquad = PC->FindSquadMarkerAtScreenPosition(MousePosition);
			for (AStrategySquad* Squad : State->GetSquads())
			{
				FVector2D MarkerPosition;
				if (IsValid(Squad) && Squad->IsAlive() && Squad->GetFaction() == EStrategyFaction::Player
					&& PC->ProjectWorldLocationToScreen(Squad->GetMarkerWorldLocation(), MarkerPosition)
					&& MarkerPosition.X >= 0.0f && MarkerPosition.X <= Canvas->SizeX
					&& MarkerPosition.Y >= 0.0f && MarkerPosition.Y <= Canvas->SizeY)
				{
					const bool bHighlighted = Squad == HoveredSquad || PC->IsSquadSelected(Squad) || Squad == PC->GetSquadDragSource();
					DrawSquadMarker(MarkerPosition, bHighlighted ? 42.0f : 34.0f,
						Squad->GetUnitType(), Squad->GetHealthPercent(), bHighlighted);
				}
			}

			if (PC->IsSquadMarkerDragging())
			{
				AStrategySquad* Source = PC->GetSquadDragSource();
				FVector2D SourcePosition;
				const FLinearColor DragColor = PC->GetSquadDragTarget()
					? FLinearColor(0.95f, 0.12f, 0.08f, 1.0f)
					: FLinearColor(0.10f, 0.92f, 0.24f, 1.0f);
				if (IsValid(Source) && PC->ProjectWorldLocationToScreen(Source->GetMarkerWorldLocation(), SourcePosition))
				{
					DrawDragArrow(SourcePosition, MousePosition, DragColor);
				}

				const FVector DropWorldLocation = PC->GetSquadDragTarget()
					? PC->GetSquadDragTarget()->GetActorLocation()
					: PC->GetSquadDragDestination();
				FVector2D DropPosition;
				if (PC->ProjectWorldLocationToScreen(DropWorldLocation, DropPosition))
				{
					DrawCircle(DropPosition, 18.0f, DragColor, 3.0f);
				}
			}

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
			if (!PC->IsBuildMenuOpen())
			{
				Context += TEXT("   Drag squad badge to move/attack");
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

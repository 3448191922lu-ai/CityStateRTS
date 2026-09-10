// Copyright Epic Games, Inc. All Rights Reserved.


#include "StrategyHUD.h"
#include "StrategyUnit.h"
#include "StrategyPlayerController.h"
#include "StrategyUI.h"
#include "Engine/Canvas.h"
#include "StrategyGameState.h"
#include "StrategyArtStyle.h"
#include "StrategySystems.h"
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
	const FLinearColor HealthColor = HealthPercent > 0.5f ? FLinearColor(0.18f, 0.72f, 0.35f, 1.0f)
		: HealthPercent > 0.25f ? FLinearColor(0.95f, 0.72f, 0.18f, 1.0f) : FLinearColor(0.88f, 0.18f, 0.16f, 1.0f);
	DrawRect(HealthColor, Center.X - 15.0f, HealthBarY, 30.0f * HealthPercent, 4.0f);
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

void AStrategyHUD::DrawTownSymbol(const FVector2D& Center, EStrategyTownSpecialization Specialization,
	const FLinearColor& Color)
{
	DrawCircle(Center, 13.0f, Color, 2.5f);
	switch (Specialization)
	{
	case EStrategyTownSpecialization::Trade:
		DrawCircle(Center, 7.0f, Color, 2.0f);
		DrawLine(Center.X, Center.Y - 5.0f, Center.X, Center.Y + 5.0f, Color, 2.0f);
		break;
	case EStrategyTownSpecialization::Recruitment:
		DrawLine(Center.X - 8.0f, Center.Y + 5.0f, Center.X, Center.Y - 3.0f, Color, 2.0f);
		DrawLine(Center.X, Center.Y - 3.0f, Center.X + 8.0f, Center.Y + 5.0f, Color, 2.0f);
		DrawLine(Center.X - 8.0f, Center.Y - 2.0f, Center.X, Center.Y - 10.0f, Color, 2.0f);
		DrawLine(Center.X, Center.Y - 10.0f, Center.X + 8.0f, Center.Y - 2.0f, Color, 2.0f);
		break;
	case EStrategyTownSpecialization::Fortress:
		DrawLine(Center.X - 8.0f, Center.Y - 7.0f, Center.X + 8.0f, Center.Y - 7.0f, Color, 2.0f);
		DrawLine(Center.X - 8.0f, Center.Y - 7.0f, Center.X - 6.0f, Center.Y + 5.0f, Color, 2.0f);
		DrawLine(Center.X + 8.0f, Center.Y - 7.0f, Center.X + 6.0f, Center.Y + 5.0f, Color, 2.0f);
		DrawLine(Center.X - 6.0f, Center.Y + 5.0f, Center.X, Center.Y + 10.0f, Color, 2.0f);
		DrawLine(Center.X, Center.Y + 10.0f, Center.X + 6.0f, Center.Y + 5.0f, Color, 2.0f);
		break;
	default:
		break;
	}
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
			const FLinearColor PlayerColor = StrategyArtStyle::GetFactionColor(EStrategyFaction::Player);
			const FLinearColor EnemyColor = StrategyArtStyle::GetFactionColor(EStrategyFaction::Enemy);
			const float IntroOpacity = FStrategyIntroPromptRules::GetOpacity(GetWorld()->GetTimeSeconds());
			if (State->IsMatchRunning() && IntroOpacity > 0.0f)
			{
				const float IntroWidth = 620.0f;
				const float IntroX = Canvas->SizeX * 0.5f - IntroWidth * 0.5f;
				DrawRect(FLinearColor(0.015f, 0.02f, 0.04f, 0.82f * IntroOpacity), IntroX, 92.0f, IntroWidth, 82.0f);
				DrawText(TEXT("占领城镇  发展军队  摧毁敌方主城"), FLinearColor(1.0f, 0.88f, 0.34f, IntroOpacity),
					IntroX + 74.0f, 120.0f, nullptr, 1.35f);
			}

			// 补给线只绘制玩家自己的据点网络，不推断敌方迷雾状态。
			const TArray<TObjectPtr<AStrategyControlPoint>>& Points = State->GetControlPoints();
			for (int32 LeftIndex = 0; LeftIndex < Points.Num(); ++LeftIndex)
			{
				const AStrategyControlPoint* Left = Points[LeftIndex];
				if (!IsValid(Left) || Left->GetStrategyFaction() != EStrategyFaction::Player)
				{
					continue;
				}
				for (int32 RightIndex = LeftIndex + 1; RightIndex < Points.Num(); ++RightIndex)
				{
					const AStrategyControlPoint* Right = Points[RightIndex];
					if (!IsValid(Right) || Right->GetStrategyFaction() != EStrategyFaction::Player
						|| FVector::DistSquared2D(Left->GetActorLocation(), Right->GetActorLocation()) > FMath::Square(FStrategySupplyRules::LinkDistance))
					{
						continue;
					}
					FVector2D LeftScreen;
					FVector2D RightScreen;
					if (PC->ProjectWorldLocationToScreen(Left->GetActorLocation() + FVector(0.0f, 0.0f, 80.0f), LeftScreen)
						&& PC->ProjectWorldLocationToScreen(Right->GetActorLocation() + FVector(0.0f, 0.0f, 80.0f), RightScreen))
					{
						const bool bLeftConnected = Left->IsCapital() || State->IsTownSupplyConnected(Left);
						const bool bRightConnected = Right->IsCapital() || State->IsTownSupplyConnected(Right);
						const FLinearColor LinkColor = bLeftConnected && bRightConnected
							? FLinearColor(PlayerColor.R, PlayerColor.G, PlayerColor.B, 0.45f)
							: FLinearColor(0.34f, 0.36f, 0.40f, 0.55f);
						DrawLine(LeftScreen.X, LeftScreen.Y, RightScreen.X, RightScreen.Y, LinkColor, 2.0f);
					}
				}
			}

			for (const AStrategyControlPoint* Point : Points)
			{
				if (!IsValid(Point) || Point->IsCapital())
				{
					continue;
				}
				const bool bVisible = State->IsVisibleToFaction(EStrategyFaction::Player, Point->GetActorLocation());
				if (!FStrategyTownVisibilityRules::CanShowPublicDetails(
					EStrategyFaction::Player, Point->GetStrategyFaction(), bVisible))
				{
					continue;
				}
				FVector2D IconPosition;
				if (!PC->ProjectWorldLocationToScreen(Point->GetActorLocation() + FVector(0.0f, 0.0f, 470.0f), IconPosition))
				{
					continue;
				}
				const FStrategyTownDevelopment& Development = Point->GetTownDevelopment();
				const FLinearColor PointColor = StrategyArtStyle::GetFactionColor(Point->GetStrategyFaction());
				DrawTownSymbol(IconPosition, FStrategyTownVisibilityRules::GetPublicSpecialization(
					EStrategyFaction::Player, Point->GetStrategyFaction(), bVisible,
					Development.Specialization, Development.State), PointColor);
				if (FStrategyTownVisibilityRules::CanShowLiveDetails(
					EStrategyFaction::Player, Point->GetStrategyFaction(), bVisible)
					&& (Development.State == EStrategyTownDevelopmentState::Building
						|| Development.State == EStrategyTownDevelopmentState::Downgrading
						|| Development.State == EStrategyTownDevelopmentState::DisabledAfterCapture))
				{
					DrawRect(FLinearColor(0.02f, 0.02f, 0.03f, 0.92f), IconPosition.X - 23.0f, IconPosition.Y + 17.0f, 46.0f, 6.0f);
					const FLinearColor DevelopmentColor = Development.State == EStrategyTownDevelopmentState::Downgrading
						? FLinearColor(0.92f, 0.42f, 0.16f, 1.0f) : FLinearColor(0.84f, 0.65f, 0.26f, 1.0f);
					DrawRect(DevelopmentColor, IconPosition.X - 21.0f, IconPosition.Y + 19.0f,
						42.0f * Point->GetDevelopmentProgress(), 2.0f);
				}
				if (Point->GetStrategyFaction() == EStrategyFaction::Player
					&& Development.Specialization != EStrategyTownSpecialization::None
					&& !State->IsTownSupplyConnected(Point))
				{
					const FLinearColor Broken(0.55f, 0.57f, 0.62f, 1.0f);
					DrawLine(IconPosition.X - 20.0f, IconPosition.Y - 5.0f, IconPosition.X - 12.0f, IconPosition.Y + 3.0f, Broken, 3.0f);
					DrawLine(IconPosition.X - 12.0f, IconPosition.Y - 5.0f, IconPosition.X - 20.0f, IconPosition.Y + 3.0f, Broken, 3.0f);
				}
			}

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

				const FVector DropWorldLocation = PC->GetSquadDragGarrisonPoint()
					? PC->GetSquadDragGarrisonPoint()->GetActorLocation()
					: PC->GetSquadDragTarget()
					? PC->GetSquadDragTarget()->GetActorLocation()
					: PC->GetSquadDragDestination();
				FVector2D DropPosition;
				if (PC->ProjectWorldLocationToScreen(DropWorldLocation, DropPosition))
				{
					DrawCircle(DropPosition, 18.0f, DragColor, 3.0f);
					if (PC->GetSquadDragTarget())
					{
						DrawLine(DropPosition.X - 8.0f, DropPosition.Y - 8.0f, DropPosition.X + 8.0f, DropPosition.Y + 8.0f, DragColor, 3.0f);
						DrawLine(DropPosition.X + 8.0f, DropPosition.Y - 8.0f, DropPosition.X - 8.0f, DropPosition.Y + 8.0f, DragColor, 3.0f);
					}
				}
			}

			// 建造进度只作屏幕投影显示，不参与任何鼠标命中。
			for (AStrategyBuilding* Building : State->GetBuildings())
			{
				FVector2D BarPosition;
				if (IsValid(Building) && !Building->IsConstructionComplete()
					&& State->IsVisibleToFaction(EStrategyFaction::Player, Building->GetActorLocation())
					&& PC->ProjectWorldLocationToScreen(Building->GetActorLocation() + FVector(0.0f, 0.0f, 360.0f), BarPosition))
				{
					const FLinearColor ProgressColor = Building->GetStrategyFaction() == EStrategyFaction::Player
						? FLinearColor(0.84f, 0.65f, 0.26f, 1.0f) : EnemyColor;
					DrawRect(FLinearColor(0.015f, 0.02f, 0.04f, 0.92f), BarPosition.X - 27.0f, BarPosition.Y, 54.0f, 7.0f);
					DrawRect(ProgressColor, BarPosition.X - 25.0f, BarPosition.Y + 2.0f,
						50.0f * Building->GetConstructionProgress(), 3.0f);
				}
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

		// update the selection count on the UI widget
		if (UIWidget)
		{
			UIWidget->SetSelectedUnitsCount(PC->GetSelectedSquads().Num());
		}

	}

}

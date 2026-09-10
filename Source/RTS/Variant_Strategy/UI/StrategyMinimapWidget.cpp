#include "StrategyMinimapWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Framework/Application/SlateApplication.h"
#include "Input/Reply.h"
#include "InputCoreTypes.h"
#include "Rendering/DrawElements.h"
#include "StrategyGameState.h"
#include "StrategyGameUserSettings.h"
#include "StrategyMinimapModel.h"
#include "StrategyPawn.h"
#include "StrategyPlayerController.h"
#include "StrategyUnit.h"
#include "StrategyWorldActors.h"
#include "Styling/CoreStyle.h"

namespace
{
	const FLinearColor BackgroundColor(0.025f, 0.055f, 0.07f, 0.98f);
	const FLinearColor GroundColor(0.16f, 0.28f, 0.20f, 1.0f);
	const FLinearColor WaterColor(0.06f, 0.26f, 0.47f, 1.0f);
	const FLinearColor RouteColor(0.46f, 0.34f, 0.19f, 1.0f);
	const FLinearColor PlayerColor(0.18f, 0.72f, 1.0f, 1.0f);
	const FLinearColor EnemyColor(0.92f, 0.20f, 0.16f, 1.0f);
	const FLinearColor NeutralColor(0.82f, 0.68f, 0.30f, 1.0f);
	const FLinearColor UnknownColor(0.48f, 0.52f, 0.50f, 1.0f);

	FLinearColor FactionColor(EStrategyFaction Faction)
	{
		return Faction == EStrategyFaction::Player ? PlayerColor
			: Faction == EStrategyFaction::Enemy ? EnemyColor : NeutralColor;
	}

	void DrawBox(FSlateWindowElementList& Elements, int32 Layer, const FGeometry& Geometry,
		const FVector2D& Position, const FVector2D& Size, const FLinearColor& Color)
	{
		FSlateDrawElement::MakeBox(Elements, Layer,
			Geometry.ToPaintGeometry(Size, FSlateLayoutTransform(Position)),
			FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")), ESlateDrawEffect::None, Color);
	}

	void DrawClosedLines(FSlateWindowElementList& Elements, int32 Layer, const FGeometry& Geometry,
		const TArray<FVector2D>& Positions, const FLinearColor& Color, float Thickness)
	{
		TArray<FVector2f> Points;
		for (const FVector2D& Position : Positions)
		{
			Points.Add(FVector2f(Position));
		}
		Points.Add(FVector2f(Positions[0]));
		FSlateDrawElement::MakeLines(Elements, Layer, Geometry.ToPaintGeometry(), Points,
			ESlateDrawEffect::None, Color, true, Thickness);
	}

	void DrawQuad(FSlateWindowElementList& Elements, int32 Layer, const FGeometry& Geometry,
		const FSlateBrush& Brush, const TArray<FVector2D>& Positions,
		const TArray<FVector2D>& UVs, const FLinearColor& Color)
	{
		const FSlateResourceHandle Resource = FSlateApplication::Get().GetRenderer()->GetResourceHandle(Brush);
		TArray<FSlateVertex> Vertices;
		for (int32 Index = 0; Index < 4; ++Index)
		{
			Vertices.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(
				Geometry.GetAccumulatedRenderTransform(), FVector2f(Positions[Index]),
				FVector2f(UVs[Index]), Color.ToFColor(true)));
		}
		const TArray<SlateIndex> Indices = {0, 1, 2, 0, 2, 3};
		FSlateDrawElement::MakeCustomVerts(Elements, Layer, Resource, Vertices, Indices,
			nullptr, 0, 0, ESlateDrawEffect::None);
	}

	TArray<FVector2D> BoxCorners(const FBox2D& Box)
	{
		return {
			FVector2D(Box.Min.X, Box.Min.Y), FVector2D(Box.Max.X, Box.Min.Y),
			FVector2D(Box.Max.X, Box.Max.Y), FVector2D(Box.Min.X, Box.Max.Y)};
	}
}

void UStrategyMinimapWidget::InitializeForController(AStrategyPlayerController* InController)
{
	Controller = InController;
	MapDefinition = FStrategyMapDefinitions::Resolve(Controller->GetWorld()->GetMapName());
	InitialCameraYaw = CastChecked<AStrategyPawn>(Controller->GetPawn())->GetCameraYaw();
}

TSharedRef<SWidget> UStrategyMinimapWidget::RebuildWidget()
{
	UBorder* Root = WidgetTree->ConstructWidget<UBorder>();
	Root->SetBrushColor(FLinearColor::Transparent);
	WidgetTree->RootWidget = Root;
	return Super::RebuildWidget();
}

void UStrategyMinimapWidget::NativeTick(const FGeometry& Geometry, float DeltaSeconds)
{
	Super::NativeTick(Geometry, DeltaSeconds);
	if (Controller->IsPauseMenuOpen() && bDraggingCamera)
	{
		bDraggingCamera = false;
		FSlateApplication::Get().ReleaseAllPointerCapture();
	}
	RefreshIcons(Geometry.GetLocalSize());
	Invalidate(EInvalidateWidgetReason::Paint);
}

FVector2D UStrategyMinimapWidget::WorldToLocal(const FVector& World, const FVector2D& LocalSize) const
{
	const AStrategyPawn* Pawn = CastChecked<AStrategyPawn>(Controller->GetPawn());
	return FStrategyMinimapProjection::WorldToLocal(FVector2D(World.X, World.Y),
		MapDefinition.FogMin, MapDefinition.FogMax, LocalSize,
		UStrategyGameUserSettings::Get()->GetMinimapOrientation(), Pawn->GetCameraYaw(), InitialCameraYaw);
}

FVector2D UStrategyMinimapWidget::LocalToWorld(const FVector2D& Local, const FVector2D& LocalSize) const
{
	const AStrategyPawn* Pawn = CastChecked<AStrategyPawn>(Controller->GetPawn());
	FVector2D World = FStrategyMinimapProjection::LocalToWorld(Local,
		MapDefinition.FogMin, MapDefinition.FogMax, LocalSize,
		UStrategyGameUserSettings::Get()->GetMinimapOrientation(), Pawn->GetCameraYaw(), InitialCameraYaw);
	World.X = FMath::Clamp(World.X, MapDefinition.FogMin.X, MapDefinition.FogMax.X);
	World.Y = FMath::Clamp(World.Y, MapDefinition.FogMin.Y, MapDefinition.FogMax.Y);
	return World;
}

void UStrategyMinimapWidget::RefreshIcons(const FVector2D& LocalSize)
{
	Icons.Reset();
	AStrategyGameState* State = Controller->GetWorld()->GetGameState<AStrategyGameState>();
	for (AStrategyControlPoint* Point : State->GetControlPoints())
	{
		const bool bVisible = State->IsVisibleToFaction(EStrategyFaction::Player, Point->GetActorLocation());
		const bool bExplored = State->IsExploredToFaction(EStrategyFaction::Player, Point->GetActorLocation());
		const EStrategyMinimapEntityKind Kind = Point->IsCapital()
			? EStrategyMinimapEntityKind::Capital : EStrategyMinimapEntityKind::Town;
		if (!FStrategyMinimapVisibilityRules::ShouldDraw(
			Kind, Point->GetStrategyFaction(), bVisible, bExplored))
		{
			continue;
		}
		if (bVisible || Point->GetStrategyFaction() == EStrategyFaction::Player)
		{
			ObservedTownOwners.FindOrAdd(Point) = Point->GetStrategyFaction();
		}
		const EStrategyFaction* ObservedOwner = ObservedTownOwners.Find(Point);
		FStrategyMinimapIcon& Icon = Icons.AddDefaulted_GetRef();
		Icon.LocalPosition = WorldToLocal(Point->GetActorLocation(), LocalSize);
		Icon.Color = ObservedOwner ? FactionColor(*ObservedOwner)
			: Point->GetStrategyFaction() == EStrategyFaction::Neutral ? NeutralColor : UnknownColor;
		Icon.Radius = Point->IsCapital() ? 7.0f : 6.0f;
		if (Point->IsCapital() && Point->GetStrategyFaction() == EStrategyFaction::Enemy && bVisible)
		{
			Icon.TargetActor = Point;
			Icon.bEnemyTarget = Point->IsStrategyAlive();
		}
	}

	for (AStrategyBuilding* Building : State->GetBuildings())
	{
		if (!IsValid(Building) || !Building->IsStrategyAlive())
		{
			continue;
		}
		const bool bVisible = State->IsVisibleToFaction(EStrategyFaction::Player, Building->GetActorLocation());
		if (!FStrategyMinimapVisibilityRules::ShouldDraw(EStrategyMinimapEntityKind::Building,
			Building->GetStrategyFaction(), bVisible, bVisible))
		{
			continue;
		}
		FStrategyMinimapIcon& Icon = Icons.AddDefaulted_GetRef();
		Icon.LocalPosition = WorldToLocal(Building->GetActorLocation(), LocalSize);
		Icon.Color = FactionColor(Building->GetStrategyFaction());
		Icon.Radius = Building->GetBuildingType() == EStrategyBuildingType::Wall ? 2.5f : 4.0f;
		Icon.TargetActor = Building;
		Icon.bEnemyTarget = Building->GetStrategyFaction() == EStrategyFaction::Enemy;
	}

	for (AStrategySquad* Squad : State->GetSquads())
	{
		if (!IsValid(Squad) || !Squad->IsAlive())
		{
			continue;
		}
		const FVector Location = Squad->IsGarrisoned() ? Squad->GetMarkerWorldLocation() : Squad->GetCenterLocation();
		const bool bVisible = State->IsVisibleToFaction(EStrategyFaction::Player, Location);
		if (!FStrategyMinimapVisibilityRules::ShouldDraw(EStrategyMinimapEntityKind::Squad,
			Squad->GetFaction(), bVisible, bVisible))
		{
			continue;
		}
		FStrategyMinimapIcon& Icon = Icons.AddDefaulted_GetRef();
		Icon.LocalPosition = WorldToLocal(Location, LocalSize);
		Icon.Color = FactionColor(Squad->GetFaction());
		Icon.Radius = 4.5f;
		Icon.bGarrisoned = Squad->IsGarrisoned();
		if (Squad->GetFaction() == EStrategyFaction::Enemy)
		{
			for (AStrategyUnit* Unit : Squad->GetMembers())
			{
				if (IsValid(Unit))
				{
					Icon.TargetActor = Unit;
					Icon.bEnemyTarget = true;
					break;
				}
			}
		}
	}
}

int32 UStrategyMinimapWidget::NativePaint(const FPaintArgs& Args, const FGeometry& Geometry,
	const FSlateRect& CullingRect, FSlateWindowElementList& OutDrawElements,
	int32 LayerId, const FWidgetStyle& Style, bool bParentEnabled) const
{
	int32 Layer = Super::NativePaint(Args, Geometry, CullingRect, OutDrawElements, LayerId, Style, bParentEnabled);
	const FVector2D Size = Geometry.GetLocalSize();
	DrawBox(OutDrawElements, ++Layer, Geometry, FVector2D::ZeroVector, Size, BackgroundColor);

	const FSlateBrush* WhiteBrush = FCoreStyle::Get().GetBrush(TEXT("WhiteBrush"));
	const TArray<FVector2D> UVs = {
		FVector2D(0.0f, 0.0f), FVector2D(1.0f, 0.0f),
		FVector2D(1.0f, 1.0f), FVector2D(0.0f, 1.0f)};
	auto ProjectBox = [this, &Size](const FBox2D& Box)
	{
		TArray<FVector2D> Result;
		for (const FVector2D& Corner : BoxCorners(Box))
		{
			Result.Add(WorldToLocal(FVector(Corner.X, Corner.Y, 0.0f), Size));
		}
		return Result;
	};

	const FBox2D MapBox(MapDefinition.FogMin, MapDefinition.FogMax);
	const TArray<FVector2D> MapCorners = ProjectBox(MapBox);
	DrawQuad(OutDrawElements, ++Layer, Geometry, *WhiteBrush, MapCorners, UVs, GroundColor);
	for (const FBox2D& Water : MapDefinition.MinimapWaterAreas)
	{
		DrawQuad(OutDrawElements, Layer, Geometry, *WhiteBrush, ProjectBox(Water), UVs, WaterColor);
	}
	for (const FBox2D& Route : MapDefinition.MinimapRouteAreas)
	{
		DrawQuad(OutDrawElements, Layer, Geometry, *WhiteBrush, ProjectBox(Route), UVs, RouteColor);
	}

	if (UTexture2D* FogTexture = Controller->GetWorld()->GetGameState<AStrategyGameState>()->GetPlayerFogTexture())
	{
		FSlateBrush FogBrush;
		FogBrush.SetResourceObject(FogTexture);
		FogBrush.ImageSize = FVector2D(FogTexture->GetSizeX(), FogTexture->GetSizeY());
		DrawQuad(OutDrawElements, ++Layer, Geometry, FogBrush, MapCorners, UVs, FLinearColor::White);
	}

	for (const FStrategyMinimapIcon& Icon : Icons)
	{
		const FVector2D IconSize(Icon.Radius * 2.0f);
		DrawBox(OutDrawElements, ++Layer, Geometry, Icon.LocalPosition - IconSize * 0.5f, IconSize, Icon.Color);
		if (Icon.bGarrisoned)
		{
			DrawClosedLines(OutDrawElements, Layer, Geometry, {
				Icon.LocalPosition + FVector2D(-7.0f, -7.0f), Icon.LocalPosition + FVector2D(7.0f, -7.0f),
				Icon.LocalPosition + FVector2D(7.0f, 7.0f), Icon.LocalPosition + FVector2D(-7.0f, 7.0f)},
				FLinearColor::White, 1.5f);
		}
	}

	TArray<FVector2D> GroundCorners;
	Controller->GetCameraGroundCorners(GroundCorners);
	TArray<FVector2D> CameraFrame;
	for (const FVector2D& Corner : GroundCorners)
	{
		CameraFrame.Add(WorldToLocal(FVector(Corner.X, Corner.Y, 0.0f), Size));
	}
	DrawClosedLines(OutDrawElements, ++Layer, Geometry, CameraFrame, FLinearColor::White, 1.5f);

	if (UStrategyGameUserSettings::Get()->GetMinimapOrientation() == EStrategyMinimapOrientation::NorthUp)
	{
		FSlateDrawElement::MakeText(OutDrawElements, ++Layer,
			Geometry.ToPaintGeometry(FVector2D(18.0f, 20.0f), FSlateLayoutTransform(FVector2D(8.0f, 5.0f))),
			TEXT("N"), FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), 14),
			ESlateDrawEffect::None, FLinearColor::White);
	}
	DrawClosedLines(OutDrawElements, ++Layer, Geometry, {
		FVector2D(1.0f, 1.0f), FVector2D(Size.X - 1.0f, 1.0f),
		FVector2D(Size.X - 1.0f, Size.Y - 1.0f), FVector2D(1.0f, Size.Y - 1.0f)},
		FLinearColor(0.62f, 0.70f, 0.67f, 1.0f), 2.0f);
	return Layer;
}

void UStrategyMinimapWidget::MoveCameraAtPointer(const FGeometry& Geometry, const FPointerEvent& Event)
{
	Controller->MoveCameraFromMinimap(LocalToWorld(
		Geometry.AbsoluteToLocal(Event.GetScreenSpacePosition()), Geometry.GetLocalSize()));
}

AActor* UStrategyMinimapWidget::FindVisibleEnemyAt(const FVector2D& LocalPosition) const
{
	for (int32 Index = Icons.Num() - 1; Index >= 0; --Index)
	{
		const FStrategyMinimapIcon& Icon = Icons[Index];
		const float HitRadius = FMath::Max(8.0f, Icon.Radius + 3.0f);
		if (Icon.bEnemyTarget && FVector2D::DistSquared(LocalPosition, Icon.LocalPosition) <= FMath::Square(HitRadius))
		{
			return Icon.TargetActor.Get();
		}
	}
	return nullptr;
}

FReply UStrategyMinimapWidget::NativeOnMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& Event)
{
	if (Event.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		bDraggingCamera = true;
		MoveCameraAtPointer(Geometry, Event);
		return FReply::Handled().CaptureMouse(TakeWidget());
	}
	if (Event.GetEffectingButton() == EKeys::RightMouseButton)
	{
		const FVector2D Local = Geometry.AbsoluteToLocal(Event.GetScreenSpacePosition());
		Controller->IssueMinimapCommand(LocalToWorld(Local, Geometry.GetLocalSize()), FindVisibleEnemyAt(Local));
		return FReply::Handled();
	}
	return Super::NativeOnMouseButtonDown(Geometry, Event);
}

FReply UStrategyMinimapWidget::NativeOnMouseMove(const FGeometry& Geometry, const FPointerEvent& Event)
{
	if (bDraggingCamera)
	{
		MoveCameraAtPointer(Geometry, Event);
		return FReply::Handled();
	}
	return Super::NativeOnMouseMove(Geometry, Event);
}

FReply UStrategyMinimapWidget::NativeOnMouseButtonUp(const FGeometry& Geometry, const FPointerEvent& Event)
{
	if (Event.GetEffectingButton() == EKeys::LeftMouseButton && bDraggingCamera)
	{
		bDraggingCamera = false;
		return FReply::Handled().ReleaseMouseCapture();
	}
	return Super::NativeOnMouseButtonUp(Geometry, Event);
}

void UStrategyMinimapWidget::NativeOnMouseLeave(const FPointerEvent& Event)
{
	if (bDraggingCamera)
	{
		bDraggingCamera = false;
		FSlateApplication::Get().ReleaseAllPointerCapture(Event.GetUserIndex());
	}
	Super::NativeOnMouseLeave(Event);
}

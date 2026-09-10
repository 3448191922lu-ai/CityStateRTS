#include "StrategyMinimapModel.h"

namespace
{
	float GetMapRotation(EStrategyMinimapOrientation Orientation, float CameraYaw, float NorthYaw)
	{
		const float UpYaw = Orientation == EStrategyMinimapOrientation::FollowCamera ? CameraYaw : NorthYaw;
		return 90.0f - UpYaw;
	}

	FVector2D GetRotatedHalfExtent(const FVector2D& HalfExtent, float AngleDegrees)
	{
		const float Radians = FMath::DegreesToRadians(AngleDegrees);
		const float AbsCos = FMath::Abs(FMath::Cos(Radians));
		const float AbsSin = FMath::Abs(FMath::Sin(Radians));
		return FVector2D(
			AbsCos * HalfExtent.X + AbsSin * HalfExtent.Y,
			AbsSin * HalfExtent.X + AbsCos * HalfExtent.Y);
	}
}

FVector2D FStrategyMinimapProjection::WorldToLocal(const FVector2D& World, const FVector2D& MapMin,
	const FVector2D& MapMax, const FVector2D& LocalSize,
	EStrategyMinimapOrientation Orientation, float CameraYaw, float NorthYaw)
{
	const FVector2D HalfExtent = (MapMax - MapMin) * 0.5f;
	const FVector2D Center = (MapMin + MapMax) * 0.5f;
	const float Rotation = GetMapRotation(Orientation, CameraYaw, NorthYaw);
	const FVector2D Rotated = (World - Center).GetRotated(Rotation);
	const FVector2D RotatedHalfExtent = GetRotatedHalfExtent(HalfExtent, Rotation);
	return FVector2D(
		(Rotated.X / RotatedHalfExtent.X + 1.0f) * 0.5f * LocalSize.X,
		(1.0f - Rotated.Y / RotatedHalfExtent.Y) * 0.5f * LocalSize.Y);
}

FVector2D FStrategyMinimapProjection::LocalToWorld(const FVector2D& Local, const FVector2D& MapMin,
	const FVector2D& MapMax, const FVector2D& LocalSize,
	EStrategyMinimapOrientation Orientation, float CameraYaw, float NorthYaw)
{
	const FVector2D HalfExtent = (MapMax - MapMin) * 0.5f;
	const FVector2D Center = (MapMin + MapMax) * 0.5f;
	const float Rotation = GetMapRotation(Orientation, CameraYaw, NorthYaw);
	const FVector2D RotatedHalfExtent = GetRotatedHalfExtent(HalfExtent, Rotation);
	const FVector2D Rotated(
		(Local.X / LocalSize.X * 2.0f - 1.0f) * RotatedHalfExtent.X,
		(1.0f - Local.Y / LocalSize.Y * 2.0f) * RotatedHalfExtent.Y);
	return Center + Rotated.GetRotated(-Rotation);
}

bool FStrategyMinimapVisibilityRules::ShouldDraw(EStrategyMinimapEntityKind Kind,
	EStrategyFaction Faction, bool bVisible, bool bExplored)
{
	if (Faction == EStrategyFaction::Player)
	{
		return true;
	}
	if (Kind == EStrategyMinimapEntityKind::Town)
	{
		return Faction == EStrategyFaction::Neutral || bVisible || bExplored;
	}
	return Faction == EStrategyFaction::Enemy && bVisible;
}

EStrategyMinimapCommandIntent FStrategyMinimapCommandRules::ResolveIntent(
	bool bHasSelectedSquads, bool bHitVisibleEnemy)
{
	if (!bHasSelectedSquads)
	{
		return EStrategyMinimapCommandIntent::None;
	}
	return bHitVisibleEnemy ? EStrategyMinimapCommandIntent::Attack : EStrategyMinimapCommandIntent::Move;
}

FVector2D FStrategyMinimapLayoutRules::ResolveSize(float ViewportHeight)
{
	const float Alpha = FMath::Clamp((ViewportHeight - 720.0f) / 360.0f, 0.0f, 1.0f);
	return FMath::Lerp(FVector2D(250.0f, 200.0f), FVector2D(300.0f, 240.0f), Alpha);
}

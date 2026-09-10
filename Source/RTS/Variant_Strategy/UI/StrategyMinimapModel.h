#pragma once

#include "CoreMinimal.h"
#include "StrategyTypes.h"
#include "StrategyMinimapModel.generated.h"

UENUM()
enum class EStrategyMinimapOrientation : uint8
{
	NorthUp,
	FollowCamera
};

enum class EStrategyMinimapEntityKind : uint8
{
	Squad,
	Building,
	Capital,
	Town
};

enum class EStrategyMinimapCommandIntent : uint8
{
	None,
	Move,
	Attack
};

struct FStrategyMinimapProjection
{
	static FVector2D WorldToLocal(const FVector2D& World, const FVector2D& MapMin,
		const FVector2D& MapMax, const FVector2D& LocalSize,
		EStrategyMinimapOrientation Orientation, float CameraYaw, float NorthYaw);
	static FVector2D LocalToWorld(const FVector2D& Local, const FVector2D& MapMin,
		const FVector2D& MapMax, const FVector2D& LocalSize,
		EStrategyMinimapOrientation Orientation, float CameraYaw, float NorthYaw);
};

struct FStrategyMinimapVisibilityRules
{
	static bool ShouldDraw(EStrategyMinimapEntityKind Kind, EStrategyFaction Faction,
		bool bVisible, bool bExplored);
};

struct FStrategyMinimapCommandRules
{
	static EStrategyMinimapCommandIntent ResolveIntent(bool bHasSelectedSquads, bool bHitVisibleEnemy);
};

struct FStrategyMinimapLayoutRules
{
	static FVector2D ResolveSize(float ViewportHeight);
};

// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "StrategyGameMode.generated.h"

/**
 *  Simple GameMode for a top down strategy game.
 */
UCLASS()
class AStrategyGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AStrategyGameMode();
	virtual void BeginPlay() override;
};

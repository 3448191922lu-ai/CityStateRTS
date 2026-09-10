#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameUserSettings.h"
#include "StrategyMinimapModel.h"
#include "StrategyGameUserSettings.generated.h"

struct FStrategySettingsDraft
{
	float MasterVolume = 1.0f;
	EWindowMode::Type WindowMode = EWindowMode::WindowedFullscreen;
	FIntPoint Resolution = FIntPoint::ZeroValue;
	int32 OverallQuality = 3;
	EStrategyMinimapOrientation MinimapOrientation = EStrategyMinimapOrientation::NorthUp;
};

struct FStrategySettingsRules
{
	static float SnapMasterVolume(float Value);
	static int32 QualityIndexToLevel(int32 Index);
	static EWindowMode::Type WindowModeIndexToValue(int32 Index);
	static int32 WindowModeValueToIndex(EWindowMode::Type Value);
	static FString FormatResolution(const FIntPoint& Resolution);
	static bool ParseResolution(const FString& Text, FIntPoint& OutResolution);
	static int32 MinimapOrientationToIndex(EStrategyMinimapOrientation Value);
	static EStrategyMinimapOrientation MinimapOrientationFromIndex(int32 Index);
};

UCLASS(Config=GameUserSettings)
class UStrategyGameUserSettings : public UGameUserSettings
{
	GENERATED_BODY()

public:
	static UStrategyGameUserSettings* Get();
	virtual void SetToDefaults() override;
	virtual void ApplyNonResolutionSettings() override;
	float GetMasterVolume() const { return MasterVolume; }
	void SetMasterVolume(float Value);
	EStrategyMinimapOrientation GetMinimapOrientation() const { return MinimapOrientation; }
	void SetMinimapOrientation(EStrategyMinimapOrientation Value) { MinimapOrientation = Value; }
	FStrategySettingsDraft MakeDraft() const;
	void ApplyDraft(const FStrategySettingsDraft& Draft);

private:
	UPROPERTY(Config)
	float MasterVolume = 1.0f;

	UPROPERTY(Config)
	EStrategyMinimapOrientation MinimapOrientation = EStrategyMinimapOrientation::NorthUp;
};

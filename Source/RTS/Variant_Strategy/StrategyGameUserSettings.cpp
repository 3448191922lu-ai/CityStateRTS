#include "StrategyGameUserSettings.h"

#include "Engine/Engine.h"
#include "Misc/App.h"

float FStrategySettingsRules::SnapMasterVolume(float Value)
{
	return FMath::RoundToFloat(FMath::Clamp(Value, 0.0f, 1.0f) * 20.0f) / 20.0f;
}

int32 FStrategySettingsRules::QualityIndexToLevel(int32 Index)
{
	return FMath::Clamp(Index, 0, 3);
}

EWindowMode::Type FStrategySettingsRules::WindowModeIndexToValue(int32 Index)
{
	switch (Index)
	{
	case 0: return EWindowMode::Windowed;
	case 1: return EWindowMode::WindowedFullscreen;
	default: return EWindowMode::Fullscreen;
	}
}

int32 FStrategySettingsRules::WindowModeValueToIndex(EWindowMode::Type Value)
{
	switch (Value)
	{
	case EWindowMode::Windowed: return 0;
	case EWindowMode::WindowedFullscreen: return 1;
	default: return 2;
	}
}

FString FStrategySettingsRules::FormatResolution(const FIntPoint& Resolution)
{
	return FString::Printf(TEXT("%d x %d"), Resolution.X, Resolution.Y);
}

bool FStrategySettingsRules::ParseResolution(const FString& Text, FIntPoint& OutResolution)
{
	FString Width;
	FString Height;
	if (!Text.Split(TEXT(" x "), &Width, &Height))
	{
		return false;
	}
	OutResolution = FIntPoint(FCString::Atoi(*Width), FCString::Atoi(*Height));
	return OutResolution.X > 0 && OutResolution.Y > 0;
}

int32 FStrategySettingsRules::MinimapOrientationToIndex(EStrategyMinimapOrientation Value)
{
	return Value == EStrategyMinimapOrientation::FollowCamera ? 1 : 0;
}

EStrategyMinimapOrientation FStrategySettingsRules::MinimapOrientationFromIndex(int32 Index)
{
	return Index == 1 ? EStrategyMinimapOrientation::FollowCamera : EStrategyMinimapOrientation::NorthUp;
}

UStrategyGameUserSettings* UStrategyGameUserSettings::Get()
{
	return CastChecked<UStrategyGameUserSettings>(GEngine->GetGameUserSettings());
}

void UStrategyGameUserSettings::SetToDefaults()
{
	Super::SetToDefaults();
	SetScreenResolution(GetDesktopResolution());
	SetFullscreenMode(EWindowMode::WindowedFullscreen);
	SetOverallScalabilityLevel(3);
	MasterVolume = 1.0f;
	MinimapOrientation = EStrategyMinimapOrientation::NorthUp;
}

void UStrategyGameUserSettings::ApplyNonResolutionSettings()
{
	Super::ApplyNonResolutionSettings();
	FApp::SetVolumeMultiplier(MasterVolume);
}

void UStrategyGameUserSettings::SetMasterVolume(float Value)
{
	MasterVolume = FStrategySettingsRules::SnapMasterVolume(Value);
}

FStrategySettingsDraft UStrategyGameUserSettings::MakeDraft() const
{
	FStrategySettingsDraft Draft;
	Draft.MasterVolume = MasterVolume;
	Draft.WindowMode = GetFullscreenMode();
	Draft.Resolution = GetScreenResolution();
	Draft.OverallQuality = FMath::Clamp(GetOverallScalabilityLevel(), 0, 3);
	Draft.MinimapOrientation = MinimapOrientation;
	return Draft;
}

void UStrategyGameUserSettings::ApplyDraft(const FStrategySettingsDraft& Draft)
{
	const FIntPoint PreviousResolution = GetScreenResolution();
	const EWindowMode::Type PreviousWindowMode = GetFullscreenMode();

	SetMasterVolume(Draft.MasterVolume);
	SetScreenResolution(Draft.Resolution);
	SetFullscreenMode(Draft.WindowMode);
	SetOverallScalabilityLevel(FMath::Clamp(Draft.OverallQuality, 0, 3));
	SetMinimapOrientation(Draft.MinimapOrientation);
	ApplySettings(false);

	if (IsScreenResolutionDirty() || IsFullscreenModeDirty())
	{
		SetScreenResolution(PreviousResolution);
		SetFullscreenMode(PreviousWindowMode);
		ApplyResolutionSettings(false);
	}

	ConfirmVideoMode();
	SaveSettings();
}

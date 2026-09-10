#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "StrategyTypes.h"
#include "StrategyTownPanel.generated.h"

class AStrategyControlPoint;
class AStrategyPlayerController;
class UButton;
class UTextBlock;

UCLASS()
class UStrategyTownPanel : public UUserWidget
{
	GENERATED_BODY()

public:
	void InitializeForController(AStrategyPlayerController* InController);
	void ShowTown(AStrategyControlPoint* InTown);
	void HideTown();
	void Refresh();

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	UButton* AddButton(class UHorizontalBox* Parent, const FString& Label);

	UFUNCTION()
	void ChooseTrade();
	UFUNCTION()
	void ChooseRecruitment();
	UFUNCTION()
	void ChooseFortress();
	UFUNCTION()
	void Downgrade();

	UPROPERTY(Transient)
	TObjectPtr<AStrategyPlayerController> Controller;
	UPROPERTY(Transient)
	TObjectPtr<AStrategyControlPoint> Town;
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> TitleText;
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> DetailText;
	UPROPERTY(Transient)
	TObjectPtr<UButton> TradeButton;
	UPROPERTY(Transient)
	TObjectPtr<UButton> RecruitmentButton;
	UPROPERTY(Transient)
	TObjectPtr<UButton> FortressButton;
	UPROPERTY(Transient)
	TObjectPtr<UButton> DowngradeButton;
};

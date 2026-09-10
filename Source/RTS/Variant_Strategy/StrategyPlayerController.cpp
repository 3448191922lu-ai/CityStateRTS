// Copyright Epic Games, Inc. All Rights Reserved.


#include "StrategyPlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "EnhancedInputComponent.h"
#include "InputMappingContext.h"
#include "Camera/CameraComponent.h"
#include "StrategyPawn.h"
#include "Camera/CameraComponent.h"
#include "InputActionValue.h"
#include "StrategyHUD.h"
#include "Engine/CollisionProfile.h"
#include "Kismet/GameplayStatics.h"
#include "StrategyUnit.h"
#include "NavigationSystem.h"
#include "Engine/OverlapResult.h"
#include "InputAction.h"
#include "StrategyTouchControls.h"
#include "StrategyPauseMenu.h"
#include "StrategyHUDRoot.h"
#include "StrategyHUDModel.h"
#include "StrategyMinimapModel.h"
#include "Widgets/Input/SVirtualJoystick.h"
#include "RTS.h"
#include "InputCoreTypes.h"
#include "Kismet/KismetSystemLibrary.h"
#include "StrategyGameState.h"
#include "StrategySystems.h"
#include "StrategyWorldActors.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "NiagaraFunctionLibrary.h"
#include "Framework/Application/SlateApplication.h"

AStrategyPlayerController::AStrategyPlayerController()
{
	// mouse cursor should always be shown
	bShowMouseCursor = true;
}

void AStrategyPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (IsLocalPlayerController())
	{
		PauseMenu = CreateWidget<UStrategyPauseMenu>(this, UStrategyPauseMenu::StaticClass());
		PauseMenu->InitializeForController(this);
		PauseMenu->AddToPlayerScreen(100);
		PauseMenu->SetVisibility(ESlateVisibility::Collapsed);
		HUDRoot = CreateWidget<UStrategyHUDRoot>(this, UStrategyHUDRoot::StaticClass());
		HUDRoot->InitializeForController(this);
		HUDRoot->AddToPlayerScreen(20);

		FInputModeGameAndUI InputMode;
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		InputMode.SetHideCursorDuringCapture(false);
		SetInputMode(InputMode);
		bShowMouseCursor = true;
	}

	// only spawn touch controls on local player controllers
	if (IsLocalPlayerController() && ShouldUseTouchControls())
	{
		// spawn the mobile controls widget
		MobileControlsWidget = CreateWidget<UStrategyTouchControls>(this, MobileControlsWidgetClass);

		if (MobileControlsWidget)
		{
			// add the controls to the player screen
			MobileControlsWidget->AddToPlayerScreen(0);

			// set the PC pointer on the mobile controls widget
			MobileControlsWidget->SetPlayerController(this);

		} else {

			UE_LOG(LogRTS, Error, TEXT("Could not spawn mobile controls widget."));

		}

	}

}

void AStrategyPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// only set up input on local player controllers
	if (IsLocalPlayerController())
	{
		// add the input mapping context
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
		{
			// choose the context based on the input mode
			UInputMappingContext* ChosenContext = nullptr;

			if (ShouldUseTouchControls())
			{
				ChosenContext = TouchMappingContext;
			}
			else
			{
				ChosenContext = MouseMappingContext;
			}

			Subsystem->AddMappingContext(ChosenContext, 0);
		}

		// bind the input mappings
		if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent))
		{
			// 触摸端继续使用摇杆移动相机，桌面端由 WASD 固定控制屏幕方向。
			if (ShouldUseTouchControls())
			{
				EnhancedInputComponent->BindAction(MoveCameraAction, ETriggerEvent::Triggered, this, &AStrategyPlayerController::MoveCamera);
			}
			EnhancedInputComponent->BindAction(ZoomCameraAction, ETriggerEvent::Triggered, this, &AStrategyPlayerController::ZoomCamera);
			EnhancedInputComponent->BindAction(ResetCameraAction, ETriggerEvent::Triggered, this, &AStrategyPlayerController::ResetCamera);

			// Mouse Interaction
			EnhancedInputComponent->BindAction(SelectHoldAction, ETriggerEvent::Started, this, &AStrategyPlayerController::SelectHoldStarted);
			EnhancedInputComponent->BindAction(SelectHoldAction, ETriggerEvent::Triggered, this, &AStrategyPlayerController::SelectHoldTriggered);
			EnhancedInputComponent->BindAction(SelectHoldAction, ETriggerEvent::Completed, this, &AStrategyPlayerController::SelectHoldCompleted);
			EnhancedInputComponent->BindAction(SelectHoldAction, ETriggerEvent::Canceled, this, &AStrategyPlayerController::SelectHoldCompleted);

			EnhancedInputComponent->BindAction(SelectClickAction, ETriggerEvent::Completed, this, &AStrategyPlayerController::SelectClick);

			EnhancedInputComponent->BindAction(SelectClickAdditiveAction, ETriggerEvent::Completed, this, &AStrategyPlayerController::SelectClickAdditive);

			EnhancedInputComponent->BindAction(SelectAllDoubleClickAction, ETriggerEvent::Completed, this, &AStrategyPlayerController::SelectAllDoubleClick);

			EnhancedInputComponent->BindAction(InteractHoldAction, ETriggerEvent::Started, this, &AStrategyPlayerController::InteractHoldStarted);
			EnhancedInputComponent->BindAction(InteractHoldAction, ETriggerEvent::Triggered, this, &AStrategyPlayerController::InteractHoldTriggered);

			EnhancedInputComponent->BindAction(InteractClickAction, ETriggerEvent::Completed, this, &AStrategyPlayerController::InteractClick);

			// Touch Interaction
			EnhancedInputComponent->BindAction(TouchPrimaryHoldAction, ETriggerEvent::Started, this, &AStrategyPlayerController::TouchPrimaryHoldStarted);
			EnhancedInputComponent->BindAction(TouchPrimaryHoldAction, ETriggerEvent::Triggered, this, &AStrategyPlayerController::TouchPrimaryHoldTriggered);
			EnhancedInputComponent->BindAction(TouchPrimaryHoldAction, ETriggerEvent::Completed, this, &AStrategyPlayerController::TouchPrimaryHoldCompleted);

			EnhancedInputComponent->BindAction(TouchSecondaryAction, ETriggerEvent::Triggered, this, &AStrategyPlayerController::TouchSecondaryTriggered);
			EnhancedInputComponent->BindAction(TouchSecondaryAction, ETriggerEvent::Completed, this, &AStrategyPlayerController::TouchSecondaryCompleted);
			EnhancedInputComponent->BindAction(TouchSecondaryAction, ETriggerEvent::Canceled, this, &AStrategyPlayerController::TouchSecondaryCompleted);

		}

		InputComponent->BindKey(EKeys::F, IE_Pressed, this, &AStrategyPlayerController::HandleAttackMoveKey);
		InputComponent->BindKey(EKeys::X, IE_Pressed, this, &AStrategyPlayerController::HandleStopKey);
		InputComponent->BindKey(EKeys::B, IE_Pressed, this, &AStrategyPlayerController::HandleBuildMenuKey);
		InputComponent->BindKey(EKeys::One, IE_Pressed, this, &AStrategyPlayerController::HandleNumber1);
		InputComponent->BindKey(EKeys::Two, IE_Pressed, this, &AStrategyPlayerController::HandleNumber2);
		InputComponent->BindKey(EKeys::Three, IE_Pressed, this, &AStrategyPlayerController::HandleNumber3);
		InputComponent->BindKey(EKeys::Four, IE_Pressed, this, &AStrategyPlayerController::HandleNumber4);
		InputComponent->BindKey(EKeys::Five, IE_Pressed, this, &AStrategyPlayerController::HandleNumber5);
		InputComponent->BindKey(EKeys::Six, IE_Pressed, this, &AStrategyPlayerController::HandleNumber6);
		InputComponent->BindKey(EKeys::Seven, IE_Pressed, this, &AStrategyPlayerController::HandleNumber7);
		InputComponent->BindKey(EKeys::Escape, IE_Pressed, this, &AStrategyPlayerController::HandlePauseKey);
		InputComponent->BindKey(EKeys::R, IE_Pressed, this, &AStrategyPlayerController::HandleRestartKey);
		InputComponent->BindKey(EKeys::Q, IE_Pressed, this, &AStrategyPlayerController::HandleQuitKey);
	}
}

void AStrategyPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);
	UpdateMatchResultFeedback();
	if (bSelectionFeedbackPending)
	{
		if (const AStrategyGameState* State = GetWorld()->GetGameState<AStrategyGameState>())
		{
			if (const UStrategyPresentationDataAsset* Presentation = State->GetPresentationDefinition())
			{
				UGameplayStatics::PlaySound2D(this, Presentation->SelectSound);
			}
		}
		bSelectionFeedbackPending = false;
	}
	if (!ControlledCameraPawn)
	{
		return;
	}
	int32 Width = 0;
	int32 Height = 0;
	GetViewportSize(Width, Height);

	if (!ShouldUseTouchControls())
	{
		const float CameraRotationInput = (IsInputKeyDown(EKeys::C) ? 1.0f : 0.0f) - (IsInputKeyDown(EKeys::Z) ? 1.0f : 0.0f);
		if (!FMath::IsNearlyZero(CameraRotationInput) && Width > 0 && Height > 0)
		{
			ControlledCameraPawn->RotateCameraYaw(
				CameraRotationInput * 90.0f * DeltaTime, static_cast<float>(Width) / static_cast<float>(Height));
		}
	}

	if (!ShouldUseTouchControls())
	{
		const FVector2D KeyboardDirection = FStrategyCameraMovement::ResolveScreenDirection(
			IsInputKeyDown(EKeys::W), IsInputKeyDown(EKeys::S), IsInputKeyDown(EKeys::A), IsInputKeyDown(EKeys::D));
		if (!KeyboardDirection.IsNearlyZero())
		{
			ControlledCameraPawn->AddActorWorldOffset(
				FStrategyCameraMovement::ScreenToWorld(KeyboardDirection.GetSafeNormal(), ControlledCameraPawn->GetCameraYaw()) * 1400.0f * DeltaTime, true);
		}
	}

	float MouseX = 0.0f;
	float MouseY = 0.0f;
	if (!GetMousePosition(MouseX, MouseY) || Width <= 0 || Height <= 0)
	{
		if (Width > 0 && Height > 0)
		{
			ControlledCameraPawn->ClampToMapBounds(static_cast<float>(Width) / static_cast<float>(Height));
		}
		return;
	}

	FVector2D Direction = FVector2D::ZeroVector;
	Direction.X = MouseX < 16.0f ? -1.0f : MouseX > Width - 16.0f ? 1.0f : 0.0f;
	Direction.Y = MouseY < 16.0f ? 1.0f : MouseY > Height - 16.0f ? -1.0f : 0.0f;
	if (!Direction.IsNearlyZero())
	{
		ControlledCameraPawn->AddActorWorldOffset(
			FStrategyCameraMovement::ScreenToWorld(Direction.GetSafeNormal(), ControlledCameraPawn->GetCameraYaw()) * 1400.0f * DeltaTime, true);
	}
	ControlledCameraPawn->ClampToMapBounds(static_cast<float>(Width) / static_cast<float>(Height));
}

void AStrategyPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	// ensure we have the right pawn type
	ControlledCameraPawn = Cast<AStrategyPawn>(InPawn);
	check(ControlledCameraPawn);

	// set the zoom level from the pawn's camera
	DefaultZoom = CameraZoom = ControlledCameraPawn->GetCamera()->OrthoWidth;

	// cast the HUD pointer
	StrategyHUD = Cast<AStrategyHUD>(GetHUD());

	// if we have a touch controls widget, sync the camera zoom
	if (MobileControlsWidget)
	{
		MobileControlsWidget->BP_SetZoomPercentage(GetDefaultZoomPercentage());
	}
}

void AStrategyPlayerController::DragSelectUnits(const TArray<AStrategyUnit*>& Units)
{
	const TArray<TObjectPtr<AStrategySquad>> PreviousSquads = ControlledSquads;
	bSuppressSelectionFeedback = true;
	// do we have units in the list?
	if (Units.Num() > 0)
	{
		// ensure any previous units are deselected
		DoDeselectAllUnitsCommand();

		// 选择框命中成员后按小队去重。
		for (AStrategyUnit* CurrentUnit : Units)
		{
			if (IsValid(CurrentUnit) && CurrentUnit->GetStrategyFaction() == EStrategyFaction::Player && CurrentUnit->GetSquad())
			{
				SelectSquad(CurrentUnit->GetSquad(), false);
			}
		}

	}
	else
	{

		// release any currently selected units since nothing is on the box
		if (ControlledUnits.Num() > 0)
		{
			DoDeselectAllUnitsCommand();
		}

	}
	RefreshControlledUnits();
	bSuppressSelectionFeedback = false;
	if (!ControlledSquads.IsEmpty() && PreviousSquads != ControlledSquads)
	{
		bSelectionFeedbackPending = true;
	}
}

const TArray<AStrategyUnit*>& AStrategyPlayerController::GetSelectedUnits()
{
	RefreshControlledUnits();
	return ControlledUnits;
}

float AStrategyPlayerController::GetDefaultZoomPercentage() const
{
	float ZoomPct = (DefaultZoom - MinZoomLevel) / (MaxZoomLevel - MinZoomLevel);
	return FMath::Clamp(ZoomPct, 0.0f, 1.0f);
}

bool AStrategyPlayerController::ShouldUseTouchControls() const
{
	// are we on a mobile platform? Should we force touch?
	return SVirtualJoystick::ShouldDisplayTouchInterface() || bForceTouchControls;
}

void AStrategyPlayerController::MoveCamera(const FInputActionValue& Value)
{
	FVector2D InputVector = Value.Get<FVector2D>();

	// get the forward input component vector
	FRotator ForwardRot(0.0f, ControlledCameraPawn ? ControlledCameraPawn->GetCameraYaw() : -45.0f, 0.0f);

	// get the right input component vector
	FRotator RightRot = ForwardRot;

	// add the forward input
	if (ControlledCameraPawn)
	{
		ControlledCameraPawn->AddMovementInput(ForwardRot.RotateVector(FVector::ForwardVector), InputVector.X + InputVector.Y);

		// add the right input
		ControlledCameraPawn->AddMovementInput(RightRot.RotateVector(FVector::RightVector), InputVector.X - InputVector.Y);
	}
}

void AStrategyPlayerController::ZoomCamera(const FInputActionValue& Value)
{
	DoCameraModifyZoomCommand(Value.Get<float>() * ZoomScaling);
}

void AStrategyPlayerController::ResetCamera(const FInputActionValue& Value)
{
	DoCameraResetZoomCommand();
}

void AStrategyPlayerController::SelectHoldStarted(const FInputActionValue& Value)
{
	// 点击屏蔽只属于上一次按下，长拖拽可能没有对应的 Tap 回调。
	bConsumeNextSelectClick = false;
	if (bWallPlacementActive)
	{
		FVector CursorLocation;
		if (GetLocationUnderCursor(CursorLocation))
		{
			WallDragStart = CursorLocation;
			bWallDragging = true;
		}
		return;
	}
	if (!FStrategySquadMarkerRules::CanInteract(bBuildingPlacementActive, bWallPlacementActive))
	{
		return;
	}

	const FVector2D Cursor = GetMouseLocationForPlayer();
	SquadMarkerSource = FindSquadMarkerAtScreenPosition(Cursor);
	if (SquadMarkerSource)
	{
		SquadMarkerPressScreen = Cursor;
		bSquadMarkerInputActive = true;
		bConsumeNextSelectClick = true;
		return;
	}

	// save the box selection start position
	StartingBoxSelectionPosition = Cursor;

}

void AStrategyPlayerController::SelectHoldTriggered(const FInputActionValue& Value)
{
	if (bWallDragging)
	{
		FVector CursorLocation;
		if (GetLocationUnderCursor(CursorLocation))
		{
			UpdateWallPreview(CursorLocation);
		}
		return;
	}
	if (bWallPlacementActive || bBuildingPlacementActive)
	{
		return;
	}
	if (bSquadMarkerInputActive)
	{
		const FVector2D Cursor = GetMouseLocationForPlayer();
		bSquadMarkerDragging |= FVector2D::Distance(Cursor, SquadMarkerPressScreen) >= 6.0f;
		if (bSquadMarkerDragging)
		{
			UpdateSquadMarkerDragTarget();
		}
		return;
	}

	// get the current mouse position
	FVector2D SelectionPosition = GetMouseLocationForPlayer();

	// calculate the size of the selection box
	FVector2D SelectionSize = SelectionPosition - StartingBoxSelectionPosition;

	// update the selection box on the HUD
	if (StrategyHUD)
	{
		StrategyHUD->DragSelectUpdate(StartingBoxSelectionPosition, SelectionSize, SelectionPosition, true);
	}	
}

void AStrategyPlayerController::SelectHoldCompleted(const FInputActionValue& Value)
{
	if (bWallDragging)
	{
		FVector CursorLocation;
		if (GetLocationUnderCursor(CursorLocation))
		{
			if (GetWorld()->GetGameState<AStrategyGameState>()->TryPlaceWallLine(
				EStrategyFaction::Player, WallDragStart, CursorLocation) == 0)
			{
				PlayInvalidActionFeedback();
			}
		}
		bWallDragging = false;
		bWallPlacementActive = false;
		bBuildMenuOpen = false;
		ClearWallPreview();
		return;
	}
	if (bWallPlacementActive || bBuildingPlacementActive)
	{
		return;
	}
	if (bSquadMarkerInputActive)
	{
		if (!bSquadMarkerDragging)
		{
			const bool bAdditive = IsInputKeyDown(EKeys::LeftShift) || IsInputKeyDown(EKeys::RightShift);
			if (!bAdditive)
			{
				DoDeselectAllUnitsCommand();
			}
			SelectSquad(SquadMarkerSource, bAdditive);
		}
		else
		{
			if (!FStrategySquadMarkerRules::ShouldCommandSelectedSquads(ControlledSquads.Contains(SquadMarkerSource)))
			{
				DoDeselectAllUnitsCommand();
				SelectSquad(SquadMarkerSource, false);
			}
			if (SquadDragGarrisonPoint)
			{
				RequestSelectedSquadsGarrison(SquadDragGarrisonPoint);
			}
			else
			{
				FStrategyOrder Order;
				Order.Type = FStrategySquadMarkerRules::ResolveOrderType(SquadDragTarget != nullptr);
				Order.TargetActor = SquadDragTarget;
				Order.Destination = SquadDragTarget ? SquadDragTarget->GetActorLocation() : SquadDragDestination;
				DoIssueOrder(Order);
			}
		}
		ClearSquadMarkerInput();
		return;
	}

	// reset the drag box on the HUD
	if (StrategyHUD)
	{
		StrategyHUD->DragSelectUpdate(FVector2D::ZeroVector, FVector2D::ZeroVector, FVector2D::ZeroVector, false);
	}
}

void AStrategyPlayerController::SelectClick(const FInputActionValue& Value)
{
	if (bConsumeNextSelectClick)
	{
		bConsumeNextSelectClick = false;
		return;
	}
	FHitResult Hit;
	if (!GetHitUnderCursor(Hit))
	{
		return;
	}
	if (bBuildingPlacementActive)
	{
		if (AStrategyGameState* State = GetWorld()->GetGameState<AStrategyGameState>())
		{
			const EStrategyBuildingType BuildingType = static_cast<EStrategyBuildingType>(PendingBuildingIndex);
			if (State->TryPlaceBuilding(EStrategyFaction::Player, BuildingType, Hit.Location))
			{
				bBuildingPlacementActive = false;
				bBuildMenuOpen = false;
			}
			else
			{
				const FString Reason = State->GetFactionState(EStrategyFaction::Player).Gold
					< State->GetBuildingDefinition(BuildingType)->GoldCost
					? TEXT("金币不足") : FStrategyPlacementIssueRules::GetIssueText(State->GetBuildingPlacementIssue(
						EStrategyFaction::Player, BuildingType, Hit.Location));
				PlayInvalidActionFeedback(Reason);
			}
		}
		return;
	}
	if (AStrategyUnit* Unit = Cast<AStrategyUnit>(Hit.GetActor()))
	{
		DoDeselectAllUnitsCommand();
		if (Unit->GetStrategyFaction() == EStrategyFaction::Player)
		{
			SelectSquad(Unit->GetSquad(), false);
		}
		return;
	}
	if (AStrategyControlPoint* Point = Cast<AStrategyControlPoint>(Hit.GetActor()))
	{
		SelectControlPoint(Point);
		return;
	}
	if (AStrategyBuilding* Building = Cast<AStrategyBuilding>(Hit.GetActor()))
	{
		DoDeselectAllUnitsCommand();
		SelectBuilding(Building->GetStrategyFaction() == EStrategyFaction::Player ? Building : nullptr);
		return;
	}
	DoDeselectAllUnitsCommand();
}

void AStrategyPlayerController::SelectClickAdditive(const FInputActionValue& Value)
{
	if (bConsumeNextSelectClick)
	{
		bConsumeNextSelectClick = false;
		return;
	}
	// get the cursor location
	FVector CursorLocation;

	if (GetLocationUnderCursor(CursorLocation))
	{
		// additive select at the cursor
		DoSelectCommand(CursorLocation, true);
	}
}

void AStrategyPlayerController::SelectAllDoubleClick(const FInputActionValue& Value)
{
	DoSelectAllUnitsOnScreenCommand();
}

void AStrategyPlayerController::InteractHoldStarted(const FInputActionValue& Value)
{

	// save the starting interaction position
	StartingDragScrollPosition = GetMouseLocationForPlayer();
}

void AStrategyPlayerController::InteractHoldTriggered(const FInputActionValue& Value)
{
	// do a drag scroll 
	DoCameraDragScrollCommand(GetMouseLocationForPlayer());
}

void AStrategyPlayerController::InteractClick(const FInputActionValue& Value)
{
	FHitResult Hit;
	if (GetHitUnderCursor(Hit))
	{
		if (AStrategyControlPoint* Point = Cast<AStrategyControlPoint>(Hit.GetActor());
			FStrategyGarrisonCommandRules::ShouldEnterPoint(EStrategyFaction::Player,
				Point ? Point->GetStrategyFaction() : EStrategyFaction::Neutral, Point != nullptr))
		{
			RequestSelectedSquadsGarrison(Point);
			bAttackMovePending = false;
			return;
		}
		FStrategyOrder Order;
		AStrategyGameState* State = GetWorld()->GetGameState<AStrategyGameState>();
		IStrategyDamageable* Target = Cast<IStrategyDamageable>(Hit.GetActor());
		if (Target && FStrategyOrderTargetRules::CanAttack(EStrategyFaction::Player, Target->GetStrategyFaction(),
			Target->IsStrategyAlive(), State->IsVisibleToFaction(EStrategyFaction::Player, Hit.GetActor()->GetActorLocation())))
		{
			Order.Type = EStrategyOrderType::AttackTarget;
			Order.TargetActor = Hit.GetActor();
			Order.Destination = Hit.GetActor()->GetActorLocation();
		}
		else
		{
			Order.Type = bAttackMovePending ? EStrategyOrderType::AttackMove : EStrategyOrderType::Move;
			Order.Destination = Hit.Location;
		}
		DoIssueOrder(Order);
		bAttackMovePending = false;
	}
}

void AStrategyPlayerController::TouchPrimaryHoldStarted(const FInputActionValue& Value)
{
	// save the camera drag screen coords
	StartingDragScrollPosition = Value.Get<FVector2D>();

	
}

void AStrategyPlayerController::TouchPrimaryHoldTriggered(const FInputActionInstance& Instance)
{
	FVector2D InputVector = Instance.GetValue().Get<FVector2D>();

	// update the box select start position
	StartingBoxSelectionPosition = InputVector;

	if (Instance.GetElapsedTime() > TouchDragScrollHoldTime)
	{
		DoCameraDragScrollCommand(InputVector);

		// save the game time
		LastTouchDragScrollTime = GetWorld()->GetTimeSeconds();
	}
}

void AStrategyPlayerController::TouchPrimaryHoldCompleted(const FInputActionValue& Value)
{
	// ensure we don't trigger a tap input right after we finish a drag scroll
	if (GetWorld()->GetTimeSeconds() - LastTouchDragScrollTime > 0.1f)
	{
		// get the touch location in world space
		FVector TouchLocation = ProjectTouchPointToWorldSpace();

		// try to do a select command
		if (!DoSelectCommand(TouchLocation, true))
		{
			// if nothing was selected, do a move units command instead
			DoMoveUnitsCommand(TouchLocation);
		}
	}
}

void AStrategyPlayerController::TouchSecondaryTriggered(const FInputActionValue& Value)
{
	// get the touch 2 screen coords
	FVector2D SelectionPosition = Value.Get<FVector2D>();

	// calculate the size of the selection box
	FVector2D SelectionSize = SelectionPosition - StartingBoxSelectionPosition;

	// update the selection box on the HUD
	if (StrategyHUD)
	{
		StrategyHUD->DragSelectUpdate(StartingBoxSelectionPosition, SelectionSize, SelectionPosition, true);
	}
		
}

void AStrategyPlayerController::TouchSecondaryCompleted(const FInputActionValue& Value)
{
	if (StrategyHUD)
	{
		// hide the selection box
		StrategyHUD->DragSelectUpdate(FVector2D::ZeroVector, FVector2D::ZeroVector, FVector2D::ZeroVector, false);
	}
}

bool AStrategyPlayerController::DoSelectCommand(const FVector& SelectLocation, bool bAdditiveSelection)
{
	// deselect any units unless this is an additive selection
	if (!bAdditiveSelection)
	{
		DoDeselectAllUnitsCommand();
	}

	// do an overlap test at the cursor location
	TArray<FOverlapResult> OutOverlaps;

	FCollisionShape CollisionSphere;
	CollisionSphere.SetSphere(SelectionRadius);

	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_GameTraceChannel1);

	FCollisionQueryParams QueryParams;

	if (GetWorld()->OverlapMultiByObjectType(OutOverlaps, SelectLocation, FQuat::Identity, ObjectParams, CollisionSphere, QueryParams))
	{
		// find the first unit we've overlapped
		for (const FOverlapResult& CurrentOverlap : OutOverlaps)
		{
			if (AStrategyUnit* CurrentUnit = Cast<AStrategyUnit>(CurrentOverlap.GetActor()))
			{
				if (CurrentUnit->GetStrategyFaction() == EStrategyFaction::Player && CurrentUnit->GetSquad())
				{
					SelectSquad(CurrentUnit->GetSquad(), bAdditiveSelection);
					return true;
				}
			}
		}
	}

	// didn't find a unit
	return false;
}

void AStrategyPlayerController::DoSelectAllUnitsOnScreenCommand()
{
	// get all units on the level
	TArray<AActor*> Units;

	UGameplayStatics::GetAllActorsOfClass(this, AStrategyUnit::StaticClass(), Units);

	// process each unit
	for (AActor* CurrentActor : Units)
	{
		if (AStrategyUnit* CurrentUnit = Cast<AStrategyUnit>(CurrentActor))
		{
			// is the unit is not already selected, and is on screen?
			if (CurrentUnit->GetStrategyFaction() == EStrategyFaction::Player && CurrentUnit->GetSquad() && CurrentUnit->WasRecentlyRendered(0.2f))
			{
				SelectSquad(CurrentUnit->GetSquad(), false);
			}
		}
		
	}
}

void AStrategyPlayerController::DoDeselectAllUnitsCommand()
{
	for (AStrategySquad* Squad : ControlledSquads)
	{
		if (IsValid(Squad))
		{
			Squad->SetSelected(false);
		}
	}
	ControlledSquads.Empty();
	ControlledUnits.Empty();
	SelectBuilding(nullptr);
	SelectControlPoint(nullptr);
}

void AStrategyPlayerController::DoToggleSelectAllUnitsCommand()
{
	// do we have units selected?
	if (ControlledUnits.Num() > 0)
	{
		// deselect all units
		DoDeselectAllUnitsCommand();
	}
	else
	{
		// select all units on screen
		DoSelectAllUnitsOnScreenCommand();
	}
}

void AStrategyPlayerController::DoCameraDragScrollCommand(const FVector2D& CurrentCursorPosition)
{
	// subtract the starting position from the cursor to find the on-screen movement delta
	FVector2D MoveDelta = StartingDragScrollPosition - CurrentCursorPosition;

	// rotate the movement delta to match the isometric perspective
	const FRotator IsoRotation(0.0f, ControlledCameraPawn ? ControlledCameraPawn->GetCameraYaw() : -45.0f, 0.0f);

	FVector RotatedDelta = IsoRotation.RotateVector(FVector(MoveDelta.X, MoveDelta.Y, 0.0f));

	// apply drag
	RotatedDelta *= DragMultiplier;

	// apply the offset to the camera pawn
	if (ControlledCameraPawn)
	{
		ControlledCameraPawn->AddActorWorldOffset(RotatedDelta);
	}
}

void AStrategyPlayerController::DoMoveUnitsCommand(const FVector& GoalLocation)
{
	FStrategyOrder Order;
	Order.Type = EStrategyOrderType::Move;
	Order.Destination = GoalLocation;
	DoIssueOrder(Order);
}

void AStrategyPlayerController::DoIssueOrder(const FStrategyOrder& Order)
{
	int32 IssuedCount = 0;
	for (AStrategySquad* Squad : ControlledSquads)
	{
		if (!IsValid(Squad))
		{
			continue;
		}
		if (AStrategyControlPoint* Point = Squad->GetGarrisonPoint())
		{
			const float Distance = FVector::Dist2D(Point->GetActorLocation(), Order.Destination);
			if (!FStrategyGarrisonCommandRules::ShouldExitForOrder(Order.Type, Distance))
			{
				++IssuedCount;
				continue;
			}
			FVector Direction = (Order.Destination - Point->GetActorLocation()).GetSafeNormal2D();
			if (Direction.IsNearlyZero())
			{
				Direction = FVector::ForwardVector;
			}
			Squad->ExitGarrison(Point->GetActorLocation() + Direction * 760.0f);
		}
		Squad->IssueOrder(Order);
		++IssuedCount;
	}
	BP_CursorFeedback(Order.Destination, IssuedCount > 0);
	if (IssuedCount > 0)
	{
		ShowOrderFeedback(Order);
	}
	else
	{
		PlayInvalidActionFeedback();
	}
}

void AStrategyPlayerController::MoveCameraFromMinimap(const FVector2D& WorldLocation)
{
	int32 Width = 0;
	int32 Height = 0;
	GetViewportSize(Width, Height);
	ControlledCameraPawn->SetGroundFocus(WorldLocation, static_cast<float>(Width) / Height);
}

bool AStrategyPlayerController::GetCameraGroundCorners(TArray<FVector2D>& OutCorners) const
{
	int32 Width = 0;
	int32 Height = 0;
	GetViewportSize(Width, Height);
	OutCorners.Reset(4);
	for (const FVector2D& Screen : {
		FVector2D(0.0f, 0.0f), FVector2D(static_cast<float>(Width), 0.0f),
		FVector2D(static_cast<float>(Width), static_cast<float>(Height)), FVector2D(0.0f, static_cast<float>(Height))})
	{
		FVector Origin;
		FVector Direction;
		DeprojectScreenPositionToWorld(Screen.X, Screen.Y, Origin, Direction);
		const FVector Ground = Origin + Direction * (-Origin.Z / Direction.Z);
		OutCorners.Add(FVector2D(Ground.X, Ground.Y));
	}
	return true;
}

void AStrategyPlayerController::IssueMinimapCommand(const FVector2D& WorldLocation, AActor* VisibleEnemyTarget)
{
	const EStrategyMinimapCommandIntent Intent = FStrategyMinimapCommandRules::ResolveIntent(
		!ControlledSquads.IsEmpty(), IsValid(VisibleEnemyTarget));
	if (Intent == EStrategyMinimapCommandIntent::None)
	{
		return;
	}

	FStrategyOrder Order;
	if (Intent == EStrategyMinimapCommandIntent::Attack)
	{
		Order.Type = EStrategyOrderType::AttackTarget;
		Order.TargetActor = VisibleEnemyTarget;
		Order.Destination = VisibleEnemyTarget->GetActorLocation();
	}
	else
	{
		Order.Type = EStrategyOrderType::Move;
		Order.Destination = FVector(WorldLocation.X, WorldLocation.Y, 0.0f);
	}
	DoIssueOrder(Order);
}

void AStrategyPlayerController::ShowOrderFeedback(const FStrategyOrder& Order)
{
	const AStrategyGameState* State = GetWorld()->GetGameState<AStrategyGameState>();
	const UStrategyPresentationDataAsset* Presentation = State->GetPresentationDefinition();
	UNiagaraSystem* Effect = Presentation->MoveCommandEffect;
	USoundBase* Sound = Presentation->MoveSound;
	if (Order.Type == EStrategyOrderType::AttackMove)
	{
		Effect = Presentation->AttackMoveCommandEffect;
		Sound = Presentation->AttackOrderSound;
	}
	else if (Order.Type == EStrategyOrderType::AttackTarget)
	{
		Effect = Presentation->AttackTargetEffect;
		Sound = Presentation->AttackOrderSound;
	}
	const FVector FeedbackLocation = IsValid(Order.TargetActor) ? Order.TargetActor->GetActorLocation() : Order.Destination;
	UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, Effect, FeedbackLocation);
	UGameplayStatics::PlaySound2D(this, Sound);
}

void AStrategyPlayerController::PlayInvalidActionFeedback(const FString& Message)
{
	const AStrategyGameState* State = GetWorld()->GetGameState<AStrategyGameState>();
	UGameplayStatics::PlaySound2D(this, State->GetPresentationDefinition()->InvalidSound);
	if (HUDRoot)
	{
		HUDRoot->PushNotification(Message, FLinearColor(0.93f, 0.34f, 0.28f, 1.0f));
	}
}

void AStrategyPlayerController::UpdateMatchResultFeedback()
{
	if (bMatchResultFeedbackPlayed)
	{
		return;
	}
	const AStrategyGameState* State = GetWorld()->GetGameState<AStrategyGameState>();
	if (State && State->GetWinner() != EStrategyFaction::Neutral)
	{
		const UStrategyPresentationDataAsset* Presentation = State->GetPresentationDefinition();
		UGameplayStatics::PlaySound2D(this,
			State->GetWinner() == EStrategyFaction::Player ? Presentation->VictorySound : Presentation->DefeatSound);
		bMatchResultFeedbackPlayed = true;
	}
}

void AStrategyPlayerController::DoCameraModifyZoomCommand(float ZoomDelta)
{
	// add the delta
	CameraZoom += ZoomDelta;

	// clamp between min and max
	CameraZoom = FMath::Clamp(CameraZoom, MinZoomLevel, MaxZoomLevel);

	// set the zoom on the camera pawn
	if (ControlledCameraPawn)
	{
		ControlledCameraPawn->SetZoomModifier(CameraZoom);
	}
}

void AStrategyPlayerController::DoCameraResetZoomCommand()
{
	// reset to default zoom
	CameraZoom = DefaultZoom;

	// set the zoom on the camera pawn
	if (ControlledCameraPawn)
	{
		ControlledCameraPawn->SetZoomModifier(CameraZoom);
	}
}

void AStrategyPlayerController::DoCameraSetZoomPercentageCommand(float Percentage)
{
	// lerp between min and max zoom
	CameraZoom = FMath::Lerp(MinZoomLevel, MaxZoomLevel, FMath::Clamp(Percentage, 0.0f, 1.0f));

	// set the zoom on the camera pawn
	if (ControlledCameraPawn)
	{
		ControlledCameraPawn->SetZoomModifier(CameraZoom);
	}
}

AStrategyUnit* AStrategyPlayerController::GetClosestSelectedUnitToLocation(FVector TargetLocation)
{
	RefreshControlledUnits();

	// closest unit and distance
	AStrategyUnit* OutUnit = nullptr;
	float Closest = 0.0f;

	// process each unit on the list
	for (AStrategyUnit* CurrentUnit : ControlledUnits)
	{
		if (IsValid(CurrentUnit))
		{
			// have we selected a unit already?
			if (OutUnit != nullptr)
			{
				// calculate the squared distance to the target location
				float Dist = FVector::DistSquared2D(TargetLocation, CurrentUnit->GetActorLocation());

				// is this unit closer?
				if (Dist < Closest)
				{
					// update the closest unit and distance
					OutUnit = CurrentUnit;
					Closest = Dist;
				}

			}
			else
			{

				// no previously selected unit, so use this one
				OutUnit = CurrentUnit;

				// initialize the closest distance
				Closest = FVector::DistSquared2D(TargetLocation, CurrentUnit->GetActorLocation());
			}
		}
		
	}

	// return the selected unit
	return OutUnit;
}

FVector2D AStrategyPlayerController::GetMouseLocationForPlayer()
{
	// attempt to get the mouse position from this PC
	float MouseX, MouseY;

	if (GetMousePosition(MouseX, MouseY))
	{
		return FVector2D(MouseX, MouseY);
	}

	// return an invalid vector
	return FVector2D::ZeroVector;
}

bool AStrategyPlayerController::GetLocationUnderCursor(FVector& Location)
{
	// trace the visibility channel at the cursor location
	FHitResult OutHit;

	GetHitResultUnderCursorByChannel(SelectionTraceChannel, false, OutHit);

	// if there was a blocking hit, return the hit location
	if (OutHit.bBlockingHit)
	{
		Location = OutHit.Location;
		return true;
	}

	return OutHit.bBlockingHit;
}

bool AStrategyPlayerController::GetLocationUnderFinger(FVector& Location)
{
	// trace the visibility channel at Touch 1 location
	FHitResult OutHit;

	GetHitResultUnderFingerByChannel(ETouchIndex::Touch1, SelectionTraceChannel, false, OutHit);

	// if there was a blocking hit, return the hit location
	if (OutHit.bBlockingHit)
	{
		Location = OutHit.Location;
		return true;
	}

	return OutHit.bBlockingHit;
}

FVector AStrategyPlayerController::ProjectTouchPointToWorldSpace()
{
	// get the touch coordinates for the first finger
	float TouchX, TouchY = 0.0f;
	bool bPressed = false;

	GetInputTouchState(ETouchIndex::Touch1, TouchX, TouchY, bPressed);

	FVector WorldLocation = FVector::ZeroVector;
	FVector WorldDirection = FVector::ZeroVector;

	// deproject the coords into world space
	if (DeprojectScreenPositionToWorld(TouchX, TouchY, WorldLocation, WorldDirection))
	{
		// run a line trace down the camera
		FHitResult OutHit;

		GetWorld()->LineTraceSingleByChannel(OutHit, WorldLocation, WorldLocation + WorldDirection * 10000.0f, ECC_Visibility);

		// if we hit something, return the impact point
		if (OutHit.bBlockingHit)
		{
			return OutHit.ImpactPoint;
		}
		

		// intersect with a horizontal plane and return the resulting point
		const FPlane IntersectPlane(FVector::ZeroVector, FVector::UpVector);
		return FMath::LinePlaneIntersection(WorldLocation, WorldLocation + (WorldDirection * 100000.0f), IntersectPlane);
	}

	// failed to deproject, return a zero vector
	return FVector::ZeroVector;
}

bool AStrategyPlayerController::GetHitUnderCursor(FHitResult& Hit)
{
	GetHitResultUnderCursorByChannel(SelectionTraceChannel, false, Hit);
	return Hit.bBlockingHit;
}

AStrategySquad* AStrategyPlayerController::FindSquadMarkerAtScreenPosition(const FVector2D& ScreenPosition) const
{
	const AStrategyGameState* State = GetWorld()->GetGameState<AStrategyGameState>();
	int32 ViewportWidth;
	int32 ViewportHeight;
	GetViewportSize(ViewportWidth, ViewportHeight);
	TArray<AStrategySquad*> Squads;
	TArray<FVector2D> MarkerPositions;
	for (AStrategySquad* Squad : State->GetSquads())
	{
		FVector2D MarkerPosition;
		if (IsValid(Squad) && Squad->GetFaction() == EStrategyFaction::Player && Squad->IsAlive()
			&& ProjectWorldLocationToScreen(Squad->GetMarkerWorldLocation(), MarkerPosition)
			&& MarkerPosition.X >= 0.0f && MarkerPosition.X <= ViewportWidth
			&& MarkerPosition.Y >= 0.0f && MarkerPosition.Y <= ViewportHeight)
		{
			Squads.Add(Squad);
			MarkerPositions.Add(MarkerPosition);
		}
	}
	const int32 HoveredIndex = FStrategySquadMarkerRules::FindHoveredMarker(MarkerPositions, ScreenPosition, 17.0f);
	return Squads.IsValidIndex(HoveredIndex) ? Squads[HoveredIndex] : nullptr;
}

void AStrategyPlayerController::UpdateSquadMarkerDragTarget()
{
	FHitResult Hit;
	GetHitUnderCursor(Hit);
	AStrategyGameState* State = GetWorld()->GetGameState<AStrategyGameState>();
	SquadDragGarrisonPoint = Cast<AStrategyControlPoint>(Hit.GetActor());
	if (SquadDragGarrisonPoint && SquadDragGarrisonPoint->GetStrategyFaction() == EStrategyFaction::Player)
	{
		SquadDragTarget = nullptr;
		SquadDragDestination = SquadDragGarrisonPoint->GetActorLocation();
		return;
	}
	SquadDragGarrisonPoint = nullptr;
	IStrategyDamageable* Target = Cast<IStrategyDamageable>(Hit.GetActor());
	if (Target && FStrategyOrderTargetRules::CanAttack(EStrategyFaction::Player, Target->GetStrategyFaction(),
		Target->IsStrategyAlive(), State->IsVisibleToFaction(EStrategyFaction::Player, Hit.GetActor()->GetActorLocation())))
	{
		SquadDragTarget = Hit.GetActor();
	}
	else
	{
		SquadDragTarget = nullptr;
		SquadDragDestination = Hit.Location;
	}
}

void AStrategyPlayerController::ClearSquadMarkerInput()
{
	SquadMarkerSource = nullptr;
	SquadDragTarget = nullptr;
	SquadDragGarrisonPoint = nullptr;
	SquadDragDestination = FVector::ZeroVector;
	bSquadMarkerInputActive = false;
	bSquadMarkerDragging = false;
}

void AStrategyPlayerController::RequestSelectedSquadsGarrison(AStrategyControlPoint* Point)
{
	int32 AvailableSlots = Point ? Point->GetGarrisonCapacity() - Point->GetGarrisonedSquads().Num() : 0;
	int32 RequestedCount = 0;
	for (AStrategySquad* Squad : ControlledSquads)
	{
		if (!IsValid(Squad) || Squad->GetFaction() != EStrategyFaction::Player)
		{
			continue;
		}
		if (Squad->GetGarrisonPoint() == Point)
		{
			++RequestedCount;
			continue;
		}
		if (AvailableSlots <= 0)
		{
			continue;
		}
		Squad->RequestGarrison(Point);
		--AvailableSlots;
		++RequestedCount;
	}
	if (RequestedCount > 0)
	{
		BP_CursorFeedback(Point->GetActorLocation(), true);
	}
	else
	{
		PlayInvalidActionFeedback(TEXT("驻防容量已满"));
	}
}

void AStrategyPlayerController::SelectSquad(AStrategySquad* Squad, bool bToggle)
{
	if (!IsValid(Squad))
	{
		return;
	}
	SelectControlPoint(nullptr);
	if (bToggle && ControlledSquads.Contains(Squad))
	{
		ControlledSquads.Remove(Squad);
		Squad->SetSelected(false);
	}
	else if (!ControlledSquads.Contains(Squad))
	{
		ControlledSquads.Add(Squad);
		Squad->SetSelected(true);
		bSelectionFeedbackPending |= !bSuppressSelectionFeedback;
	}
	RefreshControlledUnits();
}

void AStrategyPlayerController::SelectBuilding(AStrategyBuilding* Building)
{
	if (Building)
	{
		SelectControlPoint(nullptr);
	}
	if (SelectedBuilding)
	{
		SelectedBuilding->SetSelected(false);
	}
	SelectedBuilding = Building;
	if (SelectedBuilding)
	{
		SelectedBuilding->SetSelected(true);
		bSelectionFeedbackPending |= !bSuppressSelectionFeedback;
	}
}

void AStrategyPlayerController::SelectControlPoint(AStrategyControlPoint* Point)
{
	if (Point)
	{
		DoDeselectAllUnitsCommand();
	}
	SelectedControlPoint = Point;
}

bool AStrategyPlayerController::TrySpecializeSelectedTown(EStrategyTownSpecialization Specialization)
{
	const bool bSucceeded = GetWorld()->GetGameState<AStrategyGameState>()->TryStartTownSpecialization(
		EStrategyFaction::Player, SelectedControlPoint, Specialization);
	if (!bSucceeded)
	{
		PlayInvalidActionFeedback(TEXT("城镇当前不可专精或金币不足"));
	}
	return bSucceeded;
}

bool AStrategyPlayerController::TryDowngradeSelectedTown()
{
	const bool bSucceeded = GetWorld()->GetGameState<AStrategyGameState>()->TryStartTownDowngrade(
		EStrategyFaction::Player, SelectedControlPoint);
	if (!bSucceeded)
	{
		PlayInvalidActionFeedback(TEXT("城镇当前不可降级"));
	}
	return bSucceeded;
}

void AStrategyPlayerController::RefreshControlledUnits()
{
	ControlledUnits.Reset();
	for (AStrategySquad* Squad : ControlledSquads)
	{
		if (IsValid(Squad))
		{
			for (AStrategyUnit* Unit : Squad->GetMembers())
			{
				if (IsValid(Unit))
				{
					ControlledUnits.Add(Unit);
				}
			}
		}
	}
}

void AStrategyPlayerController::HandleAttackMoveKey()
{
	bAttackMovePending = true;
}

void AStrategyPlayerController::BeginMoveCommandFromUI()
{
	bAttackMovePending = false;
	if (HUDRoot)
	{
		HUDRoot->PushNotification(TEXT("右键选择移动目标"), FLinearColor(0.95f, 0.78f, 0.28f, 1.0f));
	}
	RestoreGameFocus();
}

void AStrategyPlayerController::BeginAttackMoveCommandFromUI()
{
	HandleAttackMoveKey();
	RestoreGameFocus();
}

void AStrategyPlayerController::StopSelectedSquadsFromUI()
{
	HandleStopKey();
	RestoreGameFocus();
}

void AStrategyPlayerController::ToggleBuildMenuFromUI()
{
	HandleBuildMenuKey();
	RestoreGameFocus();
}

void AStrategyPlayerController::SelectBuildItemFromUI(int32 Index)
{
	if (Index == 7 && !bBuildMenuOpen)
	{
		bBuildMenuOpen = true;
	}
	HandleNumberKey(Index);
	RestoreGameFocus();
}

bool AStrategyPlayerController::TrainSelectedBuildingFromUI(EStrategyUnitType UnitType)
{
	const bool bSucceeded = IsValid(SelectedBuilding) && SelectedBuilding->QueueUnit(UnitType);
	if (!bSucceeded)
	{
		FString Reason = TEXT("当前建筑不能训练此单位");
		if (IsValid(SelectedBuilding))
		{
			AStrategyGameState* State = GetWorld()->GetGameState<AStrategyGameState>();
			const FStrategyFactionState& Faction = State->GetFactionState(EStrategyFaction::Player);
			const UStrategyUnitDataAsset* Unit = State->GetUnitDefinition(UnitType);
			const UStrategyBuildingDataAsset* Building = State->GetBuildingDefinition(SelectedBuilding->GetBuildingType());
			Reason = FStrategyHUDActionRules::GetUnavailableReasonText(FStrategyHUDActionRules::GetTrainingUnavailableReason(
				SelectedBuilding->IsConstructionComplete(), Building->TrainableUnits.Contains(UnitType), SelectedBuilding->GetQueueLength(),
				Faction.Gold, Faction.UsedPopulation + Faction.ReservedPopulation, Faction.PopulationCap,
				Unit->GoldCost, Unit->PopulationCost));
		}
		PlayInvalidActionFeedback(Reason);
	}
	RestoreGameFocus();
	return bSucceeded;
}

void AStrategyPlayerController::RestoreGameFocus()
{
	FSlateApplication::Get().SetAllUserFocusToGameViewport();
	bShowMouseCursor = true;
}

bool AStrategyPlayerController::GetCursorWorldLocationForUI(FVector& Location)
{
	return GetLocationUnderCursor(Location);
}

void AStrategyPlayerController::HandleStopKey()
{
	FStrategyOrder Order;
	Order.Type = EStrategyOrderType::Stop;
	DoIssueOrder(Order);
}

void AStrategyPlayerController::HandleBuildMenuKey()
{
	ClearSquadMarkerInput();
	bBuildMenuOpen = !bBuildMenuOpen;
	bBuildingPlacementActive = false;
	bWallPlacementActive = false;
	bWallDragging = false;
	ClearWallPreview();
}

void AStrategyPlayerController::HandleNumberKey(int32 Index)
{
	if (bBuildMenuOpen && Index >= 1 && Index <= 5)
	{
		ClearSquadMarkerInput();
		PendingBuildingIndex = static_cast<uint8>(Index - 1);
		bBuildingPlacementActive = true;
		bWallPlacementActive = false;
		return;
	}
	if (bBuildMenuOpen && Index == 6)
	{
		ClearSquadMarkerInput();
		bBuildingPlacementActive = false;
		bWallPlacementActive = true;
		return;
	}
	if (bBuildMenuOpen && Index == 7)
	{
		AStrategyBuilding* Gate = GetWorld()->GetGameState<AStrategyGameState>()->TryUpgradeWallToGate(
			EStrategyFaction::Player, SelectedBuilding);
		if (Gate)
		{
			SelectBuilding(Gate);
			bBuildMenuOpen = false;
		}
		else
		{
			PlayInvalidActionFeedback(TEXT("需要选中已完成城墙且金币充足"));
		}
		return;
	}
	if (SelectedBuilding && Index >= 1 && Index <= 3)
	{
		TrainSelectedBuildingFromUI(static_cast<EStrategyUnitType>(Index - 1));
	}
}

void AStrategyPlayerController::HandleNumber1() { HandleNumberKey(1); }
void AStrategyPlayerController::HandleNumber2() { HandleNumberKey(2); }
void AStrategyPlayerController::HandleNumber3() { HandleNumberKey(3); }
void AStrategyPlayerController::HandleNumber4() { HandleNumberKey(4); }
void AStrategyPlayerController::HandleNumber5() { HandleNumberKey(5); }
void AStrategyPlayerController::HandleNumber6() { HandleNumberKey(6); }
void AStrategyPlayerController::HandleNumber7() { HandleNumberKey(7); }

void AStrategyPlayerController::UpdateWallPreview(const FVector& End)
{
	ClearWallPreview();
	AStrategyGameState* State = GetWorld()->GetGameState<AStrategyGameState>();
	const TArray<FStrategyWallSegmentPlan> Plans = FStrategyWallPlanner::BuildLine(WallDragStart, End, 400.0f, 30);
	const UStrategyBuildingDataAsset* Definition = State->GetBuildingDefinition(EStrategyBuildingType::Wall);
	float RemainingGold = State->GetFactionState(EStrategyFaction::Player).Gold;
	UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	UMaterialInterface* BaseMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));

	for (const FStrategyWallSegmentPlan& Plan : Plans)
	{
		const bool bValid = RemainingGold >= Definition->GoldCost && State->CanPlaceBuilding(
			EStrategyFaction::Player, EStrategyBuildingType::Wall, Plan.Location, Plan.Rotation);
		if (bValid)
		{
			RemainingGold -= Definition->GoldCost;
		}

		AStaticMeshActor* Preview = GetWorld()->SpawnActor<AStaticMeshActor>(Plan.Location, Plan.Rotation);
		Preview->SetActorEnableCollision(false);
		Preview->SetFlags(RF_Transient);
		UStaticMeshComponent* PreviewMesh = Preview->GetStaticMeshComponent();
		PreviewMesh->SetStaticMesh(Cube);
		PreviewMesh->SetWorldScale3D(FVector(4.0f, 1.2f, 2.5f));
		PreviewMesh->SetRenderCustomDepth(true);
		UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(BaseMaterial, Preview);
		Material->SetVectorParameterValue(TEXT("Color"), bValid
			? FLinearColor(0.1f, 0.9f, 0.25f, 0.55f)
			: FLinearColor(0.95f, 0.1f, 0.08f, 0.55f));
		PreviewMesh->SetMaterial(0, Material);
		WallPreviewActors.Add(Preview);
	}
}

void AStrategyPlayerController::ClearWallPreview()
{
	for (AStaticMeshActor* Preview : WallPreviewActors)
	{
		if (IsValid(Preview))
		{
			Preview->Destroy();
		}
	}
	WallPreviewActors.Reset();
}

void AStrategyPlayerController::HandlePauseKey()
{
	OpenPauseMenu();
}

void AStrategyPlayerController::OpenPauseMenu()
{
	bBuildMenuOpen = false;
	bBuildingPlacementActive = false;
	bWallPlacementActive = false;
	bWallDragging = false;
	bAttackMovePending = false;
	ClearWallPreview();
	ClearSquadMarkerInput();
	if (StrategyHUD)
	{
		StrategyHUD->DragSelectUpdate(FVector2D::ZeroVector, FVector2D::ZeroVector, FVector2D::ZeroVector, false);
	}

	SetPause(true);
	PauseMenu->OpenPausePage();
	PauseMenu->SetVisibility(ESlateVisibility::Visible);
	FInputModeUIOnly InputMode;
	InputMode.SetWidgetToFocus(PauseMenu->TakeWidget());
	SetInputMode(InputMode);
	bShowMouseCursor = true;
	PauseMenu->SetKeyboardFocus();
}

void AStrategyPlayerController::ClosePauseMenu()
{
	// “继续游戏”由按钮的鼠标抬起事件触发，先释放 Slate 捕获，避免菜单隐藏后仍占用左右键。
	FSlateApplication::Get().ReleaseAllPointerCapture();
	PauseMenu->SetVisibility(ESlateVisibility::Collapsed);
	SetPause(false);
	FInputModeGameAndUI InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	InputMode.SetHideCursorDuringCapture(false);
	SetInputMode(InputMode);
	FSlateApplication::Get().SetAllUserFocusToGameViewport();
	bShowMouseCursor = true;
}

void AStrategyPlayerController::QuitFromPauseMenu()
{
	UKismetSystemLibrary::QuitGame(this, this, EQuitPreference::Quit, false);
}

bool AStrategyPlayerController::IsPauseMenuOpen() const
{
	return PauseMenu && PauseMenu->GetVisibility() == ESlateVisibility::Visible;
}

void AStrategyPlayerController::HandleRestartKey()
{
	if (const AStrategyGameState* State = GetWorld()->GetGameState<AStrategyGameState>(); State && !State->IsMatchRunning())
	{
		UGameplayStatics::OpenLevel(this, FName(*UGameplayStatics::GetCurrentLevelName(this, true)));
	}
}

void AStrategyPlayerController::HandleQuitKey()
{
	if (const AStrategyGameState* State = GetWorld()->GetGameState<AStrategyGameState>(); State && !State->IsMatchRunning())
	{
		UKismetSystemLibrary::QuitGame(this, this, EQuitPreference::Quit, false);
	}
}

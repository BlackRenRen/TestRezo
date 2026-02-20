#include "MenuPlayerController.h"

#include "SessionMenuWidget.h"
#include "Blueprint/UserWidget.h"

void AMenuPlayerController::BeginPlay()
{
    Super::BeginPlay();

	UE_LOG(LogTemp, Log, TEXT("AMenuPlayerController::BeginPlay"));

	// Très important : ne créer le widget que pour le contrôleur local
	if (!IsLocalController())
	{
		return;
	}

	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;

	if (!SessionMenuClass)
	{
		UE_LOG(LogTemp, Error, TEXT("AMenuPlayerController: SessionMenuClass is not set"));
		return;
	}

	SessionMenu = CreateWidget<USessionMenuWidget>(this, SessionMenuClass);
	if (!SessionMenu)
	{
		UE_LOG(LogTemp, Error, TEXT("AMenuPlayerController: Failed to create SessionMenu widget"));
		return;
	}

	SessionMenu->AddToViewport(0);
	SetCurrentMenuWidget(SessionMenu);

	// Pas de focus clavier (VR-friendly) : ne pas appeler SetWidgetToFocus.
	FInputModeUIOnly InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputMode);
}

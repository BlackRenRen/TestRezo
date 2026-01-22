#include "MenuPlayerController.h"

#include "SessionMenuWidget.h"
#include "Blueprint/UserWidget.h"

void AMenuPlayerController::BeginPlay()
{
    Super::BeginPlay();

    UE_LOG(LogTemp, Log, TEXT("AMenuPlayerController::BeginPlay appele"));

    if (!IsLocalController())
    {
        UE_LOG(LogTemp, Log, TEXT("AMenuPlayerController: non-local controller, pas de widget de menu"));
        return;
    }

    if (!SessionMenuClass)
    {
        UE_LOG(LogTemp, Error, TEXT("AMenuPlayerController : SessionMenuClass n'est PAS configure !"));
        return;
    }

    SessionMenu = CreateWidget<USessionMenuWidget>(this, SessionMenuClass);
    if (!SessionMenu)
    {
        UE_LOG(LogTemp, Error, TEXT("AMenuPlayerController : CreateWidget a renvoy nullptr !"));
        return;
    }

    SessionMenu->AddToViewport();
    SetCurrentMenuWidget(SessionMenu);

    bShowMouseCursor = true;

    FInputModeUIOnly InputMode;
    InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);

    // IMPORTANT : SetWidgetToFocus attend un TSharedPtr<SWidget>
    // TakeWidget() retourne un TSharedRef<SWidget> -> conversion OK, mais on reste explicite.
 
    SetInputMode(InputMode);
}


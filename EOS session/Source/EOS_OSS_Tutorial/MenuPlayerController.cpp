#include "MenuPlayerController.h"

#include "SessionMenuWidget.h"
#include "Blueprint/UserWidget.h"

void AMenuPlayerController::BeginPlay()
{
    Super::BeginPlay();

    UE_LOG(LogTemp, Log, TEXT("AMenuPlayerController::BeginPlay appele"));

    // Très important : ne créer le widget que pour le contrôleur local
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

    // Afficher le menu à l’écran
    SessionMenu->AddToViewport();

    // Mode d’input UI uniquement, focus sur ce widget
    FInputModeUIOnly InputMode;
    InputMode.SetWidgetToFocus(SessionMenu->TakeWidget());
    InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
    SetInputMode(InputMode);

    SetCurrentMenuWidget(SessionMenu);

    if (SessionMenuClass)
    {
        UUserWidget* Menu = CreateWidget<UUserWidget>(this, SessionMenuClass);
        if (Menu)
        {
            Menu->AddToViewport();
            bShowMouseCursor = true;
            SetInputMode(FInputModeUIOnly());
        }
    }

    bShowMouseCursor = true;
}

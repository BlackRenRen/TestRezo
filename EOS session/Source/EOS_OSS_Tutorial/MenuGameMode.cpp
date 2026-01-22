// MenuGameMode.cpp

#include "MenuGameMode.h"
#include "MenuPlayerController.h"

AMenuGameMode::AMenuGameMode()
{
    PlayerControllerClass = AMenuPlayerController::StaticClass();

    // Pas de pawn obligatoire dans un menu
    DefaultPawnClass = nullptr;
}

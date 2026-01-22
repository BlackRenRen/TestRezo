#include "EOSGameMode.h"
#include "EOSPlayerState.h"

AEOSGameMode::AEOSGameMode()
{
}

void AEOSGameMode::PostLogin(APlayerController* NewPlayer)
{
    Super::PostLogin(NewPlayer);

    if (APlayerState* PS = NewPlayer ? NewPlayer->PlayerState : nullptr)
    {
        if (AEOSPlayerState* EOSPS = Cast<AEOSPlayerState>(PS))
        {
            // Exemple : aller chercher les stats au login
            EOSPS->FetchStats(FName(TEXT("TotalWins")));
        }
    }
}

void AEOSGameMode::Logout(AController* Exiting)
{
    Super::Logout(Exiting);
    // Place pour une logique de nettoyage éventuelle
}

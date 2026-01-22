#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"

// On a besoin des types FOnlineStatsUserStats et du delegate
#include "Interfaces/OnlineStatsInterface.h"

#include "EOSPlayerState.generated.h"

UCLASS()
class EOS_OSS_TUTORIAL_API AEOSPlayerState : public APlayerState
{
    GENERATED_BODY()

public:
    AEOSPlayerState();

    // Exemple : on stocke une stat entière dans un cache simple
    UPROPERTY(BlueprintReadOnly, Category = "Stats")
    int32 CachedKills = 0;

    // Demande de lecture d’une stat distante pour CE joueur
    UFUNCTION(BlueprintCallable, Category = "Stats")
    void FetchStats(const FName& StatName);

private:
    // Callback du delegate FOnlineStatsQueryUsersStatsComplete
    void OnStatsQueryComplete(
        const FOnlineError& Result,
        const TArray<TSharedRef<const FOnlineStatsUserStats>>& Users,
        FName RequestedStat);
};

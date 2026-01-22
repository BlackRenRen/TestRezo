#include "EOSPlayerState.h"

#include "OnlineSubsystem.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "OnlineError.h"               // définition complète de FOnlineError


AEOSPlayerState::AEOSPlayerState()
{
}

void AEOSPlayerState::FetchStats(const FName& StatName)
{
    // Récupération du OnlineSubsystem actif (EOS si configuré)
    IOnlineSubsystem* OSS = IOnlineSubsystem::Get();
    if (!OSS)
    {
        UE_LOG(LogTemp, Warning, TEXT("FetchStats: OnlineSubsystem not found"));
        return;
    }

    IOnlineIdentityPtr Identity = OSS->GetIdentityInterface();
    if (!Identity.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("FetchStats: Identity interface invalid"));
        return;
    }

    // UE5.7 : GetUniqueId() renvoie un FUniqueNetIdRepl
    const FUniqueNetIdRepl ReplId = GetUniqueId();
    if (!ReplId.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("FetchStats: Player has no valid UniqueNetId"));
        return;
    }

    TSharedPtr<const FUniqueNetId> UserId = ReplId.GetUniqueNetId();
    if (!UserId.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("FetchStats: UniqueNetId invalid"));
        return;
    }

    IOnlineStatsPtr Stats = OSS->GetStatsInterface();
    if (!Stats.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("FetchStats: Stats interface unavailable"));
        return;
    }

    // Liste des utilisateurs pour lesquels on demande des stats (ici : le joueur local)
    TArray<FUniqueNetIdRef> Users;
    Users.Add(UserId.ToSharedRef());

    // Liste des noms de stats à demander
    TArray<FString> StatNames;
    StatNames.Add(StatName.ToString());

    // Delegate UE5.7 : FOnlineStatsQueryUsersStatsComplete
    FOnlineStatsQueryUsersStatsComplete Delegate =
        FOnlineStatsQueryUsersStatsComplete::CreateUObject(
            this,
            &AEOSPlayerState::OnStatsQueryComplete,
            StatName  // paramètre supplémentaire passé à la fin
        );

    // Signature UE5.7 :
    // void QueryStats(const FUniqueNetIdRef LocalUserId,
    //                 const TArray<FUniqueNetIdRef>& StatUsers,
    //                 const TArray<FString>& StatNames,
    //                 const FOnlineStatsQueryUsersStatsComplete& Delegate)
    Stats->QueryStats(
        UserId.ToSharedRef(),  // LocalUserId
        Users,                 // StatUsers
        StatNames,             // StatNames
        Delegate               // Delegate
    );

    UE_LOG(LogTemp, Log, TEXT("FetchStats: QueryStats launched for %s"), *StatName.ToString());
}

void AEOSPlayerState::OnStatsQueryComplete(
    const FOnlineError& Result,
    const TArray<TSharedRef<const FOnlineStatsUserStats>>& Users,
    FName RequestedStat)
{
    if (!Result.bSucceeded)
    {
        UE_LOG(LogTemp, Error,
            TEXT("OnStatsQueryComplete: Stats query failed: %s"),
            *Result.ErrorMessage.ToString());
        return;
    }

    if (Users.Num() == 0)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("OnStatsQueryComplete: Query succeeded but returned no users"));
        return;
    }

    // On ne regarde que le premier user (ton joueur local)
    const FOnlineStatsUserStats& Stats = *Users[0];

    // Recherche de la stat demandée par nom
    const FString RequestedKey = RequestedStat.ToString();

    int32 Value = 0;
    if (const FOnlineStatValue* Found = Stats.Stats.Find(RequestedKey))
    {
        // En supposant que la stat soit bien un int
        Found->GetValue(Value);
        CachedKills = Value;

        UE_LOG(LogTemp, Log,
            TEXT("OnStatsQueryComplete: Stat %s = %d"),
            *RequestedKey, Value);
    }
    else
    {
        UE_LOG(LogTemp, Warning,
            TEXT("OnStatsQueryComplete: Stat %s not found"),
            *RequestedKey);
    }
}

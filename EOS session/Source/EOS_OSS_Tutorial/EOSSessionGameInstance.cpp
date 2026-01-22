#include "EOSSessionGameInstance.h"

#include "OnlineSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"

#include "Engine/Engine.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/ConfigCacheIni.h"

static void LogEOSContextAndLoginState(const TCHAR* Where)
{
    IOnlineSubsystem* OSS = IOnlineSubsystem::Get(TEXT("EOS"));
    UE_LOG(LogTemp, Warning, TEXT("[EOSDBG][%s] OSS ptr=%p"), Where, OSS);

    if (!OSS)
    {
        UE_LOG(LogTemp, Error, TEXT("[EOSDBG][%s] OnlineSubsystem EOS is NULL"), Where);
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("[EOSDBG][%s] SubsystemName=%s InstanceName=%s"),
        Where,
        *OSS->GetSubsystemName().ToString(),
        *OSS->GetInstanceName().ToString());

    IOnlineIdentityPtr Identity = OSS->GetIdentityInterface();
    if (!Identity.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("[EOSDBG][%s] IdentityInterface invalid"), Where);
        return;
    }

    const int32 LocalUserNum = 0;

    const ELoginStatus::Type Status = Identity->GetLoginStatus(LocalUserNum);
    TSharedPtr<const FUniqueNetId> UserId = Identity->GetUniquePlayerId(LocalUserNum);

    UE_LOG(LogTemp, Warning, TEXT("[EOSDBG][%s] LoginStatus=%s UserIdValid=%d UserId=%s"),
        Where,
        ELoginStatus::ToString(Status),
        (UserId.IsValid() && UserId->IsValid()) ? 1 : 0,
        (UserId.IsValid() ? *UserId->ToString() : TEXT("<null>")));

    // Log artifact selection hints (INI + command line override)
    FString DefaultArtifactName;
    GConfig->GetString(TEXT("/Script/OnlineSubsystemEOS.EOSSettings"), TEXT("DefaultArtifactName"), DefaultArtifactName, GEngineIni);

    FString CmdArtifactName;
    FParse::Value(FCommandLine::Get(), TEXT("EOSArtifactName="), CmdArtifactName);

    UE_LOG(LogTemp, Warning, TEXT("[EOSDBG][%s] DefaultArtifactName(ini)='%s' EOSArtifactName(cmd)='%s'"),
        Where,
        *DefaultArtifactName,
        *CmdArtifactName);

    // Optional: also log the key EOS IDs you configured (to ensure PIE & packaged point to same deployment)
    FString ProductId, SandboxId, DeploymentId, ClientId;
    GConfig->GetString(TEXT("/Script/OnlineSubsystemEOS.EOSSettings"), TEXT("ProductId"), ProductId, GEngineIni);
    GConfig->GetString(TEXT("/Script/OnlineSubsystemEOS.EOSSettings"), TEXT("SandboxId"), SandboxId, GEngineIni);
    GConfig->GetString(TEXT("/Script/OnlineSubsystemEOS.EOSSettings"), TEXT("DeploymentId"), DeploymentId, GEngineIni);
    GConfig->GetString(TEXT("/Script/OnlineSubsystemEOS.EOSSettings"), TEXT("ClientId"), ClientId, GEngineIni);

    UE_LOG(LogTemp, Warning, TEXT("[EOSDBG][%s] ProductId=%s SandboxId=%s DeploymentId=%s ClientId=%s"),
        Where, *ProductId, *SandboxId, *DeploymentId, *ClientId);
}

namespace
{
    // Valeurs de GameMode utilisées dans les settings réseau
    static const FString Casual = TEXT("QuickMatch");
    static const FString Ranked = TEXT("Ranked");
    static const FString Training = TEXT("Training");
    static const FString ChampionshipMatch = TEXT("ChampionshipMatch");
    //static const FString GAME_MODE_DEATHMATCH = TEXT("Deathmatch");
    //static const FString GAME_MODE_TEAM_ELIMINATION = TEXT("TeamElimination");
    //static const FString GAME_MODE_KOTH = TEXT("KingOfTheHill");
    //static const FString GAME_MODE_TRAINING = TEXT("Training");
    //static const FString GAME_MODE_CHAMPIONSHIP = TEXT("ChampionshipMatch");
}

UEOSSessionGameInstance::UEOSSessionGameInstance()
    : DevAuthHost(TEXT("localhost:8081"))
{
}

FString
UEOSSessionGameInstance::GameModeToString(EMatchGameMode Mode)
{
    switch (Mode)
    {
    case EMatchGameMode::QuickMatch:
        return TEXT("QuickMatch");
    case EMatchGameMode::Ranked:
        return TEXT("Ranked");
    case EMatchGameMode::Training:
        return TEXT("Training");
    case EMatchGameMode::ChampionshipMatch:
        return TEXT("ChampionshipMatch");
    default:
        return TEXT("Unknown");
    }
}


void UEOSSessionGameInstance::Init()
{
    Super::Init();

    IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get();
    if (!Subsystem)
    {
        UE_LOG(LogTemp, Warning, TEXT("No OnlineSubsystem found!"));
        return;
    }

    SessionInterface = Subsystem->GetSessionInterface();
    if (!SessionInterface.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("No SessionInterface found!"));
        return;
    }

    SessionInterface->OnCreateSessionCompleteDelegates.AddUObject(this, &UEOSSessionGameInstance::OnCreateSessionComplete);
    SessionInterface->OnFindSessionsCompleteDelegates.AddUObject(this, &UEOSSessionGameInstance::OnFindSessionsComplete);
    SessionInterface->OnJoinSessionCompleteDelegates.AddUObject(this, &UEOSSessionGameInstance::OnJoinSessionComplete);
}


void UEOSSessionGameInstance::LoginWithDevAuth(const FString& CredentialName)
{
    IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get();
    if (!Subsystem)
    {
        UE_LOG(LogTemp, Warning, TEXT("LoginWithDevAuth: OnlineSubsystem not available"));
        return;
    }

    IOnlineIdentityPtr Identity = Subsystem->GetIdentityInterface();
    if (!Identity.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("LoginWithDevAuth: Identity interface not valid"));
        return;
    }

    if (DevAuthHost.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("LoginWithDevAuth: DevAuthHost is empty. Set it on UEOSSessionGameInstance or change the default C++ value."));
        return;
    }

    // DevAuth: Type = "developer"
    // Id = "host:port" configuré dans DevAuthTool
    // Token = nom du credential (ex: "Renaud", "Johan")
    FOnlineAccountCredentials Credentials(TEXT("developer"), DevAuthHost, CredentialName);

    const int32 LocalUserNum = 0;
    // EOSSessionGameInstance.cpp (dans LoginWithDevAuth)
    LastCredentialName = CredentialName;
    LocalDisplayName = CredentialName; // fallback fiable (DevAuth)
    const FString Nick = Identity->GetPlayerNickname(0);
    if (!Nick.IsEmpty())
    {
        LocalDisplayName = Nick;
    }

    const bool bStarted = Identity->Login(LocalUserNum, Credentials);

    if (!bStarted)
    {
        UE_LOG(LogTemp, Error, TEXT("LoginWithDevAuth: Identity->Login returned false for '%s'"), *CredentialName);
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("LoginWithDevAuth: Login started for '%s'"), *CredentialName);
    }
}

/********************************************************************
 * HOST SESSION
 ********************************************************************/
bool UEOSSessionGameInstance::HostSession(
    const FString& SessionName,
    const FString& MapName,
    const FString& Region,
    const FString& RuleSet,
    bool bIsPrivate,
    int32 PlayersPerTeam)
{
    // On sappuie sur le SessionInterface membre déjà initialisé dans Init()
    if (!SessionInterface.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("HostSession: SessionInterface is not valid"));
        return false;
    }

    // Au cas où une session existe déjà, on la détruit avant de recréer
    if (FNamedOnlineSession* Existing = SessionInterface->GetNamedSession(NAME_GameSession))
    {
        UE_LOG(LogTemp, Warning, TEXT("HostSession: A session already exists, destroying it first"));
        SessionInterface->DestroySession(NAME_GameSession);
        // Pour un vrai jeu, on attendrait OnDestroySessionComplete avant de recréer.
    }

    UE_LOG(LogTemp, Log, TEXT("HostSession: Creating session '%s' on map '%s' (Region=%s, RuleSet=%s, Private=%d, PlayersPerTeam=%d, MaxPlayers=%d)"),
        *SessionName, *MapName, *Region, *RuleSet,
        bIsPrivate ? 1 : 0,
        PlayersPerTeam,
        PlayersPerTeam * 2);

    // Paramètres de session
    FOnlineSessionSettings Settings;

    IOnlineSubsystem* OSS = IOnlineSubsystem::Get();
    const FName SubsystemName = OSS ? OSS->GetSubsystemName() : NAME_None;

    // LAN si on est sur le subsystem NULL
    Settings.bIsLANMatch = (SubsystemName == "NULL");

    Settings.NumPublicConnections = FMath::Clamp(PlayersPerTeam * 2, 2, 64);
    Settings.NumPrivateConnections = 0;

    Settings.bAllowJoinInProgress = true;
    Settings.bAllowJoinViaPresence = true;
    Settings.bShouldAdvertise = !bIsPrivate;
    Settings.bAllowInvites = true;
    Settings.bUsesPresence = true;
    Settings.bUsesStats = false;
    Settings.bIsDedicated = false;
    // Dans HostSession (côté GameInstance / SessionSettings)
    if (!LocalDisplayName.IsEmpty())
    {
        Settings.Set(
            FName(TEXT("HOST_NAME")),
            LocalDisplayName,
            EOnlineDataAdvertisementType::ViaOnlineServiceAndPing
        );
    }

    // Map
    Settings.Set(SETTING_MAPNAME, MapName, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);

    // Région (optionnelle)
    if (!Region.IsEmpty())
    {
        Settings.Set(FName(TEXT("REGION")), Region, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
    }

    // RuleSet / GameMode (optionnel)
    if (!RuleSet.IsEmpty())
    {
        Settings.Set(FName(TEXT("RULESET")), RuleSet, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
    }

    // juste avant CreateSession(...)
    const FString HostNameToAdvertise = !LocalDisplayName.IsEmpty()
        ? LocalDisplayName
        : LastCredentialName;

    if (!HostNameToAdvertise.IsEmpty())
    {
        Settings.Set(FName(TEXT("HOST_NAME")),
            HostNameToAdvertise,
            EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
    }
    
    // Création de la session
    const bool bResult = SessionInterface->CreateSession(0, NAME_GameSession, Settings);

    if (!bResult)
    {
        UE_LOG(LogTemp, Error, TEXT("HostSession: CreateSession failed immediately"));
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("HostSession: CreateSession started. Name=%s, Map=%s, PlayersPerTeam=%d"),
            *SessionName, *MapName, PlayersPerTeam);
    }


    return bResult;
}

bool UEOSSessionGameInstance::HostSession(
    const FString& SessionName,
    const FString& MapName,
    const FString& Region,
    EMatchGameMode GameMode,
    bool bIsPrivate,
    int32 PlayersPerTeam)
{
    const FString RuleSetString = GameModeToString(GameMode);
    return HostSession(SessionName, MapName, Region, RuleSetString, bIsPrivate, PlayersPerTeam);
}

bool UEOSSessionGameInstance::HostSessionWithGameMode(
    const FString& SessionName,
    const FString& MapName,
    const FString& Region,
    EMatchGameMode GameMode,
    bool bIsPrivate,
    int32 PlayersPerTeam)
{
    const FString RuleSetString = GameModeToString(GameMode);
    return HostSession(SessionName, MapName, Region, RuleSetString, bIsPrivate, PlayersPerTeam);
}

void UEOSSessionGameInstance::OnCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
    UE_LOG(LogTemp, Warning, TEXT("OnCreateSessionComplete => %s success=%d"),
        *SessionName.ToString(), bWasSuccessful ? 1 : 0);

    if (!bWasSuccessful)
    {
        return;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        UE_LOG(LogTemp, Error, TEXT("OnCreateSessionComplete: World is null, cannot travel"));
        return;
    }

    // Pour linstant on force une map fixe côté serveur
    const FString MapPath = TEXT("/Game/ThirdPerson/Maps/TestNetMap?listen");
    UE_LOG(LogTemp, Warning, TEXT("OnCreateSessionComplete: ServerTravel to '%s'"), *MapPath);

    World->ServerTravel(MapPath, /*bAbsolute*/ true);
}




/********************************************************************
 * FIND SESSIONS
 ********************************************************************/
bool UEOSSessionGameInstance::FindSessions(int32 MaxResults, bool bWithPresence)
{
    IOnlineSubsystem* OSS = IOnlineSubsystem::Get(TEXT("EOS"));
    if (!OSS)
    {
        UE_LOG(LogTemp, Error, TEXT("[FindSessions] No OnlineSubsystem"));
        return false;
    }

    SessionInterface = OSS->GetSessionInterface();
    if (!SessionInterface.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("[FindSessions] No SessionInterface"));
        return false;
    }

    IOnlineIdentityPtr Identity = OSS ? OSS->GetIdentityInterface() : nullptr;
    if (!Identity.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("[FindSessions] No IdentityInterface"));
        return false;
    }

    TSharedPtr<const FUniqueNetId> UserId = Identity.IsValid() ? Identity->GetUniquePlayerId(0) : nullptr;
    if (!UserId.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("[FindSessions] Invalid UserId (not logged in?)"));
        return false;
    }

    LogEOSContextAndLoginState(TEXT("FindSessions-Before"));

    SessionInterface = OSS ? OSS->GetSessionInterface() : nullptr;

    if (Identity->GetLoginStatus(0) != ELoginStatus::LoggedIn)
    {
        UE_LOG(LogTemp, Error, TEXT("FindSessions: NOT logged in yet -> aborting FindSessions"));
        return false;
    }

    UE_LOG(LogTemp, Warning, TEXT("OSS name = %s"), OSS ? *OSS->GetSubsystemName().ToString() : TEXT("NULL"));

    UE_LOG(LogTemp, Warning, TEXT("Identity valid = %d"), Identity.IsValid());


    UE_LOG(LogTemp, Warning, TEXT("UserId valid = %d"), UserId.IsValid());
    if (UserId.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("UserId = %s"), *UserId->ToString());
    }

    SearchResultsCache.Empty();

    SessionSearch = MakeShareable(new FOnlineSessionSearch());
    SessionSearch->MaxSearchResults = MaxResults;
    SessionSearch->bIsLanQuery = false;

    SessionSearch->QuerySettings.SearchParams.Empty();

    UE_LOG(LogTemp, Warning, TEXT("SessionSearch valid = %d"), SessionSearch.IsValid());
    UE_LOG(LogTemp, Warning, TEXT("bIsLanQuery = %d"), SessionSearch->bIsLanQuery);
    UE_LOG(LogTemp, Warning, TEXT("MaxSearchResults = %d"), SessionSearch->MaxSearchResults);

    //SessionSearch->QuerySettings.Set(FName(TEXT("PRESENCE")), bWithPresence, EOnlineComparisonOp::Equals);

    UE_LOG(LogTemp, Warning, TEXT("Searching sessions..."));
    return SessionInterface->FindSessions(*UserId, SessionSearch.ToSharedRef());;
}


void UEOSSessionGameInstance::OnFindSessionsComplete(bool bWasSuccessful)
{
    UE_LOG(LogTemp, Warning, TEXT("OnFindSessionsComplete success=%d"), bWasSuccessful);

    SearchResultsCache.Reset();
    FilteredToRawIndex.Reset();
    RankedIndexes.Reset();
    EffectiveSessionCount = 0;

    if (bWasSuccessful && SessionSearch.IsValid())
    {
        // 1) Cache brut
        for (const FOnlineSessionSearchResult& R : SessionSearch->SearchResults)
        {
            SearchResultsCache.Add(R);
        }

        // 2) Par défaut : pas de filtre => tout passe
        const int32 N = SearchResultsCache.Num();
        FilteredToRawIndex.Reserve(N);
        RankedIndexes.Reserve(N);

        for (int32 i = 0; i < N; ++i)
        {
            FilteredToRawIndex.Add(i);
            RankedIndexes.Add(i);
        }

        // 3) Par défaut : l'UI peut afficher Ranked, sinon Filtered
        EffectiveSessionCount = N;

        UE_LOG(LogTemp, Warning, TEXT("[FindSessions] Cached=%d Effective=%d"), N, EffectiveSessionCount);

        // Dump optionnel
        for (int32 i = 0; i < N; ++i)
        {
            const auto& Res = SearchResultsCache[i];
            const auto& S = Res.Session;
            FString MapName;
            S.SessionSettings.Get(SETTING_MAPNAME, MapName);

            UE_LOG(LogTemp, Warning,
                TEXT("[FindSessions] #%d Owner=%s Id=%s Ping=%d OpenPub=%d/%d Map=%s"),
                i, *S.OwningUserName, *S.GetSessionIdStr(), Res.PingInMs,
                S.NumOpenPublicConnections, S.SessionSettings.NumPublicConnections,
                *MapName);
        }
    }

    // 4) Signal UI (Blueprint)
    OnSessionsSearchUpdated.Broadcast(EffectiveSessionCount);
}

void UEOSSessionGameInstance::LeaveSession()
{
    if (!SessionInterface.IsValid()) return;
    SessionInterface->DestroySession(NAME_GameSession);
}

void UEOSSessionGameInstance::BuildFilteredIndex(const FSessionClientFilterParams& InFilters)
{
    FilteredToRawIndex.Reset();

    const int32 RawCount = SearchResultsCache.Num();
    if (RawCount <= 0)
    {
        EffectiveSessionCount = 0;
        return;
    }

    FilteredToRawIndex.Reserve(RawCount);

    // Clé optionnelle pour RuleSet (si tu l'utilises vraiment côté session settings)
    static const FName KEY_RULESET(TEXT("RULESET"));

    for (int32 RawIdx = 0; RawIdx < RawCount; ++RawIdx)
    {
        const FOnlineSessionSearchResult& Result = SearchResultsCache[RawIdx];
        const FOnlineSession& Session = Result.Session;
        const FOnlineSessionSettings& Settings = Session.SessionSettings;

        // 1) Filtre LAN
        if (InFilters.bFilterLANOnly && !Settings.bIsLANMatch)
        {
            continue;
        }

        // 2) Filtre "Dedicated"
        if (InFilters.bFilterDedicatedOnly)
        {
            bool bDedicated = false;
            Settings.Get(SETTING_DEDICATED, bDedicated);
            if (!bDedicated)
            {
                continue;
            }
        }

        // 3) Filtre sur les slots libres (A/B = public / privé)
        if (InFilters.bRequireFreeSlots)
        {
            const int32 OpenPublic = Session.NumOpenPublicConnections;
            const int32 OpenPrivate = Session.NumOpenPrivateConnections;

            if (InFilters.MinFreeSlotsA > 0 && OpenPublic < InFilters.MinFreeSlotsA)
            {
                continue;
            }

            if (InFilters.MinFreeSlotsB > 0 && OpenPrivate < InFilters.MinFreeSlotsB)
            {
                continue;
            }
        }

        // 4) Filtre carte (MapName stocké dans SETTING_MAPNAME)
        if (InFilters.bFilterMap && !InFilters.MapName.IsEmpty())
        {
            FString SessionMap;
            if (Settings.Get(SETTING_MAPNAME, SessionMap))
            {
                // Comparaison simple par "contains" (tu pourras raffiner en equals/startsWith)
                if (!SessionMap.Contains(InFilters.MapName, ESearchCase::IgnoreCase))
                {
                    continue;
                }
            }
            else
            {
                // Pas de nom de map dans la session alors que le filtre est actif -> on élimine
                continue;
            }
        }

        // 5) Filtre RuleSet (si tu l'utilises)
        if (InFilters.bFilterRuleSet && !InFilters.RuleSet.IsEmpty())
        {
            FString SessionRuleSet;
            if (Settings.Get(KEY_RULESET, SessionRuleSet))
            {
                if (!SessionRuleSet.Contains(InFilters.RuleSet, ESearchCase::IgnoreCase))
                {
                    continue;
                }
            }
            else
            {
                continue;
            }
        }

        // 6) Filtre sur le nombre minimum de joueurs présents
        if (InFilters.MinPresentPlayers > 0)
        {
            const int32 TotalSlots =
                Settings.NumPublicConnections +
                Settings.NumPrivateConnections;

            const int32 OpenSlots =
                Session.NumOpenPublicConnections +
                Session.NumOpenPrivateConnections;

            const int32 PresentPlayers = FMath::Max(TotalSlots - OpenSlots, 0);

            if (PresentPlayers < InFilters.MinPresentPlayers)
            {
                continue;
            }
        }

        // 7) Filtre sur l'âge de la session
        // (on n'a pas de timestamp standard dans FOnlineSessionSearchResult,
        //  donc on ne filtre QUE si tu as toi-même stocké un float/double
        //  "AgeMinutes" dans les settings; sinon ce filtre est neutre)
        if (InFilters.MaxAgeMinutes < 9999.f)
        {
            float AgeMinutes = 0.f;
            static const FName KEY_AGE(TEXT("AgeMinutes"));
            if (Settings.Get(KEY_AGE, AgeMinutes))
            {
                if (AgeMinutes > InFilters.MaxAgeMinutes)
                {
                    continue;
                }
            }
            // Si pas de valeur, on considère "inconnu" -> on laisse passer.
        }

        // 8) Pour l'instant on ignore bMatchRegionStrict et bFilterFriendsOnly
        //    (nécessiterait plus de plumbing avec EOS + OnlineFriends)

        FilteredToRawIndex.Add(RawIdx);
    }

    EffectiveSessionCount = FilteredToRawIndex.Num();
}




void UEOSSessionGameInstance::RankAndSortFiltered(const FSessionRankingWeights& Weights)
{
    RankedIndexes = FilteredToRawIndex;

    const int32 Count = RankedIndexes.Num();
    if (Count <= 0)
    {
        EffectiveSessionCount = 0;
        return;
    }

    // Tri stable pour garder un ordre déterministe à score égal
    RankedIndexes.StableSort(
        [this, &Weights](int32 A, int32 B)
        {
            const float ScoreA = GetScoreForIndex(A, Weights);
            const float ScoreB = GetScoreForIndex(B, Weights);

            if (!FMath::IsNearlyEqual(ScoreA, ScoreB))
            {
                // Score le plus élevé en premier
                return ScoreA > ScoreB;
            }
            // Égalité -> index plus petit d'abord
            return A < B;
        });

    EffectiveSessionCount = RankedIndexes.Num();
}


float UEOSSessionGameInstance::GetScoreForIndex(int32 RawIndex, const FSessionRankingWeights& Weights) const
{
    if (!SearchResultsCache.IsValidIndex(RawIndex))
    {
        return 0.f;
    }

    const FOnlineSessionSearchResult& Result = SearchResultsCache[RawIndex];
    const FOnlineSession& Session = Result.Session;
    const FOnlineSessionSettings& Settings = Session.SessionSettings;

    // 1) Région : pour linstant, neutre (score 1 partout)
    float RegionScore = 1.f;

    // 2) Map : si on a un nom de map, on met 1, sinon 0.5 (neutre-ish)
    float MapScore = 0.5f;
    {
        FString SessionMap;
        if (Settings.Get(SETTING_MAPNAME, SessionMap) && !SessionMap.IsEmpty())
        {
            MapScore = 1.f;
        }
    }

    // 3) RuleSet : même logique que la map
    float RuleScore = 0.5f;
    {
        FString SessionRuleSet;
        static const FName KEY_RULESET(TEXT("RULESET"));
        if (Settings.Get(KEY_RULESET, SessionRuleSet) && !SessionRuleSet.IsEmpty())
        {
            RuleScore = 1.f;
        }
    }

    // 4) Ping : 1 = ping parfait, 0 = ping pourri (> 500 ms)
    float PingScore = 0.f;
    {
        const float PingMs = static_cast<float>(Result.PingInMs);
        const float ClampedPing = FMath::Clamp(PingMs, 1.f, 500.f);
        PingScore = 1.f - (ClampedPing - 1.f) / (500.f - 1.f);
    }

    // 5) Occupation / slots : plus il y a de joueurs (sans être full), mieux cest
    float SlotsScore = 0.f;
    {
        const int32 TotalSlots =
            Settings.NumPublicConnections +
            Settings.NumPrivateConnections;

        const int32 OpenSlots =
            Session.NumOpenPublicConnections +
            Session.NumOpenPrivateConnections;

        if (TotalSlots > 0)
        {
            const int32 PresentPlayers = FMath::Max(TotalSlots - OpenSlots, 0);
            // 0 -> 0, full -> 1
            SlotsScore = static_cast<float>(PresentPlayers) / static_cast<float>(TotalSlots);
        }
    }

    // 6) FriendScore : pour linstant on ne sait pas encore identifier les amis -> 0
    float FriendScore = 0.f;

    // 7) AgeScore : sans timestamp, on approxime "plus récent = plus grand RawIndex"
    float AgeScore = 0.5f;
    {
        const int32 RawCount = SearchResultsCache.Num();
        if (RawCount > 0)
        {
            AgeScore = static_cast<float>(RawIndex + 1) / static_cast<float>(RawCount);
        }
    }

    const float WeightedSum =
        Weights.w_region * RegionScore
        + Weights.w_map * MapScore
        + Weights.w_rules * RuleScore
        + Weights.w_ping * PingScore
        + Weights.w_slots * SlotsScore
        + Weights.w_friends * FriendScore
        + Weights.w_age * AgeScore;

    const float Normalizer =
        Weights.w_region +
        Weights.w_map +
        Weights.w_rules +
        Weights.w_ping +
        Weights.w_slots +
        Weights.w_friends +
        Weights.w_age;

    if (Normalizer <= KINDA_SMALL_NUMBER)
    {
        return 0.f;
    }

    return WeightedSum / Normalizer;
}



/********************************************************************
 * JOIN
 ********************************************************************/
bool UEOSSessionGameInstance::JoinSessionByRawIndex(int32 RawIndex)
{
    if (!SessionInterface.IsValid())
        return false;

    if (!SearchResultsCache.IsValidIndex(RawIndex))
        return false;

    return SessionInterface->JoinSession(0, NAME_GameSession, SearchResultsCache[RawIndex]);
}


void UEOSSessionGameInstance::OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
    UE_LOG(LogTemp, Warning, TEXT("OnJoinSessionComplete: %s Result=%s"),
        *SessionName.ToString(),
        LexToString(Result));

    if (Result != EOnJoinSessionCompleteResult::Success)
    {
        UE_LOG(LogTemp, Error, TEXT("Join failed: %s"), LexToString(Result));
        return;
    }

    IOnlineSubsystem* OSS = IOnlineSubsystem::Get();
    if (!OSS) return;

    IOnlineSessionPtr Sessions = OSS->GetSessionInterface();
    if (!Sessions.IsValid()) return;

    FString ConnectString;
    if (!Sessions->GetResolvedConnectString(SessionName, ConnectString) || ConnectString.IsEmpty())
    {
        UE_LOG(LogTemp, Error, TEXT("GetResolvedConnectString failed (empty)."));
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("Resolved ConnectString=%s"), *ConnectString);

    UWorld* World = GetWorld();
    if (!World) return;

    APlayerController* PC = World->GetFirstPlayerController();
    if (!PC) return;

    PC->ClientTravel(ConnectString, ETravelType::TRAVEL_Absolute);
}

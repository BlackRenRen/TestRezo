#include "EOSSessionGameInstance.h"

#include "SessionRowData.h"

#include "Kismet/GameplayStatics.h"
#include "OnlineSubsystem.h"
#include "OnlineSessionSettings.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/ConfigCacheIni.h"

namespace
{
    // Use explicit keys to avoid build breaks when Engine constants move.
    const FName KeyPresenceSearch(TEXT("PRESENCESEARCH"));
    const FName KeySessionName(TEXT("SESSION_NAME"));
    const FName KeyMapName(TEXT("MAPNAME"));
    const FName KeyMapFallback(TEXT("MAP"));
    const FName KeyHostName(TEXT("HOST_NAME"));
    const FName KeyRegion(TEXT("REGION"));
    const FName KeyRuleSet(TEXT("RULESET"));
    const FName KeyIsPrivate(TEXT("IS_PRIVATE"));
}

UEOSSessionGameInstance::UEOSSessionGameInstance()
{
}

void UEOSSessionGameInstance::Init()
{
    Super::Init();
    EnsureOnlineInterfaces();
}

void UEOSSessionGameInstance::EnsureOnlineInterfaces()
{
    if (SessionInterface.IsValid() && IdentityInterface.IsValid())
    {
        return;
    }

    // In editor, the default subsystem can remain "NULL" even when EOS is enabled.
    // Prefer grabbing the EOS subsystem explicitly to avoid silently routing Identity/Login to NULL.
    IOnlineSubsystem* OSS = IOnlineSubsystem::Get(TEXT("EOS"));
    if (!OSS)
    {
        OSS = IOnlineSubsystem::Get();
    }
    if (!OSS)
    {
        SessionInterface.Reset();
        IdentityInterface.Reset();
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("EnsureOnlineInterfaces: Using OSS=%s"), *OSS->GetSubsystemName().ToString());

    SessionInterface = OSS->GetSessionInterface();
    IdentityInterface = OSS->GetIdentityInterface();
}

bool UEOSSessionGameInstance::LoginWithDevAuth(const FString& CredentialName)
{
    // Compatibility overload to satisfy stale UHT wrappers that call LoginWithDevAuth(CredentialName).
    return LoginWithDevAuth(true, CredentialName);
}

bool UEOSSessionGameInstance::LoginWithDevAuth(bool bDevAuthId, const FString& CredentialName)
{
    EnsureOnlineInterfaces();
    if (!IdentityInterface.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("LoginWithDevAuth: IdentityInterface invalid"));
        return false;
    }

    const ELoginStatus::Type StatusBefore = IdentityInterface->GetLoginStatus(0);
    UE_LOG(LogTemp, Log, TEXT("LoginWithDevAuth: LoginStatus before=%d"), (int32)StatusBefore);
    if (StatusBefore == ELoginStatus::LoggedIn)
    {
        UE_LOG(LogTemp, Log, TEXT("LoginWithDevAuth: already logged in"));
        return true;
    }

    IdentityInterface->ClearOnLoginCompleteDelegates(0, this);
    IdentityInterface->AddOnLoginCompleteDelegate_Handle(
        0,
        FOnLoginCompleteDelegate::CreateUObject(this, &UEOSSessionGameInstance::OnLoginComplete)
    );

    // DevAuthTool login (EOS_LCT_Developer):
    // - Id    = host:port where DevAuthTool listens (usually localhost:6300)
    // - Token = credential name you created inside DevAuthTool (e.g. "Renaud")
    // Ref: Epic docs for Dev Auth Tool login.
    FString CredentialToken = CredentialName;
    CredentialToken.TrimStartAndEndInline();

    if (CredentialToken.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("LoginWithDevAuth: CredentialName is empty/whitespace (this becomes EOS_Auth_Credentials.Token)"));
        return false;
    }

    FString DevAuthAddress = TEXT("127.0.0.1:8081");

// Optional override via config (preferred for PIE):
//   DefaultEngine.ini:
//     [EOS_OSS_Tutorial]
//     DevAuthAddr=127.0.0.1:8081
if (GConfig)
{
    FString IniAddr;
    if (GConfig->GetString(TEXT("EOS_OSS_Tutorial"), TEXT("DevAuthAddr"), IniAddr, GEngineIni) ||
        GConfig->GetString(TEXT("EOS_OSS_Tutorial"), TEXT("DevAuthAddr"), IniAddr, GGameIni))
    {
        IniAddr.TrimStartAndEndInline();
        if (!IniAddr.IsEmpty())
        {
            DevAuthAddress = IniAddr;
        }
    }
}

// Optional override via command line:
//   -DevAuthAddr=localhost:8081
//   -DevAuthToolAddress=localhost:8081
// (Keep both to be robust across naming conventions.)
FParse::Value(FCommandLine::Get(), TEXT("DevAuthAddr="), DevAuthAddress);
FParse::Value(FCommandLine::Get(), TEXT("DevAuthToolAddress="), DevAuthAddress);

// Accept "http://host:port" / "https://host:port" and normalize to "host:port".
DevAuthAddress.ReplaceInline(TEXT("http://"), TEXT(""), ESearchCase::IgnoreCase);
DevAuthAddress.ReplaceInline(TEXT("https://"), TEXT(""), ESearchCase::IgnoreCase);
DevAuthAddress.TrimStartAndEndInline();

    FOnlineAccountCredentials Creds;
    Creds.Type  = TEXT("developer");
    Creds.Id    = DevAuthAddress;
    Creds.Token = CredentialToken;

    UE_LOG(LogTemp, Log, TEXT("LoginWithDevAuth: Type=%s Id=%s Token=%s"), *Creds.Type, *Creds.Id, *Creds.Token);

    IdentityInterface->Login(0, Creds);

    UE_LOG(LogTemp, Log, TEXT("LoginWithDevAuth: Login request issued"));

    return true;
}

void UEOSSessionGameInstance::OnLoginComplete(int32 LocalUserNum, bool bWasSuccessful, const FUniqueNetId& UserId, const FString& Error)
{
    if (bWasSuccessful)
    {
        UE_LOG(LogTemp, Log, TEXT("EOS Login successful (LocalUserNum=%d)"), LocalUserNum);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("EOS Login FAILED: %s"), *Error);
    }
}

void UEOSSessionGameInstance::FindSessions(int32 MaxResults)
{
    EnsureOnlineInterfaces();
    if (!SessionInterface.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("FindSessions: SessionInterface invalid"));
        return;
    }

    SessionSearch = MakeShared<FOnlineSessionSearch>();
    SessionSearch->MaxSearchResults = FMath::Clamp(MaxResults, 1, 5000);
    SessionSearch->QuerySettings.Set(KeyPresenceSearch, true, EOnlineComparisonOp::Equals);

    SessionInterface->ClearOnFindSessionsCompleteDelegates(this);
    SessionInterface->AddOnFindSessionsCompleteDelegate_Handle(
        FOnFindSessionsCompleteDelegate::CreateUObject(this, &UEOSSessionGameInstance::OnFindSessionsComplete)
    );

    const bool bStarted = SessionInterface->FindSessions(0, SessionSearch.ToSharedRef());
    if (!bStarted)
    {
        UE_LOG(LogTemp, Warning, TEXT("FindSessions: FindSessions() failed to start"));
    }
}

void UEOSSessionGameInstance::OnFindSessionsComplete(bool bWasSuccessful)
{
    if (!bWasSuccessful || !SessionSearch.IsValid())
    {
        CachedItems.Reset();
        OnSessionsSearchUpdated.Broadcast(0);
        return;
    }

    RebuildCachedItems();
    OnSessionsSearchUpdated.Broadcast(CachedItems.Num());
}

void UEOSSessionGameInstance::RebuildCachedItems()
{
    CachedItems.Reset();

    if (!SessionSearch.IsValid())
    {
        return;
    }

    const TArray<FOnlineSessionSearchResult>& Results = SessionSearch->SearchResults;
    CachedItems.Reserve(Results.Num());

    for (int32 Index = 0; Index < Results.Num(); ++Index)
    {
        const FOnlineSessionSearchResult& Result = Results[Index];

        USessionRowData* Item = NewObject<USessionRowData>(this);
        Item->SearchResultIndex = Index;

        Item->SessionId = Result.GetSessionIdStr();
        Item->HostName = Result.Session.OwningUserName;
        // FOnlineSession (UE 5.7) does not expose a SessionName field.
        // We store a human-readable name in custom settings when hosting.
        FString HumanName;
        if (Result.Session.SessionSettings.Get(KeySessionName, HumanName) && !HumanName.IsEmpty())
        {
            Item->SessionName = HumanName;
        }
        else
        {
            // Fallbacks: host name then session id.
            Item->SessionName = !Item->HostName.IsEmpty() ? Item->HostName : Item->SessionId;
        }
        Item->PingMs = Result.PingInMs;

        // Slots
        const int32 MaxPublic = Result.Session.SessionSettings.NumPublicConnections;
        const int32 OpenPublic = Result.Session.NumOpenPublicConnections;
        Item->MaxPublicSlots = MaxPublic;
        Item->OpenPublicSlots = OpenPublic;
        Item->MaxPlayers = MaxPublic;
        Item->CurrentPlayers = FMath::Max(0, MaxPublic - OpenPublic);

        // Custom settings
        FString Map;
        if (Result.Session.SessionSettings.Get(KeyMapName, Map) || Result.Session.SessionSettings.Get(KeyMapFallback, Map))
        {
            Item->MapName = Map;
        }

        FString HostFromSetting;
        if (Result.Session.SessionSettings.Get(KeyHostName, HostFromSetting))
        {
            Item->HostName = HostFromSetting;
        }

        FString Region;
        if (Result.Session.SessionSettings.Get(KeyRegion, Region))
        {
            Item->Region = Region;
        }

        FString RuleSet;
        if (Result.Session.SessionSettings.Get(KeyRuleSet, RuleSet))
        {
            Item->RuleSet = RuleSet;
        }

        bool bPrivate = false;
        if (Result.Session.SessionSettings.Get(KeyIsPrivate, bPrivate))
        {
            Item->bIsPrivate = bPrivate;
        }

        CachedItems.Add(Item);
    }
}

bool UEOSSessionGameInstance::HostSession(int32 NumPublicConnections, const FString& MapName, bool bIsPrivate)
{
    // Compatibility overload to satisfy stale UHT wrappers that call HostSession(NumPublicConnections, MapName, bIsPrivate).
    // We map the legacy call to the richer Blueprint API signature using cached defaults.

    const FString SessionName = !LastHostSessionName.IsEmpty() ? LastHostSessionName : TEXT("Session");
    const FString Region = !LastHostRegion.IsEmpty() ? LastHostRegion : TEXT("EU");
    const FString RuleSet = !LastHostRuleSet.IsEmpty() ? LastHostRuleSet : TEXT("Default");

    // Legacy meaning is 'public connections'. Our BP API exposes PlayersPerTeam;
    // in absence of better info, reuse the value.
    const int32 PlayersPerTeam = FMath::Max(1, NumPublicConnections);

    return HostSession(SessionName, MapName, Region, RuleSet, bIsPrivate, PlayersPerTeam);
}

bool UEOSSessionGameInstance::HostSessionAdvanced(const FString& SessionName, const FString& MapName, const FString& Region, const FString& RuleSet, bool bIsPrivate, int32 PlayersPerTeam)
{
    // Stable Blueprint API: distinct name, no overload ambiguity.
    // We delegate to the core implementation.
    return HostSession(SessionName, MapName, Region, RuleSet, bIsPrivate, PlayersPerTeam);
}

bool UEOSSessionGameInstance::HostSession(const FString& SessionName, const FString& MapName, const FString& Region, const FString& RuleSet, bool bIsPrivate, int32 PlayersPerTeam)
{
    EnsureOnlineInterfaces();
    if (!SessionInterface.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("HostSession: SessionInterface invalid"));
        return false;
    }

    // Clean existing session if any
    if (SessionInterface->GetNamedSession(NAME_GameSession) != nullptr)
    {
        SessionInterface->DestroySession(NAME_GameSession);
    }

    bLastHostWasPrivate = bIsPrivate;
    LastHostSessionName = SessionName;
    LastHostMap = MapName;
    LastHostRegion = Region;
    LastHostRuleSet = RuleSet;
    LastHostPlayersPerTeam = PlayersPerTeam;

    FOnlineSessionSettings Settings;
    Settings.bIsLANMatch = false;
    Settings.bIsDedicated = false;
    Settings.NumPublicConnections = FMath::Max(1, PlayersPerTeam);
    Settings.bShouldAdvertise = true;
    Settings.bAllowJoinInProgress = true;
    Settings.bAllowJoinViaPresence = true;
    Settings.bUsesPresence = true;
    Settings.bUseLobbiesIfAvailable = true;

    Settings.Set(KeySessionName, SessionName, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
    Settings.Set(KeyMapName, MapName, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
    Settings.Set(KeyMapFallback, MapName, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
    Settings.Set(KeyRegion, Region, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
    Settings.Set(KeyRuleSet, RuleSet, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
    Settings.Set(KeyIsPrivate, bIsPrivate, EOnlineDataAdvertisementType::ViaOnlineService);

    // Friendly host name
    FString HostName = TEXT("Host");
    if (IdentityInterface.IsValid())
    {
        TSharedPtr<const FUniqueNetId> UserId = IdentityInterface->GetUniquePlayerId(0);
        if (UserId.IsValid())
        {
            HostName = IdentityInterface->GetPlayerNickname(*UserId);
        }
    }
    Settings.Set(KeyHostName, HostName, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);

    SessionInterface->ClearOnCreateSessionCompleteDelegates(this);
    SessionInterface->AddOnCreateSessionCompleteDelegate_Handle(
        FOnCreateSessionCompleteDelegate::CreateUObject(this, &UEOSSessionGameInstance::OnCreateSessionComplete)
    );

    const bool bStarted = SessionInterface->CreateSession(0, NAME_GameSession, Settings);
    if (!bStarted)
    {
        UE_LOG(LogTemp, Warning, TEXT("HostSession: CreateSession() failed to start"));
        return false;
    }

    return true;
}

void UEOSSessionGameInstance::OnCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
    if (!bWasSuccessful)
    {
        UE_LOG(LogTemp, Warning, TEXT("CreateSession failed"));
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("CreateSession ok: %s"), *SessionName.ToString());

    // Server travel (listen)
    if (UWorld* World = GetWorld())
    {
        const FString TravelURL = LastHostMap + TEXT("?listen");
        World->ServerTravel(TravelURL);
    }
}

void UEOSSessionGameInstance::JoinSessionByIndex(int32 SearchResultIndex)
{
    EnsureOnlineInterfaces();
    if (!SessionInterface.IsValid() || !SessionSearch.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("JoinSessionByIndex: invalid interfaces/search"));
        return;
    }

    const TArray<FOnlineSessionSearchResult>& Results = SessionSearch->SearchResults;
    if (!Results.IsValidIndex(SearchResultIndex))
    {
        UE_LOG(LogTemp, Warning, TEXT("JoinSessionByIndex: invalid index %d"), SearchResultIndex);
        return;
    }

    SessionInterface->ClearOnJoinSessionCompleteDelegates(this);
    SessionInterface->AddOnJoinSessionCompleteDelegate_Handle(
        FOnJoinSessionCompleteDelegate::CreateUObject(this, &UEOSSessionGameInstance::OnJoinSessionComplete)
    );

    const bool bStarted = SessionInterface->JoinSession(0, NAME_GameSession, Results[SearchResultIndex]);
    if (!bStarted)
    {
        UE_LOG(LogTemp, Warning, TEXT("JoinSessionByIndex: JoinSession() failed to start"));
    }
}

void UEOSSessionGameInstance::JoinSessionByItem(USessionRowData* Item)
{
    if (!Item)
    {
        return;
    }
    JoinSessionByIndex(Item->SearchResultIndex);
}

void UEOSSessionGameInstance::OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
    if (Result != EOnJoinSessionCompleteResult::Success)
    {
        UE_LOG(LogTemp, Warning, TEXT("JoinSession failed (%d)"), (int32)Result);
        return;
    }

    FString ConnectString;
    if (!SessionInterface.IsValid() || !SessionInterface->GetResolvedConnectString(SessionName, ConnectString))
    {
        UE_LOG(LogTemp, Warning, TEXT("JoinSession: Could not resolve connect string"));
        return;
    }

    APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
    if (!PC)
    {
        UE_LOG(LogTemp, Warning, TEXT("JoinSession: No PlayerController"));
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("ClientTravel -> %s"), *ConnectString);
    PC->ClientTravel(ConnectString, TRAVEL_Absolute);
}

TArray<USessionRowData*> UEOSSessionGameInstance::GetSessionListItems() const
{
    TArray<USessionRowData*> Out;
    Out.Reserve(CachedItems.Num());
    for (const TObjectPtr<USessionRowData>& Item : CachedItems)
    {
        Out.Add(Item.Get());
    }
    return Out;
}

FString UEOSSessionGameInstance::GameModeToString(EMatchGameMode Mode) const
{
    switch (Mode)
    {
    case EMatchGameMode::QuickMatch:
        return TEXT("QuickMatch");
    case EMatchGameMode::Training:
        return TEXT("Training");
    case EMatchGameMode::ChampionshipMatch:
        return TEXT("ChampionshipMatch");
    default:
        return TEXT("Unknown");
    }
}

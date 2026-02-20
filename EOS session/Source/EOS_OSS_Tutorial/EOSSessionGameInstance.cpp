#include "EOSSessionGameInstance.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "Misc/ConfigCacheIni.h"

namespace
{
	static IOnlineSubsystem* GetOSSForWorld(const UObject* WorldContextObject)
	{
		if (WorldContextObject)
		{
			if (const UWorld* World = WorldContextObject->GetWorld())
			{
				if (IOnlineSubsystem* Subsystem = Online::GetSubsystem(World))
				{
					return Subsystem;
				}
			}
		}
		return IOnlineSubsystem::Get();
	}

	static bool ShouldFallbackToAccountPortalFromDevAuthError(const FString& Error)
	{
		// Be conservative: only fallback when it looks like a connectivity/service issue (e.g. DevAuthTool not running).
		return Error.Contains(TEXT("EOS_NoConnection")) ||
		       Error.Contains(TEXT("EOS_ServiceFailure")) ||
		       Error.Contains(TEXT("EOS_TimedOut")) ||
		       Error.Contains(TEXT("EOS_TooManyRequests"));
	}
}


namespace
{
	static const FName NAME_EOSGameSession(TEXT("GameSession"));
static const FName KEY_MAPNAME(TEXT("MAPNAME"));
static const FName KEY_DISPLAY_SESSION_NAME(TEXT("DISPLAY_SESSION_NAME"));
	static const FName KEY_REGION(TEXT("REGION"));
	static const FName KEY_RULESET(TEXT("RULESET"));
}

UEOSSessionGameInstance::UEOSSessionGameInstance()
{
}

void UEOSSessionGameInstance::Init()
{
	Super::Init();

	IOnlineSubsystem* Subsystem = GetOSSForWorld(this);
	SessionInterface = Subsystem ? Subsystem->GetSessionInterface() : nullptr;
	IdentityInterface = Subsystem ? Subsystem->GetIdentityInterface() : nullptr;

	// Cache DevAuth settings (read from [OnlineSubsystemEOS] in DefaultEngine.ini).
	bUseDevAuthConfig = false;
	DevAuthToolAddressConfig = TEXT("127.0.0.1:8081");
	if (GConfig)
	{
		GConfig->GetBool(TEXT("OnlineSubsystemEOS"), TEXT("bUseDevAuth"), bUseDevAuthConfig, GEngineIni);

		FString Addr;
		if (GConfig->GetString(TEXT("OnlineSubsystemEOS"), TEXT("DevAuthToolAddress"), Addr, GEngineIni) && !Addr.IsEmpty())
		{
			DevAuthToolAddressConfig = Addr;
		}
	}

	UE_LOG(LogTemp, Log,
		TEXT("EOSSessionGameInstance::Init - Subsystem=%s Session=%s Identity=%s DevAuthEnabled=%d DevAuthAddr=%s"),
		Subsystem ? *Subsystem->GetSubsystemName().ToString() : TEXT("null"),
		SessionInterface.IsValid() ? TEXT("OK") : TEXT("null"),
		IdentityInterface.IsValid() ? TEXT("OK") : TEXT("null"),
		bUseDevAuthConfig ? 1 : 0,
		*DevAuthToolAddressConfig);
}

void UEOSSessionGameInstance::Shutdown()
{
	ClearAllSessionDelegates();

	if (IdentityInterface.IsValid() && OnLoginCompleteHandle.IsValid())
	{
		IdentityInterface->ClearOnLoginCompleteDelegate_Handle(0, OnLoginCompleteHandle);
		OnLoginCompleteHandle.Reset();
	}

	Super::Shutdown();
}

void UEOSSessionGameInstance::ClearAllSessionDelegates()
{
	if (!SessionInterface.IsValid())
	{
		return;
	}

	if (OnFindSessionsCompleteHandle.IsValid())
	{
		SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(OnFindSessionsCompleteHandle);
		OnFindSessionsCompleteHandle.Reset();
	}
	if (OnCreateSessionCompleteHandle.IsValid())
	{
		SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(OnCreateSessionCompleteHandle);
		OnCreateSessionCompleteHandle.Reset();
	}
	if (OnDestroySessionCompleteHandle.IsValid())
	{
		SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(OnDestroySessionCompleteHandle);
		OnDestroySessionCompleteHandle.Reset();
	}
	if (OnJoinSessionCompleteHandle.IsValid())
	{
		SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(OnJoinSessionCompleteHandle);
		OnJoinSessionCompleteHandle.Reset();
	}
}

// ----------------------
// Login
// ----------------------

bool UEOSSessionGameInstance::LoginPreferDevAuth(const FString& CredentialName)
{
	if (!IdentityInterface.IsValid())
	{
		if (IOnlineSubsystem* Subsystem = GetOSSForWorld(this))
		{
			IdentityInterface = Subsystem->GetIdentityInterface();
		}
	}

	if (!IdentityInterface.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("LoginPreferDevAuth: IdentityInterface invalid"));
		return false;
	}

	const int32 LocalUserNum = 0;
	const ELoginStatus::Type Status = IdentityInterface->GetLoginStatus(LocalUserNum);
	if (Status == ELoginStatus::LoggedIn)
	{
		UE_LOG(LogTemp, Log, TEXT("LoginPreferDevAuth: already logged in"));
		return true;
	}

	if (bLoginInProgress)
	{
		UE_LOG(LogTemp, Warning, TEXT("LoginPreferDevAuth: login already in progress"));
		return false;
	}

	PendingCredentialName = CredentialName;

	// Prefer DevAuth when configured. Only fallback to AccountPortal when the error looks like a connectivity/service issue.
	bLoginFallbackAllowed = bUseDevAuthConfig;

	return bUseDevAuthConfig
		? LoginWithDevAuth(true, CredentialName)
		: LoginWithDevAuth(false, CredentialName);
}

bool UEOSSessionGameInstance::LoginWithDevAuth_Legacy(const FString& CredentialName)
{
	// The UI button "Login as X" should do the preferred logic.
	return LoginPreferDevAuth(CredentialName);
}

bool UEOSSessionGameInstance::LoginWithDevAuth(bool bDevAuthId, const FString& CredentialName)
{
	if (!IdentityInterface.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("LoginWithDevAuth: IdentityInterface invalid"));
		return false;
	}

	if (bLoginInProgress)
	{
		UE_LOG(LogTemp, Warning, TEXT("LoginWithDevAuth: login already in progress"));
		return false;
	}

	// Safety: if DevAuth is enabled in config, force DevAuth unless you explicitly want AccountPortal in code.
	if (!bDevAuthId && bUseDevAuthConfig)
	{
		UE_LOG(LogTemp, Warning, TEXT("LoginWithDevAuth: bDevAuthId=false while bUseDevAuth=true; forcing DevAuth"));
		bDevAuthId = true;
	}

	const int32 LocalUserNum = 0;

	if (IdentityInterface->GetLoginStatus(LocalUserNum) == ELoginStatus::LoggedIn)
	{
		UE_LOG(LogTemp, Log, TEXT("LoginWithDevAuth: already logged in"));
		return true;
	}

	if (bDevAuthId && CredentialName.IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("LoginWithDevAuth: DevAuth requires a non-empty CredentialName (DevAuthTool credential)"));
		return false;
	}

	FOnlineAccountCredentials Creds;
	Creds.Type = bDevAuthId ? TEXT("developer") : TEXT("accountportal");
	Creds.Id = bDevAuthId ? DevAuthToolAddressConfig : TEXT("");
	Creds.Token = bDevAuthId ? CredentialName : TEXT("");

	UE_LOG(LogTemp, Log, TEXT("LoginWithDevAuth: attempting %s Type=%s Id=%s Token=%s"),
		bDevAuthId ? TEXT("DevAuth") : TEXT("AccountPortal"),
		*Creds.Type,
		bDevAuthId ? *Creds.Id : TEXT(""),
		bDevAuthId ? *Creds.Token : TEXT(""));

	// Bind completion delegate.
	if (OnLoginCompleteHandle.IsValid())
	{
		IdentityInterface->ClearOnLoginCompleteDelegate_Handle(LocalUserNum, OnLoginCompleteHandle);
		OnLoginCompleteHandle.Reset();
	}

	OnLoginCompleteHandle = IdentityInterface->AddOnLoginCompleteDelegate_Handle(
		LocalUserNum,
		FOnLoginCompleteDelegate::CreateUObject(this, &UEOSSessionGameInstance::OnLoginComplete));

	bLastAttemptWasDevAuth = bDevAuthId;
	bLoginInProgress = true;

	const bool bStarted = IdentityInterface->Login(LocalUserNum, Creds);
	if (!bStarted)
	{
		UE_LOG(LogTemp, Error, TEXT("LoginWithDevAuth: IdentityInterface->Login returned false (login did not start)"));
		bLoginInProgress = false;

		IdentityInterface->ClearOnLoginCompleteDelegate_Handle(LocalUserNum, OnLoginCompleteHandle);
		OnLoginCompleteHandle.Reset();
		return false;
	}

	return true;
}

void UEOSSessionGameInstance::OnLoginComplete(int32 LocalUserNum, bool bWasSuccessful, const FUniqueNetId& UserId, const FString& Error)
{
	bLoginInProgress = false;

	if (IdentityInterface.IsValid() && OnLoginCompleteHandle.IsValid())
	{
		IdentityInterface->ClearOnLoginCompleteDelegate_Handle(LocalUserNum, OnLoginCompleteHandle);
		OnLoginCompleteHandle.Reset();
	}

	if (bWasSuccessful)
	{
		UE_LOG(LogTemp, Log, TEXT("OnLoginComplete: success=1 user=%s"), *UserId.ToString());
		bLoginFallbackAllowed = false;
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("OnLoginComplete: success=0 error=%s (lastAttemptWasDevAuth=%d fallbackAllowed=%d)"), *Error, bLastAttemptWasDevAuth ? 1 : 0, bLoginFallbackAllowed ? 1 : 0);

	// If DevAuth failed and fallback is allowed, attempt AccountPortal once (only for connectivity/service errors).
	if (bLastAttemptWasDevAuth && bLoginFallbackAllowed)
	{
		bLoginFallbackAllowed = false;
		if (ShouldFallbackToAccountPortalFromDevAuthError(Error))
		{
			UE_LOG(LogTemp, Warning, TEXT("OnLoginComplete: DevAuth failed with connectivity/service error; trying AccountPortal fallback. Error=%s"), *Error);
			LoginWithDevAuth(false, PendingCredentialName);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("OnLoginComplete: DevAuth failed; not falling back automatically (likely bad DevAuthTool credential). Error=%s"), *Error);
		}
	}
}

// ----------------------
// Sessions: Find
// ----------------------

void UEOSSessionGameInstance::FindSessions(int32 MaxResults)
{
	if (!SessionInterface.IsValid())
	{
		if (IOnlineSubsystem* Subsystem = GetOSSForWorld(this))
		{
			SessionInterface = Subsystem->GetSessionInterface();
		}
	}

	if (!SessionInterface.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("FindSessions: SessionInterface invalid"));
		return;
	}

	if (OnFindSessionsCompleteHandle.IsValid())
	{
		SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(OnFindSessionsCompleteHandle);
		OnFindSessionsCompleteHandle.Reset();
	}

	SessionSearch = MakeShared<FOnlineSessionSearch>();
	SessionSearch->MaxSearchResults = FMath::Clamp(MaxResults, 1, 5000);
	SessionSearch->bIsLanQuery = false;

	// IMPORTANT: Do not set SEARCH_PRESENCE / SEARCH_LOBBIES (undefined in some UE/EOS versions).
	// Keep the query unfiltered so we can see all advertised sessions.

	OnFindSessionsCompleteHandle = SessionInterface->AddOnFindSessionsCompleteDelegate_Handle(
		FOnFindSessionsCompleteDelegate::CreateUObject(this, &UEOSSessionGameInstance::OnFindSessionsComplete));

	UE_LOG(LogTemp, Log, TEXT("FindSessions: starting MaxResults=%d"), SessionSearch->MaxSearchResults);

	const bool bStarted = SessionInterface->FindSessions(0, SessionSearch.ToSharedRef());
	if (!bStarted)
	{
		UE_LOG(LogTemp, Error, TEXT("FindSessions: FindSessions returned false"));
		SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(OnFindSessionsCompleteHandle);
		OnFindSessionsCompleteHandle.Reset();
		SessionSearch.Reset();
	}
}

void UEOSSessionGameInstance::OnFindSessionsComplete(bool bWasSuccessful)
{
	if (SessionInterface.IsValid() && OnFindSessionsCompleteHandle.IsValid())
	{
		SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(OnFindSessionsCompleteHandle);
		OnFindSessionsCompleteHandle.Reset();
	}

	UE_LOG(LogTemp, Log, TEXT("OnFindSessionsComplete: success=%d results=%d"), bWasSuccessful ? 1 : 0,
		SessionSearch.IsValid() ? SessionSearch->SearchResults.Num() : -1);

	UpdateCachedItemsFromSearch();
	OnSessionsSearchUpdated.Broadcast(CachedItems.Num());
}

void UEOSSessionGameInstance::UpdateCachedItemsFromSearch()
{
	CachedItems.Reset();

	if (!SessionSearch.IsValid())
	{
		return;
	}

	for (int32 i = 0; i < SessionSearch->SearchResults.Num(); ++i)
	{
		const FOnlineSessionSearchResult& R = SessionSearch->SearchResults[i];

		USessionRowData* Row = NewObject<USessionRowData>(this);
		Row->SearchResultIndex = i;
		Row->SessionId = R.GetSessionIdStr();
		Row->SessionName = NAME_EOSGameSession.ToString();
		if (const FOnlineSessionSetting* Setting = R.Session.SessionSettings.Settings.Find(KEY_DISPLAY_SESSION_NAME))
		{
			Row->SessionName = Setting->Data.ToString();
		}
		Row->HostName = R.Session.OwningUserName;
		Row->OpenPublicSlots = R.Session.NumOpenPublicConnections;
		Row->MaxPublicSlots = R.Session.SessionSettings.NumPublicConnections;
		Row->PingMs = R.PingInMs;
		Row->OpenPrivateSlots = R.Session.NumOpenPrivateConnections;

		Row->MaxPrivateSlots = R.Session.SessionSettings.NumPrivateConnections;
		Row->bIsPrivate = !R.Session.SessionSettings.bShouldAdvertise;
		// Map name
		if (const FOnlineSessionSetting* Setting = R.Session.SessionSettings.Settings.Find(KEY_MAPNAME))
		{
			Row->MapName = Setting->Data.ToString();
		}
		// Region / ruleset (custom keys)
		if (const FOnlineSessionSetting* Setting = R.Session.SessionSettings.Settings.Find(KEY_REGION))
		{
			Row->Region = Setting->Data.ToString();
		}
		if (const FOnlineSessionSetting* Setting = R.Session.SessionSettings.Settings.Find(KEY_RULESET))
		{
			Row->RuleSet = Setting->Data.ToString();
		}

		CachedItems.Add(Row);
	}
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

// ----------------------
// Sessions: Host
// ----------------------

bool UEOSSessionGameInstance::HostSession_Legacy(int32 PlayersPerTeam, const FString& MapPath, bool bIsPrivate)
{
	return HostSession(NAME_EOSGameSession.ToString(), MapPath, TEXT(""), TEXT(""), bIsPrivate, PlayersPerTeam);
}

bool UEOSSessionGameInstance::HostSession(const FString& SessionName, const FString& MapName,
                                         const FString& Region, const FString& RuleSet,
                                         bool bIsPrivate, int32 PlayersPerTeam)
{
	if (!SessionInterface.IsValid())
	{
		if (IOnlineSubsystem* Subsystem = GetOSSForWorld(this))
		{
			SessionInterface = Subsystem->GetSessionInterface();
		}
	}

	if (!SessionInterface.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("HostSession: SessionInterface invalid"));
		return false;
	}

	const FName Name = NAME_EOSGameSession;
	const int32 MaxPlayers = FMath::Clamp(PlayersPerTeam, 1, 128);

	FOnlineSessionSettings Settings;
	Settings.bIsLANMatch = false;
	Settings.bUsesPresence = false;
	Settings.bAllowJoinInProgress = true;
	Settings.bAllowJoinViaPresence = false;
	Settings.bAllowInvites = true;
	Settings.bShouldAdvertise = !bIsPrivate;
	Settings.bUseLobbiesIfAvailable = false;
	Settings.NumPublicConnections = bIsPrivate ? 0 : MaxPlayers;
	Settings.NumPrivateConnections = bIsPrivate ? MaxPlayers : 0;

	Settings.Set(KEY_MAPNAME, MapName, EOnlineDataAdvertisementType::ViaOnlineService);
	const FString DisplaySessionName = SessionName.IsEmpty() ? NAME_EOSGameSession.ToString() : SessionName;
	Settings.Set(KEY_DISPLAY_SESSION_NAME, DisplaySessionName, EOnlineDataAdvertisementType::ViaOnlineService);
	if (!Region.IsEmpty())
	{
		Settings.Set(KEY_REGION, Region, EOnlineDataAdvertisementType::ViaOnlineService);
	}
	if (!RuleSet.IsEmpty())
	{
		Settings.Set(KEY_RULESET, RuleSet, EOnlineDataAdvertisementType::ViaOnlineService);
	}

	const FString ListenTravelUrl = FString::Printf(TEXT("%s?listen"), *MapName);
	return StartCreateSessionInternal(Name, Settings, ListenTravelUrl);
}

bool UEOSSessionGameInstance::StartCreateSessionInternal(const FName SessionName, const FOnlineSessionSettings& Settings, const FString& ListenTravelUrl)
{
	if (!SessionInterface.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("StartCreateSessionInternal: SessionInterface invalid"));
		return false;
	}

	// If a session exists already, destroy it first and then create.
	if (SessionInterface->GetNamedSession(SessionName) != nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("HostSession: session '%s' already exists -> DestroySession then CreateSession"), *SessionName.ToString());
		PendingHostSessionName = SessionName;
		PendingHostSettings = MakeUnique<FOnlineSessionSettings>(Settings);
		PendingListenTravelUrl = ListenTravelUrl;
		bCreateAfterDestroy = true;
		StartDestroyThenCreate(SessionName);
		return true;
	}

	// Bind create delegate.
	if (OnCreateSessionCompleteHandle.IsValid())
	{
		SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(OnCreateSessionCompleteHandle);
		OnCreateSessionCompleteHandle.Reset();
	}
	OnCreateSessionCompleteHandle = SessionInterface->AddOnCreateSessionCompleteDelegate_Handle(
		FOnCreateSessionCompleteDelegate::CreateUObject(this, &UEOSSessionGameInstance::OnCreateSessionComplete));

	PendingListenTravelUrl = ListenTravelUrl;

	UE_LOG(LogTemp, Log, TEXT("HostSession: CreateSession name=%s map=%s public=%d private=%d advertise=%d"),
		*SessionName.ToString(), *ListenTravelUrl,
		Settings.NumPublicConnections, Settings.NumPrivateConnections,
		Settings.bShouldAdvertise ? 1 : 0);

	const bool bStarted = SessionInterface->CreateSession(0, SessionName, Settings);
	UE_LOG(LogTemp, Log, TEXT("HostSession: CreateSession started=%d"), bStarted ? 1 : 0);

	if (!bStarted)
	{
		SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(OnCreateSessionCompleteHandle);
		OnCreateSessionCompleteHandle.Reset();
		return false;
	}

	return true;
}

void UEOSSessionGameInstance::StartDestroyThenCreate(const FName SessionName)
{
	if (!SessionInterface.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("StartDestroyThenCreate: SessionInterface invalid"));
		bCreateAfterDestroy = false;
		PendingHostSettings.Reset();
		return;
	}

	if (OnDestroySessionCompleteHandle.IsValid())
	{
		SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(OnDestroySessionCompleteHandle);
		OnDestroySessionCompleteHandle.Reset();
	}
	OnDestroySessionCompleteHandle = SessionInterface->AddOnDestroySessionCompleteDelegate_Handle(
		FOnDestroySessionCompleteDelegate::CreateUObject(this, &UEOSSessionGameInstance::OnDestroySessionComplete));

	const bool bStarted = SessionInterface->DestroySession(SessionName);
	UE_LOG(LogTemp, Log, TEXT("HostSession: DestroySession(%s) started=%d"), *SessionName.ToString(), bStarted ? 1 : 0);

	if (!bStarted)
	{
		SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(OnDestroySessionCompleteHandle);
		OnDestroySessionCompleteHandle.Reset();
		bCreateAfterDestroy = false;
		PendingHostSettings.Reset();
	}
}

void UEOSSessionGameInstance::OnDestroySessionComplete(FName SessionName, bool bWasSuccessful)
{
	if (SessionInterface.IsValid() && OnDestroySessionCompleteHandle.IsValid())
	{
		SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(OnDestroySessionCompleteHandle);
		OnDestroySessionCompleteHandle.Reset();
	}

	UE_LOG(LogTemp, Log, TEXT("OnDestroySessionComplete: name=%s success=%d"), *SessionName.ToString(), bWasSuccessful ? 1 : 0);

	if (bCreateAfterDestroy && PendingHostSettings.IsValid())
	{
		bCreateAfterDestroy = false;
		// Copy settings now, then clear pending to avoid re-entrancy issues.
		const FOnlineSessionSettings SettingsCopy = *PendingHostSettings;
		const FString TravelUrlCopy = PendingListenTravelUrl;
		PendingHostSettings.Reset();
		PendingListenTravelUrl.Reset();

		StartCreateSessionInternal(SessionName, SettingsCopy, TravelUrlCopy);
		return;
	}

	bCreateAfterDestroy = false;
	PendingHostSettings.Reset();
	PendingListenTravelUrl.Reset();
}

void UEOSSessionGameInstance::OnCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
	if (SessionInterface.IsValid() && OnCreateSessionCompleteHandle.IsValid())
	{
		SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(OnCreateSessionCompleteHandle);
		OnCreateSessionCompleteHandle.Reset();
	}

	UE_LOG(LogTemp, Log, TEXT("OnCreateSessionComplete: name=%s success=%d"), *SessionName.ToString(), bWasSuccessful ? 1 : 0);

	if (!bWasSuccessful)
	{
		UE_LOG(LogTemp, Error, TEXT("CreateSession failed. No listen travel will be performed."));
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogTemp, Error, TEXT("OnCreateSessionComplete: GetWorld() null"));
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("Listen travel: %s"), *PendingListenTravelUrl);
	World->ServerTravel(PendingListenTravelUrl);
}

// ----------------------
// Sessions: Join
// ----------------------

void UEOSSessionGameInstance::JoinSessionByIndex(int32 SearchResultIndex)
{
	if (!SessionInterface.IsValid() || !SessionSearch.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("JoinSessionByIndex: invalid interfaces/search"));
		return;
	}

	if (!SessionSearch->SearchResults.IsValidIndex(SearchResultIndex))
	{
		UE_LOG(LogTemp, Warning, TEXT("JoinSessionByIndex: invalid index=%d (num=%d)"), SearchResultIndex, SessionSearch->SearchResults.Num());
		return;
	}

	if (OnJoinSessionCompleteHandle.IsValid())
	{
		SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(OnJoinSessionCompleteHandle);
		OnJoinSessionCompleteHandle.Reset();
	}
	OnJoinSessionCompleteHandle = SessionInterface->AddOnJoinSessionCompleteDelegate_Handle(
		FOnJoinSessionCompleteDelegate::CreateUObject(this, &UEOSSessionGameInstance::OnJoinSessionComplete));

	const FOnlineSessionSearchResult& Result = SessionSearch->SearchResults[SearchResultIndex];
	const bool bStarted = SessionInterface->JoinSession(0, NAME_EOSGameSession, Result);
	UE_LOG(LogTemp, Log, TEXT("JoinSession: started=%d index=%d sessionId=%s"), bStarted ? 1 : 0, SearchResultIndex, *Result.GetSessionIdStr());

	if (!bStarted)
	{
		SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(OnJoinSessionCompleteHandle);
		OnJoinSessionCompleteHandle.Reset();
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
	if (SessionInterface.IsValid() && OnJoinSessionCompleteHandle.IsValid())
	{
		SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(OnJoinSessionCompleteHandle);
		OnJoinSessionCompleteHandle.Reset();
	}

	UE_LOG(LogTemp, Log, TEXT("OnJoinSessionComplete: name=%s result=%d"), *SessionName.ToString(), int32(Result));

	if (Result != EOnJoinSessionCompleteResult::Success)
	{
		UE_LOG(LogTemp, Error, TEXT("JoinSession failed (result=%d)"), int32(Result));
		return;
	}

	FString ConnectString;
	if (!SessionInterface.IsValid() || !SessionInterface->GetResolvedConnectString(SessionName, ConnectString))
	{
		UE_LOG(LogTemp, Error, TEXT("OnJoinSessionComplete: GetResolvedConnectString failed"));
		return;
	}

	APlayerController* PC = GetFirstLocalPlayerController();
	if (!PC)
	{
		UE_LOG(LogTemp, Error, TEXT("OnJoinSessionComplete: no local PlayerController"));
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("ClientTravel to %s"), *ConnectString);
	PC->ClientTravel(ConnectString, TRAVEL_Absolute);
}

// ----------------------
// Misc
// ----------------------

FString UEOSSessionGameInstance::GameModeToString(EMatchGameMode Mode) const
{
	switch (Mode)
	{
	case EMatchGameMode::GM_FreeForAll:
		return TEXT("FreeForAll");
	case EMatchGameMode::GM_TeamDeathMatch:
		return TEXT("TeamDeathMatch");
	case EMatchGameMode::Default:
	default:
		return TEXT("Default");
	}
}
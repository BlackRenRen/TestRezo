#include "EOSSessionGameInstance.h"
// v31_linkfix: avoid Set<int64> to prevent LNK2019

#include "Interfaces/OnlineSessionInterface.h"

#include "OnlineSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/ConfigCacheIni.h"

#include "Sockets.h"
#include "SocketSubsystem.h"
#include "IPAddress.h"

#include "SessionRowData.h"

const FName UEOSSessionGameInstance::GAME_SESSION_NAME(TEXT("GameSession"));
const FName UEOSSessionGameInstance::KEY_APP_TAG(TEXT("APP_TAG"));
const FName UEOSSessionGameInstance::KEY_MAPNAME(TEXT("MAPNAME"));
const FName UEOSSessionGameInstance::KEY_DISPLAY_NAME(TEXT("DISPLAY_NAME"));
const FName UEOSSessionGameInstance::KEY_HOST_TS(TEXT("HOST_TS"));

static bool IsDevAuthToolReachable(const FString& Addr, double TimeoutSeconds = 0.25)
{
	FString HostStr;
	FString PortStr;
	if (!Addr.Split(TEXT(":"), &HostStr, &PortStr))
	{
		return false;
	}

	const int32 Port = FCString::Atoi(*PortStr);
	if (Port <= 0)
	{
		return false;
	}

	// Normalize localhost to an IPv4 literal (FInternetAddr::SetIp expects an address, not a hostname)
	if (HostStr.Equals(TEXT("localhost"), ESearchCase::IgnoreCase))
	{
		HostStr = TEXT("127.0.0.1");
	}

	ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
	if (!SocketSubsystem)
	{
		return false;
	}

	TSharedRef<FInternetAddr> InternetAddr = SocketSubsystem->CreateInternetAddr();
	bool bIsValidIp = false;
	InternetAddr->SetIp(*HostStr, bIsValidIp);
	if (!bIsValidIp)
	{
		return false;
	}
	InternetAddr->SetPort(Port);

	FSocket* Socket = SocketSubsystem->CreateSocket(NAME_Stream, TEXT("DevAuthProbe"), false);
	if (!Socket)
	{
		return false;
	}

	Socket->SetNonBlocking(true);
	bool bConnected = Socket->Connect(*InternetAddr);
	if (!bConnected)
	{
		const ESocketErrors Err = SocketSubsystem->GetLastErrorCode();
		if (Err == SE_EWOULDBLOCK || Err == SE_EINPROGRESS || Err == SE_EALREADY)
		{
			bConnected = Socket->Wait(ESocketWaitConditions::WaitForWrite, FTimespan::FromSeconds(TimeoutSeconds))
				&& (Socket->GetConnectionState() == SCS_Connected);
		}
	}

	Socket->Close();
	SocketSubsystem->DestroySocket(Socket);
	return bConnected;
}

void UEOSSessionGameInstance::Init()
{
	Super::Init();

	IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get();
	if (!Subsystem)
	{
		UE_LOG(LogTemp, Error, TEXT("EOSSessionGameInstance::Init - No OnlineSubsystem"));
		return;
	}

	SessionInterface = Subsystem->GetSessionInterface();
	IdentityInterface = Subsystem->GetIdentityInterface();

	UE_LOG(LogTemp, Warning, TEXT("EOSSessionGameInstance::Init - Subsystem=%s Session=%s Identity=%s"),
		*Subsystem->GetSubsystemName().ToString(),
		SessionInterface.IsValid() ? TEXT("OK") : TEXT("NULL"),
		IdentityInterface.IsValid() ? TEXT("OK") : TEXT("NULL"));
}

void UEOSSessionGameInstance::Shutdown()
{
	if (IdentityInterface.IsValid() && LoginCompleteHandle.IsValid())
	{
		IdentityInterface->ClearOnLoginCompleteDelegate_Handle(LOCAL_USER_NUM, LoginCompleteHandle);
		LoginCompleteHandle.Reset();
	}

	if (SessionInterface.IsValid())
	{
		if (FindSessionsCompleteHandle.IsValid())
		{
			SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteHandle);
			FindSessionsCompleteHandle.Reset();
		}
		if (CreateSessionCompleteHandle.IsValid())
		{
			SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteHandle);
			CreateSessionCompleteHandle.Reset();
		}
		if (DestroySessionCompleteHandle.IsValid())
		{
			SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteHandle);
			DestroySessionCompleteHandle.Reset();
		}
		if (JoinSessionCompleteHandle.IsValid())
		{
			SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteHandle);
			JoinSessionCompleteHandle.Reset();
		}
	}

	Super::Shutdown();
}

FString UEOSSessionGameInstance::GetDevAuthAddressFromConfig() const
{
	// Try several known keys/sections because UE/EOS plugin versions differ.
	FString Addr;
	FString Host;
	int32 Port = 0;

	auto PullFromSection = [&](const TCHAR* Section)
	{
		if (!GConfig) { return; }

		FString Tmp;
		if (Addr.IsEmpty())
		{
			if (GConfig->GetString(Section, TEXT("DevAuthToolAddress"), Tmp, GEngineIni) && !Tmp.IsEmpty())
			{
				Addr = Tmp;
			}
			else if (GConfig->GetString(Section, TEXT("DevAuthToolEndpoint"), Tmp, GEngineIni) && !Tmp.IsEmpty())
			{
				Addr = Tmp;
			}
		}

		if (Host.IsEmpty())
		{
			if (GConfig->GetString(Section, TEXT("DevAuthToolHost"), Tmp, GEngineIni) && !Tmp.IsEmpty())
			{
				Host = Tmp;
			}
			else if (GConfig->GetString(Section, TEXT("DevAuthToolIP"), Tmp, GEngineIni) && !Tmp.IsEmpty())
			{
				Host = Tmp;
			}
		}

		if (Port <= 0)
		{
			int32 TmpPort = 0;
			if (GConfig->GetInt(Section, TEXT("DevAuthToolPort"), TmpPort, GEngineIni) && TmpPort > 0)
			{
				Port = TmpPort;
			}
		}
	};

	PullFromSection(TEXT("OnlineSubsystemEOS"));
	PullFromSection(TEXT("/Script/OnlineSubsystemEOS.EOSSettings"));

	if (Addr.IsEmpty() && !Host.IsEmpty())
	{
		if (Port <= 0) { Port = 8081; }
		Addr = FString::Printf(TEXT("%s:%d"), *Host, Port);
	}

	if (Addr.IsEmpty())
	{
		Addr = TEXT("127.0.0.1:8081");
	}

	// Normalize (strip protocol/path, ensure host:port)
	Addr.TrimStartAndEndInline();
	Addr.ReplaceInline(TEXT("http://"), TEXT(""), ESearchCase::IgnoreCase);
	Addr.ReplaceInline(TEXT("https://"), TEXT(""), ESearchCase::IgnoreCase);

	int32 SlashIdx = INDEX_NONE;
	if (Addr.FindChar(TEXT('/'), SlashIdx))
	{
		Addr = Addr.Left(SlashIdx);
	}

	FString HostPart;
	FString PortPart;
	if (!Addr.Split(TEXT(":"), &HostPart, &PortPart))
	{
		HostPart = Addr;
		PortPart = TEXT("8081");
	}

	HostPart.TrimStartAndEndInline();
	PortPart.TrimStartAndEndInline();

	if (HostPart.IsEmpty())
	{
		HostPart = TEXT("127.0.0.1");
	}
	if (HostPart.Equals(TEXT("localhost"), ESearchCase::IgnoreCase))
	{
		HostPart = TEXT("127.0.0.1");
	}

	const int32 ParsedPort = FCString::Atoi(*PortPart);
	const int32 FinalPort = (ParsedPort > 0) ? ParsedPort : 8081;

	return FString::Printf(TEXT("%s:%d"), *HostPart, FinalPort);
}

bool UEOSSessionGameInstance::LoginPreferDevAuth(const FString& CredentialName)
{
	if (bLoginInProgress)
	{
		UE_LOG(LogTemp, Warning, TEXT("LoginPreferDevAuth: login already in progress"));
		return false;
	}

	IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get(TEXT("EOS"));
	if (!Subsystem)
	{
		UE_LOG(LogTemp, Error, TEXT("LoginPreferDevAuth: EOS subsystem not found"));
		return false;
	}

	// NOTE: avoid shadowing the class member 'IdentityInterface' (C4458 may be treated as error).
	IOnlineIdentityPtr LocalIdentityInterface = Subsystem->GetIdentityInterface();
	if (!LocalIdentityInterface.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("LoginPreferDevAuth: Identity interface invalid"));
		return false;
	}

	if (LocalIdentityInterface->GetLoginStatus(0) == ELoginStatus::LoggedIn)
	{
		UE_LOG(LogTemp, Warning, TEXT("LoginPreferDevAuth: already logged in"));
		OnLoginStateChanged.Broadcast(true, TEXT(""));
		return true;
	}

	const FString DevAuthAddr = GetDevAuthAddressFromConfig();
	const bool bReachable = !DevAuthAddr.IsEmpty() && IsDevAuthToolReachable(DevAuthAddr);

	UE_LOG(LogTemp, Warning, TEXT("LoginPreferDevAuth: DevAuthAddr=%s reachable=%d (will try DevAuth first)"),
		*DevAuthAddr, bReachable ? 1 : 0);

	return LoginWithDevAuth(true, CredentialName);
}


bool UEOSSessionGameInstance::LoginWithDevAuth(bool bDevAuthId, const FString& CredentialName)
{
	if (!IdentityInterface.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("Login: IdentityInterface invalid"));
		return false;
	}

	if (bLoginInProgress)
	{
		UE_LOG(LogTemp, Warning, TEXT("Login: login already in progress"));
		return false;
	}

	const ELoginStatus::Type Status = IdentityInterface->GetLoginStatus(LOCAL_USER_NUM);
	if (Status == ELoginStatus::LoggedIn)
	{
		UE_LOG(LogTemp, Warning, TEXT("Login: already logged in"));
		OnLoginStateChanged.Broadcast(true, TEXT(""));
		return true;
	}

	if (!LoginCompleteHandle.IsValid())
	{
		LoginCompleteHandle = IdentityInterface->AddOnLoginCompleteDelegate_Handle(
			LOCAL_USER_NUM,
			FOnLoginCompleteDelegate::CreateUObject(this, &UEOSSessionGameInstance::HandleLoginComplete)
		);
	}

	
FOnlineAccountCredentials Creds;
bLastAttemptWasDevAuth = bDevAuthId;

FString DevAuthAddr;
bool bDevAuthReachable = false;
if (bDevAuthId)
{
	DevAuthAddr = GetDevAuthAddressFromConfig();
	bDevAuthReachable = !DevAuthAddr.IsEmpty() && IsDevAuthToolReachable(DevAuthAddr);
	// Fallback to AccountPortal only if DevAuthTool is NOT reachable.
	bFallbackAllowed = !bDevAuthReachable;

	Creds.Type = TEXT("developer");
	Creds.Id = DevAuthAddr;
	Creds.Token = CredentialName;

	UE_LOG(LogTemp, Warning, TEXT("Login: attempting DevAuth Type=%s Id=%s Token=%s (reachable=%d fallbackAllowed=%d)"),
		*Creds.Type, *Creds.Id, *Creds.Token, bDevAuthReachable ? 1 : 0, bFallbackAllowed ? 1 : 0);
}
else
{
	bFallbackAllowed = false;
		Creds.Type = TEXT("accountportal");
		Creds.Id = TEXT("");
		Creds.Token = TEXT("");

		UE_LOG(LogTemp, Warning, TEXT("Login: attempting AccountPortal Type=accountportal"));
	}

	const bool bStarted = IdentityInterface->Login(LOCAL_USER_NUM, Creds);
	bLoginInProgress = bStarted;

	// If DevAuth couldn't even start, allow immediate fallback to accountportal.
	if (!bStarted && bDevAuthId && bFallbackAllowed)
	{
		bLoginInProgress = false;
		bFallbackAllowed = false;
		UE_LOG(LogTemp, Warning, TEXT("Login: DevAuth could not start -> fallback AccountPortal"));
		return LoginWithDevAuth(false, TEXT(""));
	}

	
// If nothing started and we are not falling back, let UI know.
if (!bStarted && !(bDevAuthId && bFallbackAllowed))
{
	OnLoginStateChanged.Broadcast(false, TEXT("Login request could not be started"));
}

	return bStarted;
}

void UEOSSessionGameInstance::HandleLoginComplete(int32 LocalUserNum, bool bWasSuccessful, const FUniqueNetId& UserId, const FString& Error)
{
	bLoginInProgress = false;

	UE_LOG(LogTemp, Warning, TEXT("OnLoginComplete: user=%d success=%d id=%s error=%s"),
		LocalUserNum, bWasSuccessful ? 1 : 0, *UserId.ToString(), *Error);

	if (bWasSuccessful)
	{
		bFallbackAllowed = false;
		OnLoginStateChanged.Broadcast(true, TEXT(""));
		return;
	}

	// DevAuth failed: fallback to accountportal once.
	if (bFallbackAllowed && bLastAttemptWasDevAuth)
	{
		bFallbackAllowed = false;
		UE_LOG(LogTemp, Warning, TEXT("Login: DevAuth failed -> fallback AccountPortal (once)"));
		LoginWithDevAuth(false, TEXT(""));
		return;
	}

	OnLoginStateChanged.Broadcast(false, Error);
}

void UEOSSessionGameInstance::FindSessions(int32 MaxResults)
{
	if (!SessionInterface.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("FindSessions: SessionInterface invalid"));
		return;
	}

	if (!FindSessionsCompleteHandle.IsValid())
	{
		FindSessionsCompleteHandle = SessionInterface->AddOnFindSessionsCompleteDelegate_Handle(
			FOnFindSessionsCompleteDelegate::CreateUObject(this, &UEOSSessionGameInstance::HandleFindSessionsComplete)
		);
	}

	SessionSearch = MakeShared<FOnlineSessionSearch>();
	SessionSearch->MaxSearchResults = MaxResults;
	SessionSearch->bIsLanQuery = false;

	// Restrict to our app tag to avoid stale sessions from other tests.
	SessionSearch->QuerySettings.Set(KEY_APP_TAG, FString(TEXT("EOS_OSS_TUTORIAL")), EOnlineComparisonOp::Equals);

	UE_LOG(LogTemp, Warning, TEXT("FindSessions: starting MaxResults=%d"), MaxResults);
	const bool bStarted = SessionInterface->FindSessions(LOCAL_USER_NUM, SessionSearch.ToSharedRef());
	UE_LOG(LogTemp, Warning, TEXT("FindSessions: started=%d"), bStarted ? 1 : 0);
}

void UEOSSessionGameInstance::HandleFindSessionsComplete(bool bWasSuccessful)
{
	BuildCachedItemsFromSearchResults(bWasSuccessful);
	OnSessionsSearchUpdated.Broadcast(CachedItems.Num());
}

FString UEOSSessionGameInstance::GetSettingString(const FOnlineSessionSettings& Settings, const FName& Key, const FString& DefaultValue)
{
	const FOnlineSessionSetting* S = Settings.Settings.Find(Key);
	return S ? S->Data.ToString() : DefaultValue;
}

int64 UEOSSessionGameInstance::GetSettingInt64(const FOnlineSessionSettings& Settings, const FName& Key, int64 DefaultValue)
{
	const FOnlineSessionSetting* S = Settings.Settings.Find(Key);
	if (!S) return DefaultValue;

	// Robust across UE/OSS versions: try numeric read first, then parse ToString().
	int64 V = DefaultValue;
	S->Data.GetValue(V);

	// For our use (HOST_TS), DefaultValue is 0 and real timestamps are non-zero.
	if (V != DefaultValue)
	{
		return V;
	}

	const FString AsString = S->Data.ToString();
	if (AsString.IsEmpty())
	{
		return DefaultValue;
	}

	return FCString::Atoi64(*AsString);
}

void UEOSSessionGameInstance::BuildCachedItemsFromSearchResults(bool bWasSuccessful)
{
	CachedItems.Reset();

	if (!bWasSuccessful || !SessionSearch.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("BuildCachedItemsFromSearchResults: no results (success=%d searchValid=%d)"),
			bWasSuccessful ? 1 : 0, SessionSearch.IsValid() ? 1 : 0);
		return;
	}

	const auto& Results = SessionSearch->SearchResults;
	UE_LOG(LogTemp, Warning, TEXT("OnFindSessionsComplete: success=1 results=%d"), Results.Num());

	// Best per (Host|Map) using HOST_TS to avoid showing stale duplicates.
	TMap<FString, TObjectPtr<USessionRowData>> BestByKey;

	for (int32 i = 0; i < Results.Num(); ++i)
	{
		const FOnlineSessionSearchResult& R = Results[i];

		const FString SessionIdStr = R.GetSessionIdStr();
		const FString Host = R.Session.OwningUserName;
		const FString Map = GetSettingString(R.Session.SessionSettings, KEY_MAPNAME, TEXT(""));
		const int64 TS = GetSettingInt64(R.Session.SessionSettings, KEY_HOST_TS, 0);

		const FString Key = Host + TEXT("|") + Map;

		USessionRowData* Row = NewObject<USessionRowData>(this);
		Row->SearchResultIndex = i;
		Row->SessionId = SessionIdStr;
		Row->HostName = Host;
		Row->MapName = Map;
		Row->SessionName = GetSettingString(R.Session.SessionSettings, KEY_DISPLAY_NAME, TEXT(""));

		Row->PingMs = R.PingInMs;

		Row->MaxPlayers = R.Session.SessionSettings.NumPublicConnections;
		Row->OpenPublicSlots = R.Session.NumOpenPublicConnections;
		Row->CurrentPlayers = FMath::Max(0, Row->MaxPlayers - Row->OpenPublicSlots);
		Row->bIsJoinable = (Row->SearchResultIndex != INDEX_NONE);

		TObjectPtr<USessionRowData>* Existing = BestByKey.Find(Key);
		if (!Existing)
		{
			BestByKey.Add(Key, Row);
		}
		else
		{
			// Keep the most recent by HOST_TS; if equal, keep lower ping.
			const int64 OldTS = GetSettingInt64(Results[(*Existing)->SearchResultIndex].Session.SessionSettings, KEY_HOST_TS, 0);
			if (TS > OldTS || (TS == OldTS && Row->PingMs < (*Existing)->PingMs))
			{
				BestByKey[Key] = Row;
			}
		}

		UE_LOG(LogTemp, Warning, TEXT("FindResult[%d] id=%s host=%s players=%d/%d open=%d ping=%d map=%s ts=%lld"),
			i, *Row->SessionId, *Row->HostName, Row->CurrentPlayers, Row->MaxPlayers, Row->OpenPublicSlots, Row->PingMs, *Row->MapName, TS);
	}

	for (const auto& Pair : BestByKey)
	{
		CachedItems.Add(Pair.Value);
	}
}

bool UEOSSessionGameInstance::HostSession(int32 PlayersPerTeam, const FString& MapPath, bool bIsPrivate)
{
	if (!SessionInterface.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("HostSession: SessionInterface invalid"));
		return false;
	}

	PendingHostMap = MapPath;

	FOnlineSessionSettings S;
	S.bIsLANMatch = false;
	S.bShouldAdvertise = !bIsPrivate;
	S.bAllowJoinInProgress = true;
	S.bUsesPresence = false;
	S.bAllowJoinViaPresence = false;
	S.bUseLobbiesIfAvailable = false;

	S.NumPublicConnections = PlayersPerTeam;
	S.NumPrivateConnections = 0;
	S.bIsDedicated = false;

	S.Set(KEY_APP_TAG, FString(TEXT("EOS_OSS_TUTORIAL")), EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	S.Set(KEY_MAPNAME, MapPath, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	S.Set(KEY_DISPLAY_NAME, bIsPrivate ? FString(TEXT("Private")) : FString(TEXT("Public")), EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	S.Set(KEY_HOST_TS, FString::Printf(TEXT("%lld"), (long long)FDateTime::UtcNow().ToUnixTimestamp()), EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);

	UE_LOG(LogTemp, Warning, TEXT("HostSession: CreateSession map=%s public=%d private=%d"), *MapPath, S.NumPublicConnections, bIsPrivate ? 1 : 0);

	return DestroySessionIfExistsThenCreate(S);
}

bool UEOSSessionGameInstance::DestroySessionIfExistsThenCreate(const FOnlineSessionSettings& Settings)
{
	if (!SessionInterface.IsValid())
	{
		return false;
	}

	if (SessionInterface->GetNamedSession(GAME_SESSION_NAME) == nullptr)
	{
		return StartCreateSessionInternal(Settings);
	}

	if (!DestroySessionCompleteHandle.IsValid())
	{
		DestroySessionCompleteHandle = SessionInterface->AddOnDestroySessionCompleteDelegate_Handle(
			FOnDestroySessionCompleteDelegate::CreateUObject(this, &UEOSSessionGameInstance::HandleDestroySessionComplete)
		);
	}

	bRehostPending = true;
	PendingHostSettings = Settings;

	UE_LOG(LogTemp, Warning, TEXT("HostSession: GameSession exists -> DestroySession then recreate"));
	const bool bDestroyStarted = SessionInterface->DestroySession(GAME_SESSION_NAME);
	UE_LOG(LogTemp, Warning, TEXT("HostSession: DestroySession started=%d"), bDestroyStarted ? 1 : 0);
	return bDestroyStarted;
}

bool UEOSSessionGameInstance::StartCreateSessionInternal(const FOnlineSessionSettings& Settings)
{
	if (!CreateSessionCompleteHandle.IsValid())
	{
		CreateSessionCompleteHandle = SessionInterface->AddOnCreateSessionCompleteDelegate_Handle(
			FOnCreateSessionCompleteDelegate::CreateUObject(this, &UEOSSessionGameInstance::HandleCreateSessionComplete)
		);
	}

	const bool bStarted = SessionInterface->CreateSession(LOCAL_USER_NUM, GAME_SESSION_NAME, Settings);
	UE_LOG(LogTemp, Warning, TEXT("HostSession: CreateSession started=%d"), bStarted ? 1 : 0);
	return bStarted;
}

void UEOSSessionGameInstance::HandleDestroySessionComplete(FName SessionName, bool bWasSuccessful)
{
	UE_LOG(LogTemp, Warning, TEXT("OnDestroySessionComplete: name=%s success=%d"), *SessionName.ToString(), bWasSuccessful ? 1 : 0);

	if (bRehostPending)
	{
		bRehostPending = false;
		UE_LOG(LogTemp, Warning, TEXT("HostSession: recreating after destroy"));
		StartCreateSessionInternal(PendingHostSettings);
	}
}

void UEOSSessionGameInstance::HandleCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
	UE_LOG(LogTemp, Warning, TEXT("OnCreateSessionComplete: name=%s success=%d"), *SessionName.ToString(), bWasSuccessful ? 1 : 0);

	if (!bWasSuccessful)
	{
		UE_LOG(LogTemp, Error, TEXT("CreateSession failed. No listen travel will be performed."));
		return;
	}

	if (UWorld* World = GetWorld())
	{
		UE_LOG(LogTemp, Warning, TEXT("HostSession: Opening level as listen: %s ?listen"), *PendingHostMap);
		UGameplayStatics::OpenLevel(World, FName(*PendingHostMap), true, TEXT("listen"));
	}
}

void UEOSSessionGameInstance::JoinSessionByItem(USessionRowData* Item)
{
	if (!Item)
	{
		UE_LOG(LogTemp, Warning, TEXT("JoinSessionByItem: Item null"));
		return;
	}
	JoinSessionByIndex(Item->SearchResultIndex);
}

void UEOSSessionGameInstance::JoinSessionByIndex(int32 SearchResultIndex)
{
	if (!SessionInterface.IsValid() || !SessionSearch.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("JoinSessionByIndex: invalid session interface/search"));
		return;
	}

	if (!SessionSearch->SearchResults.IsValidIndex(SearchResultIndex))
	{
		UE_LOG(LogTemp, Error, TEXT("JoinSessionByIndex: invalid index %d"), SearchResultIndex);
		return;
	}

	if (SessionInterface->GetNamedSession(GAME_SESSION_NAME) != nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("JoinSessionByIndex: local GameSession exists -> DestroySession first"));
		SessionInterface->DestroySession(GAME_SESSION_NAME);
	}

	if (!JoinSessionCompleteHandle.IsValid())
	{
		JoinSessionCompleteHandle = SessionInterface->AddOnJoinSessionCompleteDelegate_Handle(
			FOnJoinSessionCompleteDelegate::CreateUObject(this, &UEOSSessionGameInstance::HandleJoinSessionComplete)
		);
	}

	const bool bStarted = SessionInterface->JoinSession(LOCAL_USER_NUM, GAME_SESSION_NAME, SessionSearch->SearchResults[SearchResultIndex]);
	UE_LOG(LogTemp, Warning, TEXT("JoinSessionByIndex: JoinSession started=%d index=%d"), bStarted ? 1 : 0, SearchResultIndex);
}

void UEOSSessionGameInstance::HandleJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	UE_LOG(LogTemp, Warning, TEXT("OnJoinSessionComplete: name=%s result=%d"), *SessionName.ToString(), (int32)Result);

	FString ConnectString;
	if (SessionInterface.IsValid() && SessionInterface->GetResolvedConnectString(SessionName, ConnectString))
	{
		UE_LOG(LogTemp, Warning, TEXT("GetResolvedConnectString: %s"), *ConnectString);

		if (UWorld* World = GetWorld())
		{
			if (APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0))
			{
				PC->ClientTravel(ConnectString, ETravelType::TRAVEL_Absolute);
			}
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("GetResolvedConnectString failed"));
	}
}

TArray<USessionRowData*> UEOSSessionGameInstance::GetSessionListItems() const
{
	TArray<USessionRowData*> Result;
	Result.Reserve(CachedItems.Num());
	for (const TObjectPtr<USessionRowData>& Item : CachedItems)
	{
		Result.Add(Item.Get());
	}
	return Result;
}

FString UEOSSessionGameInstance::GameModeToString(EMatchGameMode Mode) const
{
	switch (Mode)
	{
	case EMatchGameMode::GM_FreeForAll:
		return TEXT("FreeForAll");
	case EMatchGameMode::GM_TeamDeathMatch:
		return TEXT("TeamDeathMatch");
	case EMatchGameMode::Default:
		return TEXT("Default");
	default:
		return TEXT("Unknown");
	}
}
#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"

// Delegate macros (DECLARE_DYNAMIC_*).
#include "Delegates/DelegateCombinations.h"

// Online types & constants (FOnlineSessionSettings, SETTING_MAPNAME, EOnJoinSessionCompleteResult, ...).
#include "OnlineSessionSettings.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Interfaces/OnlineIdentityInterface.h"

#include "SessionRowData.h"

#include "EOSSessionGameInstance.generated.h"

UENUM(BlueprintType)
enum class EMatchGameMode : uint8
{
	Default UMETA(DisplayName = "Default"),
	GM_FreeForAll UMETA(DisplayName = "Free For All"),
	GM_TeamDeathMatch UMETA(DisplayName = "Team DeathMatch"),
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSessionsSearchUpdated, int32, EffectiveCount);

UCLASS()
class EOS_OSS_TUTORIAL_API UEOSSessionGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	UEOSSessionGameInstance();

	virtual void Init() override;
	virtual void Shutdown() override;

	UPROPERTY(BlueprintAssignable, Category = "EOS|Sessions")
	FOnSessionsSearchUpdated OnSessionsSearchUpdated;

	UFUNCTION(BlueprintCallable, Category = "EOS|Sessions")
	void FindSessions(int32 MaxResults);

	UFUNCTION(BlueprintCallable, Category = "EOS|Sessions")
	bool HostSession(const FString& SessionName, const FString& MapName,
	                 const FString& Region, const FString& RuleSet,
	                 bool bIsPrivate, int32 PlayersPerTeam);

	// Legacy UI call: HostSession_Legacy(PlayersPerTeam, MapPath, bIsPrivate)
	UFUNCTION(BlueprintCallable, Category = "EOS|Sessions")
	bool HostSession_Legacy(int32 PlayersPerTeam, const FString& MapPath, bool bIsPrivate);

	UFUNCTION(BlueprintCallable, Category = "EOS|Sessions")
	void JoinSessionByIndex(int32 SearchResultIndex);

	UFUNCTION(BlueprintCallable, Category = "EOS|Sessions")
	void JoinSessionByItem(USessionRowData* Item);

	UFUNCTION(BlueprintCallable, Category = "EOS|Sessions")
	TArray<USessionRowData*> GetSessionListItems() const;

	UFUNCTION(BlueprintPure, Category = "EOS|Misc")
	FString GameModeToString(EMatchGameMode Mode) const;

	// Preferred flow: DevAuth if it succeeds, otherwise fallback to AccountPortal.
	UFUNCTION(BlueprintCallable, Category = "EOS|Login")
	bool LoginPreferDevAuth(const FString& CredentialName);

	// Direct login attempt. bDevAuthId=true => Type="developer" Id="127.0.0.1:8081" Token=CredentialName
	// bDevAuthId=false => Type="accountportal" (interactive)
	UFUNCTION(BlueprintCallable, Category = "EOS|Login")
	bool LoginWithDevAuth(bool bDevAuthId, const FString& CredentialName);

	// Legacy UI call: LoginWithDevAuth_Legacy("Renaud")
	UFUNCTION(BlueprintCallable, Category = "EOS|Login")
	bool LoginWithDevAuth_Legacy(const FString& CredentialName);

protected:
	void UpdateCachedItemsFromSearch();

	// Kept non-private so generated code can always compute offsets without access issues.
	UPROPERTY(Transient)
	TArray<TObjectPtr<USessionRowData>> CachedItems;

	bool StartCreateSessionInternal(const FName SessionName, const FOnlineSessionSettings& Settings, const FString& ListenTravelUrl);
	void StartDestroyThenCreate(const FName SessionName);
	void ClearAllSessionDelegates();

private:
	IOnlineSessionPtr SessionInterface;
	IOnlineIdentityPtr IdentityInterface;


	// Cached from [OnlineSubsystemEOS] in DefaultEngine.ini
	bool bUseDevAuthConfig = false;
	FString DevAuthToolAddressConfig = TEXT("127.0.0.1:8081");
	TSharedPtr<FOnlineSessionSearch> SessionSearch;
	FDelegateHandle OnFindSessionsCompleteHandle;
	FDelegateHandle OnCreateSessionCompleteHandle;
	FDelegateHandle OnDestroySessionCompleteHandle;
	FDelegateHandle OnJoinSessionCompleteHandle;

	// ---- Login flow state ----
	bool bLoginInProgress = false;
	bool bLoginFallbackAllowed = false;
	bool bLastAttemptWasDevAuth = false;
	FString PendingCredentialName;
	FDelegateHandle OnLoginCompleteHandle;

	// ---- Host retry (Destroy -> Create) ----
	bool bCreateAfterDestroy = false;
	FName PendingHostSessionName;
	TUniquePtr<FOnlineSessionSettings> PendingHostSettings;
	FString PendingListenTravelUrl;

	// ---- Callbacks ----
	void OnLoginComplete(int32 LocalUserNum, bool bWasSuccessful, const FUniqueNetId& UserId, const FString& Error);
	void OnFindSessionsComplete(bool bWasSuccessful);
	void OnCreateSessionComplete(FName SessionName, bool bWasSuccessful);
	void OnDestroySessionComplete(FName SessionName, bool bWasSuccessful);
	void OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
};

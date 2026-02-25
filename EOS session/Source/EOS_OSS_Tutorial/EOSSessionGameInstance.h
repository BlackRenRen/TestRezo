#pragma once
// FIX_V28_MARKER

#include "CoreMinimal.h"
#include "CoreUObject.h"
#include "UObject/NoExportTypes.h"
#include "UObject/ObjectMacros.h"
#include "UObject/ScriptMacros.h"

#include "Engine/GameInstance.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "OnlineSessionSettings.h"

#include "EOSSessionGameInstance.generated.h"

class USessionRowData;

UENUM(BlueprintType)
enum class EMatchGameMode : uint8
{
	Unknown UMETA(DisplayName="Unknown"),
	Default UMETA(DisplayName="Default"),
	GM_FreeForAll UMETA(DisplayName="FreeForAll"),
	GM_TeamDeathMatch UMETA(DisplayName="TeamDeathMatch"),
};

// Blueprint delegates (Event Dispatchers)
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSessionsSearchUpdated, int32, EffectiveCount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLoginStateChanged, bool, bSuccess, const FString&, Error);

UCLASS()
class EOS_OSS_TUTORIAL_API UEOSSessionGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	// Blueprint Event Dispatchers
	UPROPERTY(BlueprintAssignable, Category="EOS|Sessions")
	FOnSessionsSearchUpdated OnSessionsSearchUpdated;

	UPROPERTY(BlueprintAssignable, Category="EOS|Auth")
	FOnLoginStateChanged OnLoginStateChanged;

public:
	virtual void Init() override;
	virtual void Shutdown() override;

	// Login: prefer DevAuthTool if possible; fallback accountportal if DevAuth fails to start or fails.
	UFUNCTION(BlueprintCallable, Category="EOS|Auth")
	bool LoginPreferDevAuth(const FString& CredentialName);

	// Advanced explicit login.
	UFUNCTION(BlueprintCallable, Category="EOS|Auth")
	bool LoginWithDevAuth(bool bDevAuthId, const FString& CredentialName);

	// Sessions
	UFUNCTION(BlueprintCallable, Category="EOS|Sessions")
	void FindSessions(int32 MaxResults = 2000);

	UFUNCTION(BlueprintCallable, Category="EOS|Sessions")
	bool HostSession(int32 PlayersPerTeam, const FString& MapPath, bool bIsPrivate);

	UFUNCTION(BlueprintCallable, Category="EOS|Sessions")
	void JoinSessionByIndex(int32 SearchResultIndex);

	UFUNCTION(BlueprintCallable, Category="EOS|Sessions")
	void JoinSessionByItem(USessionRowData* Item);

	UFUNCTION(BlueprintCallable, Category="EOS|Sessions")
	TArray<USessionRowData*> GetSessionListItems() const;

	UFUNCTION(BlueprintPure, Category="EOS|Misc")
	FString GameModeToString(EMatchGameMode Mode) const;

public:
	UPROPERTY()
	TArray<TObjectPtr<USessionRowData>> CachedItems;

private:
	IOnlineSessionPtr SessionInterface;
	IOnlineIdentityPtr IdentityInterface;
	TSharedPtr<FOnlineSessionSearch> SessionSearch;

	FDelegateHandle FindSessionsCompleteHandle;
	FDelegateHandle CreateSessionCompleteHandle;
	FDelegateHandle DestroySessionCompleteHandle;
	FDelegateHandle JoinSessionCompleteHandle;
	FDelegateHandle LoginCompleteHandle;

	bool bRehostPending = false;
	FOnlineSessionSettings PendingHostSettings;
	FString PendingHostMap;

	bool bLoginInProgress = false;
	bool bLastAttemptWasDevAuth = false;
	bool bFallbackAllowed = false;

	static constexpr int32 LOCAL_USER_NUM = 0;

	static const FName GAME_SESSION_NAME;
	static const FName KEY_APP_TAG;
	static const FName KEY_MAPNAME;
	static const FName KEY_DISPLAY_NAME;
	static const FName KEY_HOST_TS;

	FString GetDevAuthAddressFromConfig() const;

	void HandleLoginComplete(int32 LocalUserNum, bool bWasSuccessful, const FUniqueNetId& UserId, const FString& Error);
	void HandleFindSessionsComplete(bool bWasSuccessful);
	void HandleDestroySessionComplete(FName SessionName, bool bWasSuccessful);
	void HandleCreateSessionComplete(FName SessionName, bool bWasSuccessful);
	void HandleJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);

	void BuildCachedItemsFromSearchResults(bool bWasSuccessful);

	bool DestroySessionIfExistsThenCreate(const FOnlineSessionSettings& Settings);
	bool StartCreateSessionInternal(const FOnlineSessionSettings& Settings);

	static FString GetSettingString(const FOnlineSessionSettings& Settings, const FName& Key, const FString& DefaultValue);
	static int64 GetSettingInt64(const FOnlineSessionSettings& Settings, const FName& Key, int64 DefaultValue);
};

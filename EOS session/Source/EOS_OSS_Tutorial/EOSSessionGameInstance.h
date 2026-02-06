#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "UObject/ObjectMacros.h"
#include "UObject/ScriptMacros.h"

#include "Interfaces/OnlineSessionInterface.h"
#include "Interfaces/OnlineIdentityInterface.h"

#include "EOSSessionGameInstance.generated.h"

class USessionRowData;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSessionsSearchUpdated, int32, EffectiveCount);

UENUM(BlueprintType)
enum class EMatchGameMode : uint8
{
    QuickMatch UMETA(DisplayName="Quick Match"),
    Training UMETA(DisplayName="Training"),
    ChampionshipMatch UMETA(DisplayName="Championship Match")
};

USTRUCT(BlueprintType)
struct EOS_OSS_TUTORIAL_API FSessionRankingWeights
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="EOS|Sessions")
    float w_region = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="EOS|Sessions")
    float w_map = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="EOS|Sessions")
    float w_ruleset = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="EOS|Sessions")
    float w_slots = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="EOS|Sessions")
    float w_level = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="EOS|Sessions")
    float w_friends = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="EOS|Sessions")
    float w_age = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="EOS|Sessions")
    float w_rules = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="EOS|Sessions")
    float w_ping = 1.0f;
};

UCLASS()
class EOS_OSS_TUTORIAL_API UEOSSessionGameInstance : public UGameInstance
{
    GENERATED_BODY()

public:
    UEOSSessionGameInstance();

    // --- Delegates ---
    UPROPERTY(BlueprintAssignable, Category="EOS|Sessions")
    FOnSessionsSearchUpdated OnSessionsSearchUpdated;

    // --- Login ---
    // Matches existing BP nodes (Dev Auth Id + Credential Name)
    UFUNCTION(BlueprintCallable, Category="EOS|Auth")
    bool LoginWithDevAuth(bool bDevAuthId, const FString& CredentialName);

    // Compatibility overloads (NOT exposed to Blueprint).
    // These keep compilation working even if stale generated wrappers exist in Intermediate.
    bool LoginWithDevAuth(const FString& CredentialName);

    // --- Find/Host/Join ---
    UFUNCTION(BlueprintCallable, Category="EOS|Sessions")
    void FindSessions(int32 MaxResults);

    // Matches existing BP nodes (Session Name / Map Name / Region / Rule Set / Is Private / Players Per Team)
    UFUNCTION(BlueprintCallable, Category="EOS|Sessions")
    bool HostSession(const FString& SessionName, const FString& MapName, const FString& Region, const FString& RuleSet, bool bIsPrivate, int32 PlayersPerTeam);

    // Preferred Blueprint API: stable name (no overload ambiguity) + full parameter set.
    // Use this in Blueprints instead of HostSession if you have pin mismatch warnings.
    UFUNCTION(BlueprintCallable, Category="EOS|Sessions", meta=(DisplayName="Host Session Advanced"))
    bool HostSessionAdvanced(const FString& SessionName, const FString& MapName, const FString& Region, const FString& RuleSet, bool bIsPrivate, int32 PlayersPerTeam);


    // Compatibility overload (NOT exposed to Blueprint).
    bool HostSession(int32 NumPublicConnections, const FString& MapName, bool bIsPrivate);

    UFUNCTION(BlueprintCallable, Category="EOS|Sessions")
    void JoinSessionByIndex(int32 SearchResultIndex);

    UFUNCTION(BlueprintCallable, Category="EOS|Sessions")
    void JoinSessionByItem(USessionRowData* Item);

    // Data for UMG ListView
    UFUNCTION(BlueprintCallable, Category="EOS|Sessions")
    TArray<USessionRowData*> GetSessionListItems() const;

    // Optional helper expected by older generated code paths
    UFUNCTION(BlueprintPure, Category="EOS|Sessions")
    FString GameModeToString(EMatchGameMode Mode) const;

    // Weights exposed for optional sorting/ranking in BP.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="EOS|Sessions")
    FSessionRankingWeights RankingWeights;

protected:
    virtual void Init() override;

private:
    void EnsureOnlineInterfaces();

    void OnLoginComplete(int32 LocalUserNum, bool bWasSuccessful, const FUniqueNetId& UserId, const FString& Error);
    void OnFindSessionsComplete(bool bWasSuccessful);
    void OnCreateSessionComplete(FName SessionName, bool bWasSuccessful);
    void OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);

    void RebuildCachedItems();

private:
    // Online
    IOnlineSessionPtr SessionInterface;
    IOnlineIdentityPtr IdentityInterface;

    TSharedPtr<class FOnlineSessionSearch> SessionSearch;

    // Cached ListView items
    UPROPERTY()
    TArray<TObjectPtr<USessionRowData>> CachedItems;

    // Simple host settings cache
	bool bLastHostWasPrivate = false;
	FString LastHostSessionName;
	FString LastHostMap;
	FString LastHostRegion;
	FString LastHostRuleSet;
	int32 LastHostPlayersPerTeam = 0;
};

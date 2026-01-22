#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "OnlineSessionSettings.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "OnlineSubsystemTypes.h"
#include "OnlineSubsystem.h"
#include "EOSSessionGameInstance.generated.h"


// Clés de settings de session utilisées dans tout le projet.
// Ces macros n'existent pas dans l'engine, elles viennent des exemples type ShooterGame.
#ifndef SETTING_MAPNAME
#define SETTING_MAPNAME FName(TEXT("MAPNAME"))
#endif

#ifndef SETTING_DEDICATED
#define SETTING_DEDICATED FName(TEXT("DEDICATED"))
#endif

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSessionsSearchUpdated, int32, EffectiveCount);

/********************************************************************
 * SESSION FILTER PARAMETERS
 ********************************************************************/
USTRUCT(BlueprintType)
struct FSessionClientFilterParams
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bMatchRegionStrict = false;

    // Nouveau : filtrage explicite LAN / dedicated
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bFilterLANOnly = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bFilterDedicatedOnly = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bFilterMap = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bFilterRuleSet = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bRequireFreeSlots = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bFilterMaxAge = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bRequireMinPresentPlayers = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString Region;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString MapName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString RuleSet;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 MinFreeSlotsA = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 MinFreeSlotsB = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float MaxAgeMinutes = 9999.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 MinPresentPlayers = 0;
};

UENUM(BlueprintType)
enum class EMatchGameMode : uint8
{
    QuickMatch      UMETA(DisplayName = "QuickMatch"),
    Ranked      UMETA(DisplayName = "Ranked"),
    Training    UMETA(DisplayName = "Training"),
    ChampionshipMatch UMETA(DisplayName = "Championship Match"),
};

/********************************************************************
 * RANKING WEIGHTS
 ********************************************************************/
USTRUCT(BlueprintType)
struct FSessionRankingWeights
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float w_region = 1.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float w_map = 1.f;

    // Ancien champ, on le laisse au cas où tu lutilises en BP
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float w_ruleset = 1.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float w_slots = 1.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float w_level = 0.3f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float w_friends = 1.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float w_age = 1.f;

    // Nouveaux champs utilisés dans GetScoreForIndex
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float w_rules = 1.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float w_ping = 1.f;
};



UCLASS()
class EOS_OSS_TUTORIAL_API UEOSSessionGameInstance : public UGameInstance
{
    GENERATED_BODY()

public:

    UEOSSessionGameInstance();
    virtual ~UEOSSessionGameInstance() {}
    virtual void Init() override;

    UPROPERTY(BlueprintAssignable, Category = "Online|EOS")
    FOnSessionsSearchUpdated OnSessionsSearchUpdated;

    UFUNCTION(BlueprintPure, Category = "EOS|Sessions")
    static FString GameModeToString(EMatchGameMode GameMode);

    /********************************************************************
     * HOST
     ********************************************************************/
    UFUNCTION(BlueprintCallable, Category = "Online|EOS")
    bool HostSession(
        const FString& SessionName,
        const FString& MapName,
        const FString& Region,
        const FString& RuleSet,
        bool bIsPrivate,
        int32 PlayersPerTeam
    );

    bool HostSession(const FString& SessionName, const FString& MapName, const FString& Region, EMatchGameMode GameMode, bool bIsPrivate, int32 PlayersPerTeam);

    UFUNCTION(BlueprintCallable, Category = "Sessions")
    bool HostSessionWithGameMode(const FString& SessionName,
        const FString& MapName,
        const FString& Region,
        EMatchGameMode GameMode,
        bool bIsPrivate,
        int32 PlayersPerTeam);

    void OnCreateSessionComplete(FName SessionName, bool bWasSuccessful);


    /********************************************************************
     * FIND
     ********************************************************************/
    UFUNCTION(BlueprintCallable, Category = "Online|EOS")
    bool FindSessions(int32 MaxResults, bool bWithPresence);

    void OnFindSessionsComplete(bool bWasSuccessful);

    /** Raw authoritative Session Results */
    TArray<FOnlineSessionSearchResult> SearchResultsCache;// Cached search results (copie de SessionSearch->SearchResults)// Native results//


    /** Hôte DevAuthTool, par défaut "localhost:8081" */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Online|EOS|Auth")
    FString DevAuthHost;

    /** Lance un login DevAuth avec le CredentialName configuré dans DevAuthTool (ex: "Renaud", "Johan") */
    UFUNCTION(BlueprintCallable, Category = "Online|EOS|Auth")
    void LoginWithDevAuth(const FString& CredentialName);
    // EOSSessionGameInstance.h

    UPROPERTY(BlueprintReadOnly, Category = "EOS|Auth")
    FString LastCredentialName;   // "Johan" / "Renaud"

    UPROPERTY(BlueprintReadOnly, Category = "EOS|Auth")
    FString LocalDisplayName;     // ce qu'on expose comme nom joueur

    UFUNCTION(BlueprintCallable, Category = "EOS|Auth")
    FString GetLocalDisplayName() const { return LocalDisplayName; }

    /********************************************************************
     * CLIENT FILTERING
     ********************************************************************/
     // Utilisé par EOSSessionGameInstance.cpp (BuildFilteredIndex avec InFilters)
    UFUNCTION(BlueprintCallable, Category = "Online|EOS|Filtering")
    void BuildFilteredIndex(const FSessionClientFilterParams& FilterParams);

    /** Indices filtrés -> index bruts de SearchResultsCache */
    TArray<int32> FilteredToRawIndex;


    /********************************************************************
     * RANKING
     ********************************************************************/
    UFUNCTION(BlueprintCallable, Category = "Online|EOS|Ranking")
    void RankAndSortFiltered(const FSessionRankingWeights& Weights);

    /** Indices triés (toujours indices bruts de SearchResultsCache) */
    TArray<int32> RankedIndexes;

    /** Nombre effectif de sessions après filtrage / tri (pour lUI) */
    int32 EffectiveSessionCount = 0;

    /** Poids de ranking exposés aux BP / UI */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Online|EOS|Ranking")
    FSessionRankingWeights RankingWeights;

    // Nouvelle signature, alignée sur le .cpp et SessionMenuWidget.cpp
    float GetScoreForIndex(int32 RawIndex, const FSessionRankingWeights& Weights) const;


    /********************************************************************
     * JOIN SESSION
     ********************************************************************/
    UFUNCTION(BlueprintCallable, Category = "Online|EOS")
    bool JoinSessionByRawIndex(int32 RawIndex);

    void OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);

    void LeaveSession();

private:
    IOnlineSessionPtr SessionInterface;
    TSharedPtr<FOnlineSessionSearch> SessionSearch;
};

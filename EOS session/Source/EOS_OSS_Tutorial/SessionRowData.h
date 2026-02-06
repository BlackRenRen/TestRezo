#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "UObject/ObjectMacros.h"
#include "UObject/ScriptMacros.h"

#include "SessionRowData.generated.h"

UCLASS(BlueprintType)
class EOS_OSS_TUTORIAL_API USessionRowData : public UObject
{
    GENERATED_BODY()

public:
    // Index into the last FindSessions results (used to JoinSession).
    UPROPERTY(BlueprintReadOnly, Category="EOS|Sessions")
    int32 SearchResultIndex = INDEX_NONE;

    UPROPERTY(BlueprintReadOnly, Category="EOS|Sessions")
    FString SessionId;

    UPROPERTY(BlueprintReadOnly, Category="EOS|Sessions")
    FString SessionName;

    UPROPERTY(BlueprintReadOnly, Category="EOS|Sessions")
    FString HostName;

    UPROPERTY(BlueprintReadOnly, Category="EOS|Sessions")
    FString MapName;

    UPROPERTY(BlueprintReadOnly, Category="EOS|Sessions")
    FString Region;

    UPROPERTY(BlueprintReadOnly, Category="EOS|Sessions")
    FString RuleSet;

    // Legacy/compat slot fields (some UMG rows use these names).
    UPROPERTY(BlueprintReadOnly, Category="EOS|Sessions")
    int32 OpenPublicSlots = 0;

    UPROPERTY(BlueprintReadOnly, Category="EOS|Sessions")
    int32 MaxPublicSlots = 0;

    UPROPERTY(BlueprintReadOnly, Category="EOS|Sessions")
    int32 CurrentPlayers = 0;

    UPROPERTY(BlueprintReadOnly, Category="EOS|Sessions")
    int32 MaxPlayers = 0;

    UPROPERTY(BlueprintReadOnly, Category="EOS|Sessions")
    int32 PingMs = 0;

    UPROPERTY(BlueprintReadOnly, Category="EOS|Sessions")
    bool bIsPrivate = false;
};

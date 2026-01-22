#pragma once

#include "CoreMinimal.h"
#include "SessionRowData.generated.h"

USTRUCT(BlueprintType)
struct FSessionRowData
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Session")
    int32 RawIndex = INDEX_NONE;

    // Identifiant lisible de la session (GetSessionIdStr)
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Session")
    FText SessionId;

    // Nom du propriétaire (OwningUserName)
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Session")
    FText OwnerName;

    // Nom de session éventuel (si tu veux l’afficher à part)
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Session")
    FText SessionName;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Session")
    int32 CurrentPlayers = 0;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Session")
    int32 MaxPlayers = 0;

    // Ping en ms
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Session")
    int32 Ping = 0;

    // Nom de la map (SETTING_MAPNAME)
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Session")
    FText MapName;

    // Score client (résultat de GetScoreForIndex)
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Session")
    float Score = 0.f;
};

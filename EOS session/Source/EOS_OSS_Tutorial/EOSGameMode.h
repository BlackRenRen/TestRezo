// EOSGameMode.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "EOSGameMode.generated.h"

/**
 * GameMode used on the gameplay map (GameMap).
 *
 * Purpose:
 * - After ServerTravel to GameMap?listen in PIE, the default pawn spawn/possession path
 *   is sometimes skipped depending on timing/seamless travel.
 * - This GameMode enforces "a controller must end up with a pawn" via retries + a manual spawn fallback.
 */
UCLASS()
class EOS_OSS_TUTORIAL_API AEOSGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AEOSGameMode();

	virtual void BeginPlay() override;
	virtual void StartPlay() override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) override;
	virtual void PostSeamlessTravel() override;

private:
	FTimerHandle EnsurePawnTimer_Quick;
	FTimerHandle EnsurePawnTimer_Slow;

	void EnsurePawnsDelayedQuick();
	void EnsurePawnsDelayedSlow();
	void EnsurePawnsAll(const TCHAR* Context);
	bool EnsurePawnForController(APlayerController* PC, const TCHAR* Context);
};

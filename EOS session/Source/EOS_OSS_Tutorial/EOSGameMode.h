#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "EOSGameMode.generated.h"

UCLASS()
class EOS_OSS_TUTORIAL_API AEOSGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    AEOSGameMode();

    virtual void PostLogin(APlayerController* NewPlayer) override;
    virtual void Logout(AController* Exiting) override;
};

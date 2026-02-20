#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "MenuPlayerController.generated.h"

class USessionMenuWidget;

/**
 * PlayerController utilisé sur la MenuMap pour afficher le menu de sessions.
 */
UCLASS()
class EOS_OSS_TUTORIAL_API AMenuPlayerController : public APlayerController
{
    GENERATED_BODY()

public:
    virtual void BeginPlay() override;

    UPROPERTY(BlueprintReadWrite)
    USessionMenuWidget* CurrentMenuWidget = nullptr;

    void SetCurrentMenuWidget(USessionMenuWidget* InWidget)
    {
        CurrentMenuWidget = InWidget;
    }
protected:
    /** Classe du widget menu de sessions (WBP_SessionMenu) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI")
    TSubclassOf<USessionMenuWidget> SessionMenuClass;

    /** Instance active du menu de sessions */
    UPROPERTY()
    USessionMenuWidget* SessionMenu = nullptr;
};

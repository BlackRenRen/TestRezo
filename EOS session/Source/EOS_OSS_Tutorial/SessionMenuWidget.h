#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SessionMenuWidget.generated.h"

class UPanelWidget;
class USessionRowWidget;

UCLASS()
class EOS_OSS_TUTORIAL_API USessionMenuWidget : public UUserWidget
{
    GENERATED_BODY()

public:

    UFUNCTION(BlueprintCallable, Category = "Online|EOS|UI")
    void RefreshSessionList();

    void HandleJoinRequested(int32 RawIndex);

protected:

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UPanelWidget> SessionList;

    // Classe de widget pour chaque ligne de session
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Online|EOS|UI")
    TSubclassOf<USessionRowWidget> SessionRowClass;
};

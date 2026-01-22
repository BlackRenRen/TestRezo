#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "Components/Border.h"
#include "SessionRowData.h"
#include "Components/TextBlock.h"
#include "SessionRowWidget.generated.h"

class UEOSSessionGameInstance;

UCLASS()
class EOS_OSS_TUTORIAL_API USessionRowWidget : public UUserWidget
{
    GENERATED_BODY()

public:

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> JoinButton;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> ForceJoinButton;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UBorder> RootBorder;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> SessionIdText;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> MapText;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> PlayersText;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> PingText;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> OwnerText;


    // CORRECT TYPE
    FSessionRowData RowData;

    UFUNCTION()
    void OnJoinClicked();

    UFUNCTION()
    void OnForceJoinClicked();

    void Init(const FSessionRowData& InData);

protected:
    virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
    virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;
    virtual void NativeConstruct() override;
};

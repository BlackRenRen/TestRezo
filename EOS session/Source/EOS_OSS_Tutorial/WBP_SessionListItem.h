//#pragma once
//
//#include "CoreMinimal.h"
//#include "Blueprint/UserWidget.h"
//#include "SessionRowData.h"
//
//#include "WBP_SessionListItem.generated.h"
//
//class UButton;
//class UTextBlock;
//class USessionMenuWidget;
//
//UCLASS()
//class EOS_OSS_TUTORIAL_API UWBP_SessionListItem : public UUserWidget
//{
//    GENERATED_BODY()
//
//public:
//    UFUNCTION(BlueprintCallable)
//    void Init(const FSessionRowData& InData, USessionMenuWidget* InMenu);
//
//protected:
//
//    virtual void NativeConstruct() override;
//
//    UFUNCTION()
//    void OnJoinClicked();
//
//protected:
//
//    UPROPERTY(meta = (BindWidget))
//    UButton* JoinButton;
//
//    UPROPERTY(meta = (BindWidget))
//    UTextBlock* SessionNameText;
//
//    UPROPERTY(meta = (BindWidget))
//    UTextBlock* PlayerCountText;
//
//    FSessionRowData RowData;
//    USessionMenuWidget* Menu;
//};

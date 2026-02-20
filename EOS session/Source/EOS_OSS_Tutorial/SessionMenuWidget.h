#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"

#include "SessionMenuWidget.generated.h"

class UButton;
class UListView;
class UEOSSessionGameInstance;

UCLASS()
class EOS_OSS_TUTORIAL_API USessionMenuWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;

private:
	UFUNCTION()
	void HandleRefreshClicked();

	UFUNCTION()
	void HandleHostClicked();

	UFUNCTION()
	void HandleLoginRenaudClicked();

	UFUNCTION()
	void HandleLoginJohanClicked();

	UFUNCTION()
	void RebuildSessionList(int32 EffectiveCount);

private:
	// Bind these to your WBP_SessionMenuWidget widgets (names must match for BindWidget).
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = "true"))
	TObjectPtr<class UListView> SessionListView;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> RefreshButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> HostButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> LoginRenaudButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> LoginJohanButton;
};

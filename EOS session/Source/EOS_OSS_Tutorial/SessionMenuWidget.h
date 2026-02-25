#pragma once
// FIX_V28_MARKER

#include "CoreMinimal.h"
#include "CoreUObject.h"
#include "UObject/NoExportTypes.h"
#include "Blueprint/UserWidget.h"
#include "UObject/ObjectMacros.h"
#include "UObject/ScriptMacros.h"

#include "SessionMenuWidget.generated.h"

class UButton;
class UListView;

UCLASS()
class EOS_OSS_TUTORIAL_API USessionMenuWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	UFUNCTION()
	void HandleRefreshClicked();

	UFUNCTION()
	void HandleHostClicked();

	UFUNCTION()
	void HandleLoginRenaudClicked();

	UFUNCTION()
	void HandleLoginJohanClicked();

	// Dynamic multicast delegate callback (must be UFUNCTION for AddDynamic).
	UFUNCTION()
	void HandleSessionsSearchUpdated(int32 EffectiveCount);

	UFUNCTION(BlueprintCallable)
	void RebuildSessionList();

public:
	// Public to avoid access-control issues if UHT generation is disrupted.
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = "true"))
	TObjectPtr<UListView> SessionListView = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> RefreshButton = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> HostButton = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> LoginRenaudButton = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> LoginJohanButton = nullptr;

private:
	// NOTE: Dynamic multicast delegates don't use FDelegateHandle. We unbind via RemoveAll(this).
};

#pragma once
// FIX_V28_MARKER

#include "CoreMinimal.h"
#include "CoreUObject.h"
#include "UObject/NoExportTypes.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/IUserObjectListEntry.h"
#include "UObject/ObjectMacros.h"
#include "UObject/ScriptMacros.h"

#include "SessionRowWidget.generated.h"

class UButton;
class UTextBlock;
class USessionRowData;

UCLASS()
class EOS_OSS_TUTORIAL_API USessionRowWidget : public UUserWidget, public IUserObjectListEntry
{
	GENERATED_BODY()

public:
	virtual void NativeOnInitialized() override;
	virtual void NativeOnListItemObjectSet(UObject* ListItemObject) override;

protected:
	UFUNCTION()
	void HandleJoinClicked();

	void RefreshFromItem();

public:
	// Keep these PUBLIC to avoid any MSVC access-control issues if UHT generation is disrupted.
	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> SessionIdText = nullptr;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> OwnerNameText = nullptr;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> PlayersText = nullptr;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> PingText = nullptr;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> MapText = nullptr;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UButton> JoinButton = nullptr;

private:
	UPROPERTY(Transient)
	TObjectPtr<USessionRowData> Item = nullptr;
};

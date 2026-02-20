#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/IUserObjectListEntry.h"
#include "SessionRowWidget.generated.h"

class UButton;
class UTextBlock;
class USessionRowData;

/**
 * ListView entry widget for a discovered EOS session.
 *
 * Blueprint requirements (all optional but recommended):
 * - TextBlocks named: SessionIdText, OwnerNameText, PlayersText, PingText
 * - Button named: JoinButton
 */
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

	UPROPERTY(meta=(BindWidgetOptional))
	UTextBlock* SessionIdText = nullptr;

	UPROPERTY(meta=(BindWidgetOptional))
	UTextBlock* OwnerNameText = nullptr;

	UPROPERTY(meta=(BindWidgetOptional))
	UTextBlock* PlayersText = nullptr;

	UPROPERTY(meta=(BindWidgetOptional))
	UTextBlock* PingText = nullptr;

	UPROPERTY(meta=(BindWidgetOptional))
	UButton* JoinButton = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<USessionRowData> CurrentItem = nullptr;
};

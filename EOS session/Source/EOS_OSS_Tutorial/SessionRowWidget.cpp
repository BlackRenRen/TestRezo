#include "SessionRowWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"

#include "EOSSessionGameInstance.h"
#include "SessionRowData.h"

void USessionRowWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (JoinButton)
	{
		JoinButton->OnClicked.AddDynamic(this, &USessionRowWidget::HandleJoinClicked);
	}
}

void USessionRowWidget::NativeOnListItemObjectSet(UObject* ListItemObject)
{
	CurrentItem = Cast<USessionRowData>(ListItemObject);
	RefreshFromItem();
}

void USessionRowWidget::RefreshFromItem()
{
	if (!CurrentItem)
	{
		if (SessionIdText) SessionIdText->SetText(FText::GetEmpty());
		if (OwnerNameText) OwnerNameText->SetText(FText::GetEmpty());
		if (PlayersText) PlayersText->SetText(FText::GetEmpty());
		if (PingText) PingText->SetText(FText::GetEmpty());
		if (JoinButton) JoinButton->SetIsEnabled(false);
		return;
	}

	if (SessionIdText)
	{
		SessionIdText->SetText(FText::FromString(CurrentItem->SessionId));
	}

	if (OwnerNameText)
	{
		OwnerNameText->SetText(FText::FromString(CurrentItem->HostName));
	}

	if (PlayersText)
	{
		TArray<FStringFormatArg> Args;
		Args.Reserve(2);
		Args.Add(CurrentItem->CurrentPlayers);
		Args.Add(CurrentItem->MaxPlayers);
		PlayersText->SetText(FText::FromString(FString::Format(TEXT("{0}/{1}"), Args)));
	}

	if (PingText)
	{
		if (CurrentItem->PingMs >= 0)
		{
			TArray<FStringFormatArg> Args;
			Args.Reserve(1);
			Args.Add(CurrentItem->PingMs);
			PingText->SetText(FText::FromString(FString::Format(TEXT("{0} ms"), Args)));
		}
		else
		{
			PingText->SetText(FText::FromString(TEXT("-")));
		}
	}

	if (JoinButton)
	{
		JoinButton->SetIsEnabled(CurrentItem->bIsJoinable && CurrentItem->SearchResultIndex != INDEX_NONE);
	}
}

void USessionRowWidget::HandleJoinClicked()
{
	if (!CurrentItem)
	{
		return;
	}

	UEOSSessionGameInstance* GI = Cast<UEOSSessionGameInstance>(GetGameInstance());
	if (!GI)
	{
		UE_LOG(LogTemp, Warning, TEXT("SessionRowWidget: GameInstance is not UEOSSessionGameInstance"));
		return;
	}

	// Prefer the helper (keeps UI decoupled).
	GI->JoinSessionByItem(CurrentItem);
}

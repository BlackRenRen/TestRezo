#include "SessionRowWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Kismet/GameplayStatics.h"

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
	Item = Cast<USessionRowData>(ListItemObject);
	RefreshFromItem();
}

void USessionRowWidget::RefreshFromItem()
{
	if (!Item)
	{
		if (SessionIdText) SessionIdText->SetText(FText::GetEmpty());
		if (OwnerNameText) OwnerNameText->SetText(FText::GetEmpty());
		if (PlayersText) PlayersText->SetText(FText::GetEmpty());
		if (PingText) PingText->SetText(FText::GetEmpty());
		if (MapText) MapText->SetText(FText::GetEmpty());
		if (JoinButton) JoinButton->SetIsEnabled(false);
		return;
	}

	if (SessionIdText) SessionIdText->SetText(FText::FromString(Item->SessionId));
	if (OwnerNameText) OwnerNameText->SetText(FText::FromString(Item->HostName));

	if (PlayersText)
	{
		const FString Players = FString::Printf(TEXT("%d/%d"), Item->CurrentPlayers, Item->MaxPlayers);
		PlayersText->SetText(FText::FromString(Players));
	}

	if (PingText)
	{
		const FString Ping = FString::Printf(TEXT("%d ms"), Item->PingMs);
		PingText->SetText(FText::FromString(Ping));
	}

	if (MapText) MapText->SetText(FText::FromString(Item->MapName));

	// Enable join if index is valid. (Do not depend on slots; EOS sometimes reports 0 open for a moment.)
	if (JoinButton) JoinButton->SetIsEnabled(Item->SearchResultIndex != INDEX_NONE);
}

void USessionRowWidget::HandleJoinClicked()
{
	UEOSSessionGameInstance* GI = Cast<UEOSSessionGameInstance>(GetGameInstance());
	if (!GI || !Item)
	{
		return;
	}
	GI->JoinSessionByItem(Item);
}

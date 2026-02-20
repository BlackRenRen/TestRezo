#include "SessionMenuWidget.h"

#include "Components/Button.h"
#include "Components/ListView.h"

#include "EOSSessionGameInstance.h"
#include "SessionRowData.h"

void USessionMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (RefreshButton)
	{
		RefreshButton->OnClicked.AddDynamic(this, &USessionMenuWidget::HandleRefreshClicked);
	}
	if (HostButton)
	{
		HostButton->OnClicked.AddDynamic(this, &USessionMenuWidget::HandleHostClicked);
	}
	if (LoginRenaudButton)
	{
		LoginRenaudButton->OnClicked.AddDynamic(this, &USessionMenuWidget::HandleLoginRenaudClicked);
	}
	if (LoginJohanButton)
	{
		LoginJohanButton->OnClicked.AddDynamic(this, &USessionMenuWidget::HandleLoginJohanClicked);
	}

	if (UEOSSessionGameInstance* GI = Cast<UEOSSessionGameInstance>(GetGameInstance()))
	{
		// Update list when new search results arrive.
		GI->OnSessionsSearchUpdated.RemoveAll(this);
		GI->OnSessionsSearchUpdated.AddDynamic(this, &USessionMenuWidget::RebuildSessionList);
	}

	RebuildSessionList(0);
}

void USessionMenuWidget::HandleRefreshClicked()
{
	if (UEOSSessionGameInstance* GI = Cast<UEOSSessionGameInstance>(GetGameInstance()))
	{
		// Results are async. RebuildSessionList() is called via OnSessionsSearchUpdated.
		GI->FindSessions(2000);
	}
}

void USessionMenuWidget::HandleHostClicked()
{
	if (UEOSSessionGameInstance* GI = Cast<UEOSSessionGameInstance>(GetGameInstance()))
	{
		// Keep it simple: host a public session with 4 public connections on a gameplay map.
		// Adjust MapName to your actual gameplay map path.
		GI->HostSession_Legacy(4, TEXT("/Game/ThirdPerson/Maps/ThirdPersonMap"), false);
	}
}

void USessionMenuWidget::HandleLoginRenaudClicked()
{
	if (UEOSSessionGameInstance* GI = Cast<UEOSSessionGameInstance>(GetGameInstance()))
	{
		// DevAuthTool credential name (as configured in the DevAuthTool UI).
		GI->LoginWithDevAuth_Legacy(TEXT("Renaud"));
	}
}

void USessionMenuWidget::HandleLoginJohanClicked()
{
	if (UEOSSessionGameInstance* GI = Cast<UEOSSessionGameInstance>(GetGameInstance()))
	{
		GI->LoginWithDevAuth_Legacy(TEXT("Johan"));
	}
}

void USessionMenuWidget::RebuildSessionList(int32 /*EffectiveCount*/)
{
	if (!SessionListView)
	{
		UE_LOG(LogTemp, Warning, TEXT("SessionMenuWidget: SessionListView is null"));
		return;
	}

	UEOSSessionGameInstance* GI = Cast<UEOSSessionGameInstance>(GetGameInstance());
	if (!GI)
	{
		SessionListView->ClearListItems();
		return;
	}

	const TArray<USessionRowData*> Items = GI->GetSessionListItems();

	SessionListView->ClearListItems();
	for (USessionRowData* Item : Items)
	{
		if (IsValid(Item))
		{
			SessionListView->AddItem(Item);
		}
	}
}

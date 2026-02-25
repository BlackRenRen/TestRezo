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
		RefreshButton->OnClicked.Clear();
		RefreshButton->OnClicked.AddDynamic(this, &USessionMenuWidget::HandleRefreshClicked);
	}
	if (HostButton)
	{
		HostButton->OnClicked.Clear();
		HostButton->OnClicked.AddDynamic(this, &USessionMenuWidget::HandleHostClicked);
	}
	if (LoginRenaudButton)
	{
		LoginRenaudButton->OnClicked.Clear();
		LoginRenaudButton->OnClicked.AddDynamic(this, &USessionMenuWidget::HandleLoginRenaudClicked);
	}
	if (LoginJohanButton)
	{
		LoginJohanButton->OnClicked.Clear();
		LoginJohanButton->OnClicked.AddDynamic(this, &USessionMenuWidget::HandleLoginJohanClicked);
	}

	if (UEOSSessionGameInstance* GI = Cast<UEOSSessionGameInstance>(GetGameInstance()))
	{
		// Dynamic multicast delegate (BlueprintAssignable): bind via AddDynamic and unbind via RemoveAll(this).
		GI->OnSessionsSearchUpdated.RemoveAll(this);
		GI->OnSessionsSearchUpdated.AddDynamic(this, &USessionMenuWidget::HandleSessionsSearchUpdated);
	}

	RebuildSessionList();
}

void USessionMenuWidget::NativeDestruct()
{
	if (UEOSSessionGameInstance* GI = Cast<UEOSSessionGameInstance>(GetGameInstance()))
	{
		GI->OnSessionsSearchUpdated.RemoveAll(this);
	}

	Super::NativeDestruct();
}

void USessionMenuWidget::HandleRefreshClicked()
{
	if (UEOSSessionGameInstance* GI = Cast<UEOSSessionGameInstance>(GetGameInstance()))
	{
		GI->FindSessions(2000);
	}
}

void USessionMenuWidget::HandleHostClicked()
{
	if (UEOSSessionGameInstance* GI = Cast<UEOSSessionGameInstance>(GetGameInstance()))
	{
		GI->HostSession(4, TEXT("/Game/ThirdPerson/Maps/ThirdPersonMap"), false);
	}
}

void USessionMenuWidget::HandleLoginRenaudClicked()
{
	if (UEOSSessionGameInstance* GI = Cast<UEOSSessionGameInstance>(GetGameInstance()))
	{
		GI->LoginPreferDevAuth(TEXT("Renaud"));
	}
}

void USessionMenuWidget::HandleLoginJohanClicked()
{
	if (UEOSSessionGameInstance* GI = Cast<UEOSSessionGameInstance>(GetGameInstance()))
	{
		GI->LoginPreferDevAuth(TEXT("Johan"));
	}
}

void USessionMenuWidget::HandleSessionsSearchUpdated(int32 /*EffectiveCount*/)
{
	RebuildSessionList();
}

void USessionMenuWidget::RebuildSessionList()
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
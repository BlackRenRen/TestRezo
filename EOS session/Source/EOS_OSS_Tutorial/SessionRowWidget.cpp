#include "SessionRowWidget.h"
#include "SessionRowData.h"
#include "EOSSessionGameInstance.h"
#include "Kismet/GameplayStatics.h"



void USessionRowWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (JoinButton)
        JoinButton->OnClicked.AddDynamic(this, &USessionRowWidget::OnJoinClicked);

    if (ForceJoinButton)
        ForceJoinButton->OnClicked.AddDynamic(this, &USessionRowWidget::OnForceJoinClicked);
}

void USessionRowWidget::Init(const FSessionRowData& InData)
{
    RowData = InData;

    if (SessionIdText) SessionIdText->SetText(RowData.SessionId);
    if (MapText)       MapText->SetText(RowData.MapName);

    if (PlayersText)
    {
        PlayersText->SetText(FText::FromString(
            FString::Printf(TEXT("%d/%d"), RowData.CurrentPlayers, RowData.MaxPlayers)));
    }

    if (PingText)
    {
        PingText->SetText(FText::FromString(
            FString::Printf(TEXT("%d ms"), RowData.Ping)));
    }
    if (OwnerText)
    {
        const FText SafeOwner = RowData.OwnerName.IsEmpty()
            ? FText::FromString(TEXT("Unknown"))
            : RowData.OwnerName;
        OwnerText->SetText(SafeOwner);
    }
}


void USessionRowWidget::OnJoinClicked()
{
    if (UEOSSessionGameInstance* GI = Cast<UEOSSessionGameInstance>(UGameplayStatics::GetGameInstance(this)))
    {
        GI->JoinSessionByRawIndex(RowData.RawIndex);
    }
}

void USessionRowWidget::OnForceJoinClicked()
{
    if (UEOSSessionGameInstance* GI = Cast<UEOSSessionGameInstance>(UGameplayStatics::GetGameInstance(this)))
    {
        GI->JoinSessionByRawIndex(RowData.RawIndex);
    }
}


void USessionRowWidget::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    Super::NativeOnMouseEnter(InGeometry, InMouseEvent);

    if (RootBorder)
        RootBorder->SetBrushColor(FLinearColor(0.20f, 0.20f, 0.25f, 1.f));
}

void USessionRowWidget::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
    Super::NativeOnMouseLeave(InMouseEvent);

    if (RootBorder)
        RootBorder->SetBrushColor(FLinearColor(0.10f, 0.10f, 0.10f, 1.f));
}
    
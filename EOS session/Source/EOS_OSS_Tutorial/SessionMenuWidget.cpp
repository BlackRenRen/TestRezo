#include "SessionMenuWidget.h"

#include "Components/PanelWidget.h"
#include "EOSSessionGameInstance.h"
#include "SessionRowWidget.h"
#include "SessionRowData.h"
#include "Kismet/GameplayStatics.h"

static FString ShortId(const FString& Full)
{
    if (Full.Len() <= 8) return Full;
    return Full.Left(8);
    // ou Full.Right(8)
}

void USessionMenuWidget::RefreshSessionList()
{
    UE_LOG(LogTemp, Log, TEXT("USessionMenuWidget::Enter RefreshSessionList()"));

    if (!SessionList)
    {
        return;
    }

    SessionList->ClearChildren();

    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    UEOSSessionGameInstance* GI = World->GetGameInstance<UEOSSessionGameInstance>();
    if (!GI)
    {
        return;
    }

    // Vue filtrée & triée
    const TArray<FOnlineSessionSearchResult>& Results = GI->SearchResultsCache;
    const TArray<int32>& Ranked = GI->RankedIndexes;
    const TArray<int32>& Filtered = GI->FilteredToRawIndex;

    UE_LOG(LogTemp, Warning, TEXT("[UI] Results=%d Ranked=%d Filtered=%d Eff=%d RowClass=%s"),
        Results.Num(), Ranked.Num(), Filtered.Num(), GI->EffectiveSessionCount,
        *GetNameSafe(SessionRowClass));

    if (Results.Num() <= 0 || !SessionRowClass)
    {
        return;
    }


    // On utilise en priorité Rankés; sinon, on retombe sur filtrés.
    const TArray<int32>* View = &Ranked;
    if (View->Num() == 0)
    {
        View = &Filtered;
    }

    UE_LOG(LogTemp, Warning, TEXT("[UI] Using View size=%d"), View->Num());

    // Si vraiment rien, on ne remplit pas.
    if (View->Num() == 0)
    {
        return;
    }

    const int32 SafeCount = View->Num();
    for (int32 ViewIdx = 0; ViewIdx < SafeCount; ++ViewIdx)
    {
        UE_LOG(LogTemp, Log, TEXT("USessionMenuWidget::ViewIdx : %d"), ViewIdx);

        if (!View->IsValidIndex(ViewIdx))
        {
            UE_LOG(LogTemp, Log, TEXT("USessionMenuWidget::invalid index"));
            continue;
        }

        const int32 RawIdx = (*View)[ViewIdx];
        if (!Results.IsValidIndex(RawIdx))
        {
            UE_LOG(LogTemp, Log, TEXT("USessionMenuWidget::invalid index2"));
            continue;
        }

        const FOnlineSessionSearchResult& Result = Results[RawIdx];
        const FOnlineSession& Session = Result.Session;
        const FOnlineSessionSettings& Settings = Session.SessionSettings;

        UUserWidget* BaseRow = CreateWidget<UUserWidget>(this, SessionRowClass);
        USessionRowWidget* RowWidget = Cast<USessionRowWidget>(BaseRow);
        if (!RowWidget)
        {
            UE_LOG(LogTemp, Log, TEXT("USessionMenuWidget:: unableto create USessionRowWidget widget !"));
            continue;
        }

        FSessionRowData RowData;
        RowData.RawIndex = RawIdx;
        RowData.SessionId = FText::FromString(ShortId(Result.GetSessionIdStr()));

        //RowData.SessionId = FText::FromString(Result.GetSessionIdStr());
        FString HostName;
        if (Settings.Get(FName(TEXT("HOST_NAME")), HostName) && !HostName.IsEmpty())
        {
            RowData.OwnerName = FText::FromString(HostName);
        }
        else if (!Session.OwningUserName.IsEmpty())
        {
            RowData.OwnerName = FText::FromString(Session.OwningUserName);
        }
        else
        {
            RowData.OwnerName = FText::FromString(TEXT("Unknown"));
        }

        const int32 TotalSlots =
            Settings.NumPublicConnections +
            Settings.NumPrivateConnections;

        const int32 OpenSlots =
            Session.NumOpenPublicConnections +
            Session.NumOpenPrivateConnections;

        const int32 PresentPlayers = FMath::Max(TotalSlots - OpenSlots, 0);

        RowData.MaxPlayers = TotalSlots;
        RowData.CurrentPlayers = PresentPlayers;
        RowData.Ping = Result.PingInMs;

        // Score côté client : on réutilise la même fonction que pour le tri
        RowData.Score = GI->GetScoreForIndex(RawIdx, GI->RankingWeights);

        // Nom de map (optionnel)
        FString MapName;
        if (Settings.Get(SETTING_MAPNAME, MapName))
        {
            RowData.MapName = FText::FromString(MapName);
        }
        else
        {
            RowData.MapName = FText::GetEmpty();
        }

        RowWidget->Init(RowData);

        SessionList->AddChild(RowWidget);
        UE_LOG(LogTemp, Log, TEXT("USessionMenuWidget:: line created"));
    }
}


void USessionMenuWidget::HandleJoinRequested(int32 RawIndex)
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    if (UEOSSessionGameInstance* GI = World->GetGameInstance<UEOSSessionGameInstance>())
    {
        GI->JoinSessionByRawIndex(RawIndex);
    }
}

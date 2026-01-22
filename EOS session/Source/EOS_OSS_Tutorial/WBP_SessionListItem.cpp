//// WBP_SessionListItem.cpp
//
//#include "WBP_SessionListItem.h"
//#include "SessionMenuWidget.h"
//#include "Components/Button.h"
//#include "Components/TextBlock.h"
//
//void UWBP_SessionListItem::NativeConstruct()
//{
//    Super::NativeConstruct();
//
//    // On s’assure que le bouton est bien bindé une seule fois
//    if (JoinButton && !JoinButton->OnClicked.IsAlreadyBound(this, &UWBP_SessionListItem::OnJoinClicked))
//    {
//        JoinButton->OnClicked.AddDynamic(this, &UWBP_SessionListItem::OnJoinClicked);
//    }
//}
//
//void UWBP_SessionListItem::Init(const FSessionRowData& InData, USessionMenuWidget* InMenu)
//{
//    RowData = InData;
//    Menu = InMenu;
//
//
//
//    if (SessionNameText)
//    {
//        // Affiche SessionName si présent, sinon SessionId
//        const FText DisplayName = RowData.SessionName.IsEmpty() ? RowData.SessionId : RowData.SessionName;
//        SessionNameText->SetText(DisplayName);
//    }
//
//    if (PlayerCountText)
//    {
//        const FString PlayersStr = FString::Printf(TEXT("%d / %d"), RowData.CurrentPlayers, RowData.MaxPlayers);
//        PlayerCountText->SetText(FText::FromString(PlayersStr));
//    }
//
//    if (JoinButton && !JoinButton->OnClicked.IsAlreadyBound(this, &UWBP_SessionListItem::OnJoinClicked))
//    {
//        JoinButton->OnClicked.AddDynamic(this, &UWBP_SessionListItem::OnJoinClicked);
//    }
//}
//
//
//void UWBP_SessionListItem::OnJoinClicked()
//{
//    if (Menu)
//    {
//        Menu->HandleJoinRequested(RowData.RawIndex);
//    }
//}

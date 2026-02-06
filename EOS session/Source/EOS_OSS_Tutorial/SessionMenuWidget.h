#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UObject/ObjectMacros.h"
#include "UObject/ScriptMacros.h"

#include "SessionMenuWidget.generated.h"

/**
 * NOTE: This class is intentionally minimal.
 * If you prefer a pure Blueprint widget, do not derive your WBP from this.
 */
UCLASS()
class EOS_OSS_TUTORIAL_API USessionMenuWidget : public UUserWidget
{
	GENERATED_BODY()
};

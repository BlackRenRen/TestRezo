#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/IUserObjectListEntry.h"
#include "UObject/ObjectMacros.h"
#include "UObject/ScriptMacros.h"

#include "SessionRowWidget.generated.h"

UCLASS()
class EOS_OSS_TUTORIAL_API USessionRowWidget : public UUserWidget, public IUserObjectListEntry
{
	GENERATED_BODY()
};

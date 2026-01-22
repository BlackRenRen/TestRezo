// EOS_OSS_TutorialCharacter.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "EOS_OSS_TutorialCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;

UCLASS()
class EOS_OSS_TUTORIAL_API AEOS_OSS_TutorialCharacter : public ACharacter
{
    GENERATED_BODY()

public:
    AEOS_OSS_TutorialCharacter();

protected:
    virtual void BeginPlay() override;

    /** Bind input axes */
    virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

    /** Avancer / reculer (flèches haut / bas) */
    void MoveForward(float Value);

    /** Tourner (flèches gauche / droite) */
    void Turn(float Value);

protected:
    /** Boom de caméra (3rd person) */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
    USpringArmComponent* CameraBoom;

    /** Caméra qui suit le personnage */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
    UCameraComponent* FollowCamera;
};

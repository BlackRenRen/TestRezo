// EOS_OSS_TutorialCharacter.cpp

#include "EOS_OSS_TutorialCharacter.h"

#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Controller.h"

DEFINE_LOG_CATEGORY_STATIC(LogEOSTutChar, Log, All);

AEOS_OSS_TutorialCharacter::AEOS_OSS_TutorialCharacter()
{
    UE_LOG(LogEOSTutChar, Log, TEXT("AEOS_OSS_TutorialCharacter::CTOR"));

    // Capsule par défaut
    GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

    // On laisse le mouvement orienter la rotation (3rd person classique)
    bUseControllerRotationPitch = false;
    bUseControllerRotationYaw = false;
    bUseControllerRotationRoll = false;

    UCharacterMovementComponent* MoveComp = GetCharacterMovement();
    MoveComp->bOrientRotationToMovement = true;
    MoveComp->RotationRate = FRotator(0.f, 540.f, 0.f);
    MoveComp->JumpZVelocity = 600.f;
    MoveComp->AirControl = 0.2f;

    // Boom de caméra
    CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
    CameraBoom->SetupAttachment(RootComponent);
    CameraBoom->TargetArmLength = 400.0f;
    CameraBoom->bUsePawnControlRotation = true;

    // Caméra de suivi
    FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
    FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
    FollowCamera->bUsePawnControlRotation = false; // le boom gère la rotation
}

void AEOS_OSS_TutorialCharacter::BeginPlay()
{
    Super::BeginPlay();

    UE_LOG(LogEOSTutChar, Log, TEXT("AEOS_OSS_TutorialCharacter::BeginPlay - Role=%d, LocalRole=%d, IsLocallyControlled=%d"),
        (int32)GetLocalRole(), (int32)GetRemoteRole(), IsLocallyControlled() ? 1 : 0);
}

void AEOS_OSS_TutorialCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    UE_LOG(LogEOSTutChar, Log, TEXT("AEOS_OSS_TutorialCharacter::SetupPlayerInputComponent"));

    check(PlayerInputComponent);

    // Bind des axes classiques
    PlayerInputComponent->BindAxis(TEXT("MoveForward"), this, &AEOS_OSS_TutorialCharacter::MoveForward);
    PlayerInputComponent->BindAxis(TEXT("Turn"), this, &AEOS_OSS_TutorialCharacter::Turn);

    // Si tu veux sauter un jour :
    // PlayerInputComponent->BindAction(TEXT("Jump"), IE_Pressed, this, &ACharacter::Jump);
    // PlayerInputComponent->BindAction(TEXT("Jump"), IE_Released, this, &ACharacter::StopJumping);
}

void AEOS_OSS_TutorialCharacter::MoveForward(float Value)
{
    if (!Controller || FMath::IsNearlyZero(Value))
    {
        return;
    }

    UE_LOG(LogEOSTutChar, Verbose, TEXT("MoveForward Value=%f IsLocallyControlled=%d"), Value, IsLocallyControlled() ? 1 : 0);

    // on ne prend que le yaw du contrôleur
    const FRotator ControlRot = Controller->GetControlRotation();
    const FRotator YawRot(0.f, ControlRot.Yaw, 0.f);

    const FVector Direction = FRotationMatrix(YawRot).GetUnitAxis(EAxis::X);
    AddMovementInput(Direction, Value);
}

void AEOS_OSS_TutorialCharacter::Turn(float Value)
{
    if (FMath::IsNearlyZero(Value))
    {
        return;
    }

    UE_LOG(LogEOSTutChar, Verbose, TEXT("Turn Value=%f IsLocallyControlled=%d"), Value, IsLocallyControlled() ? 1 : 0);

    // Rotation yaw de la caméra / du contrôleur
    AddControllerYawInput(Value);
}

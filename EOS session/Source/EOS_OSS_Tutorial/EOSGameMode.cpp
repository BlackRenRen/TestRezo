// EOSGameMode.cpp

#include "EOSGameMode.h"

#include "Engine/World.h"
#include "TimerManager.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"

AEOSGameMode::AEOSGameMode()
{
	// Keep defaults in BP_EOSGameMode (DefaultPawnClass, PlayerControllerClass, etc.).
	// This class only adds robustness around spawning/possessing.
}

void AEOSGameMode::BeginPlay()
{
	Super::BeginPlay();

	EnsurePawnsAll(TEXT("BeginPlay"));

	// Retry shortly after (covers PlayerStart/world-partition timing issues after travel).
	GetWorldTimerManager().SetTimer(EnsurePawnTimer_Quick, this, &AEOSGameMode::EnsurePawnsDelayedQuick, 0.25f, false);
	GetWorldTimerManager().SetTimer(EnsurePawnTimer_Slow, this, &AEOSGameMode::EnsurePawnsDelayedSlow, 1.00f, false);
}

void AEOSGameMode::StartPlay()
{
	Super::StartPlay();
	EnsurePawnsAll(TEXT("StartPlay"));
}

void AEOSGameMode::PostSeamlessTravel()
{
	Super::PostSeamlessTravel();

	EnsurePawnsAll(TEXT("PostSeamlessTravel"));

	// Re-arm retries after travel.
	GetWorldTimerManager().SetTimer(EnsurePawnTimer_Quick, this, &AEOSGameMode::EnsurePawnsDelayedQuick, 0.25f, false);
	GetWorldTimerManager().SetTimer(EnsurePawnTimer_Slow, this, &AEOSGameMode::EnsurePawnsDelayedSlow, 1.00f, false);
}

void AEOSGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);
	EnsurePawnForController(NewPlayer, TEXT("PostLogin"));
}

void AEOSGameMode::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
	Super::HandleStartingNewPlayer_Implementation(NewPlayer);
	EnsurePawnForController(NewPlayer, TEXT("HandleStartingNewPlayer"));
}

void AEOSGameMode::EnsurePawnsDelayedQuick()
{
	EnsurePawnsAll(TEXT("DelayedQuick"));
}

void AEOSGameMode::EnsurePawnsDelayedSlow()
{
	EnsurePawnsAll(TEXT("DelayedSlow"));
}

static const TCHAR* NetModeToString(ENetMode NetMode)
{
	switch (NetMode)
	{
	case NM_Standalone: return TEXT("Standalone");
	case NM_DedicatedServer: return TEXT("DedicatedServer");
	case NM_ListenServer: return TEXT("ListenServer");
	case NM_Client: return TEXT("Client");
	default: return TEXT("Unknown");
	}
}

void AEOSGameMode::EnsurePawnsAll(const TCHAR* Context)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	int32 NumPC = 0;
	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		++NumPC;
	}

	UE_LOG(LogTemp, Log, TEXT("[EOSGameMode] EnsurePawnsAll(%s): NetMode=%s NumPC=%d DefaultPawnClass=%s"),
		Context,
		NetModeToString(World->GetNetMode()),
		NumPC,
		*GetNameSafe(DefaultPawnClass));

	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		if (!IsValid(PC))
		{
			continue;
		}

		EnsurePawnForController(PC, Context);
	}
}

bool AEOSGameMode::EnsurePawnForController(APlayerController* PC, const TCHAR* Context)
{
	if (!IsValid(PC))
	{
		return false;
	}

	if (IsValid(PC->GetPawn()))
	{
		return true;
	}

	UE_LOG(LogTemp, Warning, TEXT("[EOSGameMode] %s: PC=%s has no pawn -> forcing spawn/possess"),
		Context,
		*GetNameSafe(PC));

	// 1) Standard path.
	RestartPlayer(PC);
	if (IsValid(PC->GetPawn()))
	{
		UE_LOG(LogTemp, Log, TEXT("[EOSGameMode] %s: PC=%s pawn spawned via RestartPlayer: %s"),
			Context, *GetNameSafe(PC), *GetNameSafe(PC->GetPawn()));
		return true;
	}

	// 2) Try to resolve a start spot.
	AActor* StartSpot = FindPlayerStart(PC);
	if (!IsValid(StartSpot))
	{
		StartSpot = UGameplayStatics::GetActorOfClass(this, APlayerStart::StaticClass());
	}

	if (!IsValid(StartSpot))
	{
		UE_LOG(LogTemp, Error, TEXT("[EOSGameMode] %s: No PlayerStart found. Can't spawn pawn for PC=%s"),
			Context, *GetNameSafe(PC));
		return false;
	}

	// 3) Manual spawn fallback with permissive collision handling.
	UClass* PawnClass = GetDefaultPawnClassForController(PC);
	if (!PawnClass)
	{
		PawnClass = DefaultPawnClass;
	}

	if (!PawnClass)
	{
		UE_LOG(LogTemp, Error, TEXT("[EOSGameMode] %s: PawnClass is null (DefaultPawnClass not set). PC=%s"),
			Context, *GetNameSafe(PC));
		return false;
	}

	const FTransform SpawnTM = StartSpot->GetActorTransform();

	FActorSpawnParameters Params;
	Params.Owner = PC;
	Params.Instigator = PC->GetPawn();
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	APawn* NewPawn = GetWorld()->SpawnActor<APawn>(PawnClass, SpawnTM, Params);
	if (!IsValid(NewPawn))
	{
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		NewPawn = GetWorld()->SpawnActor<APawn>(PawnClass, SpawnTM, Params);
	}

	if (!IsValid(NewPawn))
	{
		UE_LOG(LogTemp, Error, TEXT("[EOSGameMode] %s: Manual spawn failed. PawnClass=%s StartSpot=%s"),
			Context, *GetNameSafe(PawnClass), *GetNameSafe(StartSpot));
		return false;
	}

	PC->Possess(NewPawn);

	UE_LOG(LogTemp, Log, TEXT("[EOSGameMode] %s: PC=%s possessed pawn=%s"),
		Context, *GetNameSafe(PC), *GetNameSafe(NewPawn));

	return IsValid(PC->GetPawn());
}

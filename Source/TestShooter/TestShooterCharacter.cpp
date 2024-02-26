// Copyright Epic Games, Inc. All Rights Reserved.

#include "TestShooterCharacter.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "Net/UnrealNetwork.h"
#include "Kismet/GameplayStatics.h"
#include "TestShooterGameMode.h"

DEFINE_LOG_CATEGORY(LogTemplateCharacter);

//////////////////////////////////////////////////////////////////////////
// ATestShooterCharacter

ATestShooterCharacter::ATestShooterCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
		
	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f); // ...at this rotation rate

	// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
	// instead of recompiling to adjust them
	GetCharacterMovement()->JumpZVelocity = 700.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f; // The camera follows at this distance behind the character	
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)
}

void ATestShooterCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ATestShooterCharacter, bJumping);

	DOREPLIFETIME(ATestShooterCharacter, bDead);
	DOREPLIFETIME(ATestShooterCharacter, PlayerName);
	DOREPLIFETIME(ATestShooterCharacter, Ammo);
	DOREPLIFETIME(ATestShooterCharacter, HP);
	DOREPLIFETIME(ATestShooterCharacter, TimeBeforeRespawn);
}

void ATestShooterCharacter::BeginPlay()
{
	// Call the base class  
	Super::BeginPlay();

	//Add Input Mapping Context
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}

	MaxAmmo = 10;
	Ammo = MaxAmmo;
	HP = 100;
	bCanShoot = true;
	bCanReloadWeapon = true;

	OnTakeAnyDamage.AddDynamic(this, &ATestShooterCharacter::OnTakeAnyDamageHandler);
}

void ATestShooterCharacter::OnTakeAnyDamageHandler(AActor* DamagedActor, float Damage, const class UDamageType* DamageType, class AController* InstigatedBy, AActor* DamageCauser)
{
	HP = FMath::Max(HP - (int32)Damage, 0);

	if (HP <= 0)
	{
		bDead = true;
		TimeBeforeRespawn = 3.0f;
		GetWorld()->GetTimerManager().SetTimer(RespawnTimerHandle, this, &ATestShooterCharacter::OnRespawnTimerEnd, TimeBeforeRespawn, false);
		Multicast_Death();
	}
}

void ATestShooterCharacter::Multicast_Death_Implementation()
{
	Client_Death();
}

void ATestShooterCharacter::Client_Death_Implementation()
{

}

void ATestShooterCharacter::Tick(float DeltaSeconds)
{
	if (HasAuthority() && bDead && TimeBeforeRespawn > 0.0f)
	{
		TimeBeforeRespawn = FMath::Max(TimeBeforeRespawn -= DeltaSeconds, 0.0f);
	}
}

//////////////////////////////////////////////////////////////////////////
// Input

void ATestShooterCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {
		
		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ATestShooterCharacter::Move);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ATestShooterCharacter::Look);

		//Shooting
		EnhancedInputComponent->BindAction(ShootAction, ETriggerEvent::Triggered, this, &ATestShooterCharacter::Shoot);

		//Reloading
		EnhancedInputComponent->BindAction(ReloadAction, ETriggerEvent::Triggered, this, &ATestShooterCharacter::ReloadWeapon);
	}
	else
	{
		UE_LOG(LogTemplateCharacter, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}

void ATestShooterCharacter::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	
		// get right vector 
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// add movement 
		AddMovementInput(ForwardDirection, MovementVector.Y);
		AddMovementInput(RightDirection, MovementVector.X);
	}
}

void ATestShooterCharacter::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

void ATestShooterCharacter::Shoot()
{
	Server_Shoot();
}

void ATestShooterCharacter::Server_Shoot_Implementation()
{
	if (bDead || !bCanShoot)
	{
		return;
	}

	if (Ammo > 0)
	{
		--Ammo;
		Multicast_PlaySound(ShootSound);

		if (UWorld* World = GetWorld())
		{
			FHitResult HitResult;
			ECollisionChannel CollisionChannel = ECollisionChannel::ECC_Visibility;
			static const FName TraceTag(TEXT("LineTraceSingle"));
			FCollisionQueryParams Params(TraceTag, SCENE_QUERY_STAT_ONLY(KismetTraceUtils), false);
			Params.bReturnPhysicalMaterial = true;
			Params.AddIgnoredActor(this);
			FVector Start = FollowCamera->GetComponentLocation();
			FVector End = Start + FollowCamera->GetForwardVector() * 50000.0f;

			World->LineTraceSingleByChannel(HitResult, Start, End, CollisionChannel, Params);

			if (ATestShooterCharacter* HitActor = Cast<ATestShooterCharacter>(HitResult.GetActor()))
			{
				int32 Damage = FMath::RandRange(5, 10);
				UGameplayStatics::ApplyDamage(HitActor, Damage, nullptr, nullptr, nullptr);
			}

			Multicast_Shoot();
		}
	}
	else
	{
		Multicast_PlaySound(NoAmmoSound);
	}

	bCanShoot = false;

	GetWorld()->GetTimerManager().SetTimer(CanShootTimerHandle, this, &ATestShooterCharacter::OnCanShootTimerEnd, 0.2f, false);
}

void ATestShooterCharacter::Multicast_Shoot_Implementation()
{
	ShowShootEffect();
}

void ATestShooterCharacter::ShowShootEffect_Implementation()
{

}

void ATestShooterCharacter::Server_PlaySound_Implementation(USoundBase* Sound)
{
	Multicast_PlaySound(Sound);
}

void ATestShooterCharacter::Multicast_PlaySound_Implementation(USoundBase* Sound)
{
	UGameplayStatics::SpawnSoundAttached(Sound, GetCapsuleComponent(), NAME_None, FVector::ZeroVector, EAttachLocation::Type::KeepRelativeOffset, false, 1.0f, 1.0f, 0.0f, AttenuationSettings);
}

void ATestShooterCharacter::ReloadWeapon()
{
	Server_ReloadWeapon();
}

void ATestShooterCharacter::Server_ReloadWeapon_Implementation()
{
	if (bCanReloadWeapon)
	{
		Ammo = MaxAmmo;
		Multicast_PlaySound(ReloadWeaponSound);

		bCanReloadWeapon = false;

		GetWorld()->GetTimerManager().SetTimer(CanReloadWeaponTimerHandle, this, &ATestShooterCharacter::OnCanReloadWeaponTimerEnd, 1.0f, false);
	}
}

void ATestShooterCharacter::ChangePlayerName(FText NewName)
{
	Server_ChangePlayerName(NewName);
}

void ATestShooterCharacter::Server_ChangePlayerName_Implementation(const FText& NewName)
{
	PlayerName = NewName;
}

void ATestShooterCharacter::OnRespawnTimerEnd()
{
	if (UWorld* World = GetWorld())
	{
		if (ATestShooterGameMode* GameMode = Cast<ATestShooterGameMode>(World->GetAuthGameMode()))
		{
			GameMode->RespawnPlayer(GetInstigatorController());
		}
	}
}

void ATestShooterCharacter::OnCanShootTimerEnd()
{
	bCanShoot = true;
}

void ATestShooterCharacter::OnCanReloadWeaponTimerEnd()
{
	bCanReloadWeapon = true;
}

void ATestShooterCharacter::OnJumped_Implementation()
{
	bJumping = true;
}

void ATestShooterCharacter::Landed(const FHitResult& Hit)
{
	bJumping = false;
}

void ATestShooterCharacter::OnRep_bDead()
{
	OnPlayerDeadDelegate.Broadcast();
}

void ATestShooterCharacter::OnRep_PlayerName()
{
	OnPlayerNameChangedDelegate.Broadcast(PlayerName);
}

void ATestShooterCharacter::OnRep_Ammo()
{
	OnAmmoChangedDelegate.Broadcast(Ammo);
}

void ATestShooterCharacter::OnRep_HP()
{
	OnHPChangedDelegate.Broadcast(HP);
}

void ATestShooterCharacter::OnRep_TimeBeforeRespawn()
{
	OnTimeBeforeRespawnChangedDelegate.Broadcast(TimeBeforeRespawn);
}
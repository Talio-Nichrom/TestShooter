// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Engine.h"
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "TestShooterCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputMappingContext;
class UInputAction;
struct FInputActionValue;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

UCLASS(config=Game)
class ATestShooterCharacter : public ACharacter
{
	GENERATED_BODY()

	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;
	
	/** MappingContext */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputMappingContext* DefaultMappingContext;

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* JumpAction;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* MoveAction;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* LookAction;

	/** Shoot Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* ShootAction;

	/** Reload Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* ReloadAction;

	bool bCanReloadWeapon;

	bool bCanShoot;

	FTimerHandle RespawnTimerHandle;
	FTimerHandle CanShootTimerHandle;
	FTimerHandle CanReloadWeaponTimerHandle;

public:

	UPROPERTY(BlueprintReadWrite, Replicated)
	bool bJumping;

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_bDead)
	bool bDead;

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_PlayerName)
	FText PlayerName;

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_Ammo)
	int32 Ammo;

	UPROPERTY(BlueprintReadWrite)
	int32 MaxAmmo = 10;

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_HP)
	int32 HP = 100;

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_TimeBeforeRespawn)
	float TimeBeforeRespawn;

	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	USoundBase* ShootSound;

	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	USoundBase* NoAmmoSound;

	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	USoundBase* ReloadWeaponSound;

	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	USoundAttenuation* AttenuationSettings;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPlayerDeadDelegate);
	UPROPERTY(BlueprintAssignable)
	FOnPlayerDeadDelegate OnPlayerDeadDelegate;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlayerNameChangedDelegate, FText, NewPlayerName);
	UPROPERTY(BlueprintAssignable)
	FOnPlayerNameChangedDelegate OnPlayerNameChangedDelegate;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAmmoChangedDelegate, int32, NewAmmo);
	UPROPERTY(BlueprintAssignable)
	FOnAmmoChangedDelegate OnAmmoChangedDelegate;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHPChangedDelegate, int32, NewHP);
	UPROPERTY(BlueprintAssignable)
	FOnHPChangedDelegate OnHPChangedDelegate;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTimeBeforeRespawnChangedDelegate, float, NewTime);
	UPROPERTY(BlueprintAssignable)
	FOnTimeBeforeRespawnChangedDelegate OnTimeBeforeRespawnChangedDelegate;

public:
	ATestShooterCharacter();
	

protected:
	UFUNCTION()
	void OnTakeAnyDamageHandler(AActor* DamagedActor, float Damage, const class UDamageType* DamageType, class AController* InstigatedBy, AActor* DamageCauser);

	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_Death();

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	void Client_Death();

	/** Called for movement input */
	void Move(const FInputActionValue& Value);

	/** Called for looking input */
	void Look(const FInputActionValue& Value);
		
	void Shoot();

	UFUNCTION(Server, Unreliable)
	void Server_Shoot();

	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_Shoot();

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	void ShowShootEffect();

	UFUNCTION(Server, Unreliable)
	void Server_PlaySound(USoundBase* Sound);

	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_PlaySound(USoundBase* Sound);

	void ReloadWeapon();

	UFUNCTION(Server, Unreliable)
	void Server_ReloadWeapon();

	UFUNCTION(BlueprintCallable)
	void ChangePlayerName(FText NewName);

	UFUNCTION(Server, Unreliable)
	void Server_ChangePlayerName(const FText& NewName);

	void OnRespawnTimerEnd();
	void OnCanShootTimerEnd();
	void OnCanReloadWeaponTimerEnd();

protected:
	// APawn interface
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	
	// To add mapping context
	virtual void BeginPlay() override;

	virtual void Tick(float DeltaSeconds) override;
	
	virtual void OnJumped_Implementation() override;

	virtual void Landed(const FHitResult& Hit) override;

public:
	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }

	UFUNCTION()
	virtual void OnRep_bDead();

	UFUNCTION()
	virtual void OnRep_PlayerName();

	UFUNCTION()
	virtual void OnRep_Ammo();

	UFUNCTION()
	virtual void OnRep_HP();

	UFUNCTION()
	virtual void OnRep_TimeBeforeRespawn();
};


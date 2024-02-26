// Copyright Epic Games, Inc. All Rights Reserved.

#include "TestShooterGameMode.h"
#include "TestShooterCharacter.h"
#include "UObject/ConstructorHelpers.h"

ATestShooterGameMode::ATestShooterGameMode()
{
	// set default pawn class to our Blueprinted character
	//static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
	//if (PlayerPawnBPClass.Class != NULL)
	{
		//DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}

void ATestShooterGameMode::RespawnPlayer_Implementation(AController* Controller)
{

}

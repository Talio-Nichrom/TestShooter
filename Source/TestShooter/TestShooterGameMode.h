// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "TestShooterGameMode.generated.h"

UCLASS(minimalapi)
class ATestShooterGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ATestShooterGameMode();

	UFUNCTION(BlueprintNativeEvent)
	void RespawnPlayer(AController* Controller);
};




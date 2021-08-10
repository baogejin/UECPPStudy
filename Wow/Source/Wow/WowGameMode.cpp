// Copyright Epic Games, Inc. All Rights Reserved.

#include "WowGameMode.h"
#include "WowCharacter.h"
#include "UObject/ConstructorHelpers.h"

AWowGameMode::AWowGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPersonCPP/Blueprints/ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}

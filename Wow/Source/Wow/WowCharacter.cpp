// Copyright Epic Games, Inc. All Rights Reserved.

#include "WowCharacter.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"
#include "Math/UnrealMathUtility.h"
#include <Net/UnrealNetwork.h>

//////////////////////////////////////////////////////////////////////////
// AWowCharacter

AWowCharacter::AWowCharacter(const FObjectInitializer& ObjectInitializer)
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f); // ...at this rotation rate
	GetCharacterMovement()->JumpZVelocity = 600.f;
	GetCharacterMovement()->AirControl = 0.2f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 300.0f; // The camera follows at this distance behind the character	
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named MyCharacter (to avoid direct content references in C++)
	TargetCameraBoomArmLen = 320;
	bRightMouse = false;
	bLeftMouse = false;
	ForwardInput = 0;
	RightInput = 0;
}

//////////////////////////////////////////////////////////////////////////
// Input

void AWowCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// Set up gameplay key bindings
	check(PlayerInputComponent);
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

	PlayerInputComponent->BindAxis("MoveForward", this, &AWowCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AWowCharacter::MoveRight);

	// We have 2 versions of the rotation bindings to handle different kinds of devices differently
	// "turn" handles devices that provide an absolute delta, such as a mouse.
	// "turnrate" is for devices that we choose to treat as a rate of change, such as an analog joystick
	PlayerInputComponent->BindAxis("Turn", this, &AWowCharacter::OnTurn);
	PlayerInputComponent->BindAxis("LookUp", this, &AWowCharacter::OnLookUp);

	// handle touch devices
	PlayerInputComponent->BindTouch(IE_Pressed, this, &AWowCharacter::TouchStarted);
	PlayerInputComponent->BindTouch(IE_Released, this, &AWowCharacter::TouchStopped);

	// VR headset functionality
	PlayerInputComponent->BindAction("ResetVR", IE_Pressed, this, &AWowCharacter::OnResetVR);
	//camera arm len
	PlayerInputComponent->BindAxis("CameraDis", this, &AWowCharacter::UpdateCameraArmLen);
	PlayerInputComponent->BindAction("RightMouse",IE_Pressed,this,&AWowCharacter::OnRightMousePressed);
	PlayerInputComponent->BindAction("RightMouse", IE_Released, this, &AWowCharacter::OnRightMouseReleased);
	PlayerInputComponent->BindAction("LeftMouse", IE_Pressed, this, &AWowCharacter::OnLeftMousePressed);
	PlayerInputComponent->BindAction("LeftMouse", IE_Released, this, &AWowCharacter::OnLeftMouseReleased);
}


void AWowCharacter::OnResetVR()
{
	// If Wow is added to a project via 'Add Feature' in the Unreal Editor the dependency on HeadMountedDisplay in Wow.Build.cs is not automatically propagated
	// and a linker error will result.
	// You will need to either:
	//		Add "HeadMountedDisplay" to [YourProject].Build.cs PublicDependencyModuleNames in order to build successfully (appropriate if supporting VR).
	// or:
	//		Comment or delete the call to ResetOrientationAndPosition below (appropriate if not supporting VR)
	UHeadMountedDisplayFunctionLibrary::ResetOrientationAndPosition();
}

void AWowCharacter::TouchStarted(ETouchIndex::Type FingerIndex, FVector Location)
{
		Jump();
}

void AWowCharacter::TouchStopped(ETouchIndex::Type FingerIndex, FVector Location)
{
		StopJumping();
}

void AWowCharacter::MoveForward(float Value)
{
	ForwardInput = Value;
	AddPlayerMoveInput(true);
}

void AWowCharacter::MoveRight(float Value)
{
	if (Value == 0)
	{
		return;
	}
	if (bRightMouse)
	{
		RightInput = Value;
		AddPlayerMoveInput(false);
	}
	else
	{
		APlayerController* const PC = CastChecked<APlayerController>(Controller);
		if (!PC)
		{
			return;
		}
		FRotator r = GetActorRotation();
		r.Yaw += Value*PC->InputYawScale*2;
		r.Pitch = 0;
		r.Roll = 0;
		if (IsLocallyControlled())
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, "LocallyControlled");
			SetActorRotation(r);
		}
		WowActorRotate(r);
		if (!bLeftMouse)
		{
			AddControllerYawInput(Value);
		}
	}
}

void AWowCharacter::UpdateCameraArmLen(float val)
{
	TargetCameraBoomArmLen += val * 50;
	TargetCameraBoomArmLen = FMath::Clamp<float>(TargetCameraBoomArmLen, -50, 1000);
	if (!CameraBoom)
	{
		return;
	}
	if (CameraBoom->TargetArmLength != TargetCameraBoomArmLen)
	{
		CameraBoom->TargetArmLength = FMath::FInterpConstantTo(CameraBoom->TargetArmLength, TargetCameraBoomArmLen, 1, 5);
	}
}

void AWowCharacter::OnRightMousePressed()
{
	bRightMouse = true;
	APlayerController* const PC = CastChecked<APlayerController>(Controller);
	if (!PC)
	{
		return;
	}
	PC->bShowMouseCursor = false;
	PC->GetMousePosition(MouseX, MouseY);
}

void AWowCharacter::OnRightMouseReleased()
{
	bRightMouse = false;
	APlayerController* const PC = CastChecked<APlayerController>(Controller);
	if (!PC)
	{
		return;
	}
	PC->bShowMouseCursor = !bLeftMouse;
}

void AWowCharacter::OnLeftMousePressed()
{
	bLeftMouse = true;
	APlayerController* const PC = CastChecked<APlayerController>(Controller);
	if (!PC)
	{
		return;
	}
	PC->bShowMouseCursor = false;
	PC->GetMousePosition(MouseX, MouseY);
}

void AWowCharacter::OnLeftMouseReleased()
{
	bLeftMouse = false;
	APlayerController* const PC = CastChecked<APlayerController>(Controller);
	if (!PC)
	{
		return;
	}
	PC->bShowMouseCursor = !bRightMouse;
}

void AWowCharacter::OnTick(float deltaTime)
{
	if (!Controller)
	{
		return;
	}
	APlayerController* const PC = CastChecked<APlayerController>(Controller);
	if ((bRightMouse || bLeftMouse) && PC)
	{
		PC->SetMouseLocation(MouseX, MouseY);
	}
	if (bLeftMouse && bRightMouse)
	{
		ForwardInput = 1;
		AddPlayerMoveInput(true);
	}
}

void AWowCharacter::OnTurn(float val)
{
	if (val == 0)
	{
		return;
	}
	APlayerController* const PC = CastChecked<APlayerController>(Controller);
	if (!PC)
	{
		return;
	}
	
	if (bRightMouse)
	{
		AddControllerYawInput(val);
		FRotator r;
		r.Yaw = PC->GetControlRotation().Yaw;
		r.Pitch = 0;
		r.Roll = 0;
		//SetActorRotation(r);
		WowActorRotate(r);
		GetCharacterMovement()->MoveUpdatedComponent(FVector::ZeroVector, r, false);
	}
	else if (bLeftMouse)
	{
		AddControllerYawInput(val);
	}
}

void AWowCharacter::OnLookUp(float val)
{
	if (bLeftMouse || bRightMouse)
	{
		AddControllerPitchInput(val);
	}
}

void AWowCharacter::AddPlayerMoveInput(bool bForward)
{
	if (bForward)
	{
		FVector Forward = GetActorForwardVector();
		AddMovementInput(Forward, ForwardInput);
		if (ForwardInput < 0)
		{
			GetCharacterMovement()->MaxWalkSpeed = 200;
		}
		else
		{
			GetCharacterMovement()->MaxWalkSpeed = 600;
		}
	}
	else
	{
		FVector Right = GetActorRightVector();
		AddMovementInput(Right, RightInput);
	}
}

void AWowCharacter::WowActorRotate_Implementation(const FRotator& NewRotation)
{
	CurrentRotation = NewRotation;
	OnCurrentRotationUpdate();
}

bool AWowCharacter::WowActorRotate_Validate(const FRotator& NewRotation)
{
	return true;
}

void AWowCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AWowCharacter,CurrentRotation);
}

void AWowCharacter::OnRep_ActorRotation()
{
	OnCurrentRotationUpdate();
}

void AWowCharacter::OnCurrentRotationUpdate()
{
	if (!IsLocallyControlled())
	{
		//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, "notLocallyControlled");
		//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Blue, *GetFName().ToString());
		SetActorRotation(CurrentRotation);
	}
}
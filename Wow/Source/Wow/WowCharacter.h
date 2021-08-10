// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "WowCharacter.generated.h"

UCLASS(config=Game)
class AWowCharacter : public ACharacter
{
	GENERATED_BODY()

	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* FollowCamera;
public:
	AWowCharacter(const FObjectInitializer& ObjectInitializer);

protected:

	/** Resets HMD orientation in VR. */
	void OnResetVR();

	/** Called for forwards/backward input */
	//void MoveForward(float Value);

	/** Called for side to side input */
	//void MoveRight(float Value);

	/** Handler for when a touch input begins. */
	void TouchStarted(ETouchIndex::Type FingerIndex, FVector Location);

	/** Handler for when a touch input stops. */
	void TouchStopped(ETouchIndex::Type FingerIndex, FVector Location);

	void UpdateCameraArmLen(float val);
	void OnTurn(float val);
	void OnLookUp(float val);

	void OnRightMousePressed();
	void OnRightMouseReleased();
	void OnLeftMousePressed();
	void OnLeftMouseReleased();
	void AddPlayerMoveInput(bool bForward);

protected:
	// APawn interface
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	// End of APawn interface

public:
	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }

	UFUNCTION(BlueprintCallable)
	void MoveRight(float Value);

	UFUNCTION(BlueprintCallable)
	void MoveForward(float Value);

	UPROPERTY()
	float TargetCameraBoomArmLen;

	UPROPERTY()
	bool bRightMouse;

	UPROPERTY()
	bool bLeftMouse;

	UPROPERTY()
	float MouseX;

	UPROPERTY()
    float MouseY;

	UPROPERTY()
	float ForwardInput;

	UPROPERTY()
	float RightInput;

	UFUNCTION(BlueprintCallable)
	void OnTick(float deltaTime);

	UFUNCTION(Server, Reliable, WithValidation)
	void WowActorRotate(const FRotator& NewRotation);
	void WowActorRotate_Implementation(const FRotator& NewRotation);
	bool WowActorRotate_Validate(const FRotator& NewRotation);

	UFUNCTION(NetMulticast, Reliable, WithValidation)
	void DoWowActorRotate(const FRotator& NewRotation);
	void DoWowActorRotate_Implementation(const FRotator& NewRotation);
	bool DoWowActorRotate_Validate(const FRotator& NewRotation);
};


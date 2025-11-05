// TurretAimingComponent.cpp
#include "TurretAimingComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Components/SceneComponent.h"
#include "GameFramework/Actor.h"

UTurretAimingComponent::UTurretAimingComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UTurretAimingComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UTurretAimingComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!Head || !TargetActor) return;

	FVector HeadLoc = Head->GetComponentLocation();
	FVector TargetLoc = TargetActor->GetActorLocation();
	TargetLoc.Z += HeightOffset;

	FVector Dir = (TargetLoc - HeadLoc).GetSafeNormal();
	FVector Up = FVector::UpVector;
	FVector Right = FVector::CrossProduct(Up, Dir).GetSafeNormal();
	Up = FVector::CrossProduct(Dir, Right).GetSafeNormal();

	FMatrix LookAtMatrix;
	LookAtMatrix.SetAxes(&Dir, &Right, &Up);

	FRotator Desired = LookAtMatrix.Rotator();
	Desired.Roll = 0.f;
	Desired.Yaw += YawOffsetDegrees;

	FRotator Current = Head->GetComponentRotation();
	FRotator NewRot = FMath::RInterpTo(Current, Desired, DeltaTime, AimSpeed);
	NewRot.Roll = 0.f;

	Head->SetWorldRotation(NewRot);

}

void UTurretAimingComponent::SetTarget(AActor* InTarget)
{
	TargetActor = InTarget;
}

void UTurretAimingComponent::SetAimSpeed(float InSpeed)
{
	AimSpeed = InSpeed;
}

void UTurretAimingComponent::SetHeightOffset(float InOffset)
{
	HeightOffset = InOffset;
}

void UTurretAimingComponent::SetHead(USceneComponent* InHead)
{
	Head = InHead;
};


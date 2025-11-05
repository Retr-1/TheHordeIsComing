// TurretAimingComponent.h
#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TurretAimingComponent.generated.h"

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class PERLINNOISEGEN_API UTurretAimingComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UTurretAimingComponent();

protected:
	virtual void BeginPlay() override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turret", meta = (AllowPrivateAccess = "true"))
	USceneComponent* Head = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turret")
	AActor* TargetActor = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turret")
	float AimSpeed = 8.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turret")
	float HeightOffset = 0.f;

	UFUNCTION(BlueprintCallable, Category = "Turret")
	void SetTarget(AActor* InTarget);

	UFUNCTION(BlueprintCallable, Category = "Turret")
	void SetAimSpeed(float InSpeed);

	UFUNCTION(BlueprintCallable, Category = "Turret")
	void SetHeightOffset(float InOffset);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turret")
	float YawOffsetDegrees = 0.f;


	UFUNCTION(BlueprintCallable, Category = "Turret")
	void SetHead(USceneComponent* InHead);
};

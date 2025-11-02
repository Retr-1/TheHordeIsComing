// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TreeActor.generated.h"

UCLASS()
class PERLINNOISEGEN_API ATreeActor : public AActor
{
    GENERATED_BODY()
public:
    ATreeActor();

    UPROPERTY(VisibleAnywhere)
    UStaticMeshComponent* MeshComp;

    // Populate with 1..3 meshes in the editor
    UPROPERTY(EditAnywhere, Category = "Tree")
    TArray<UStaticMesh*> MeshVariants;

    // Optional weights; if empty → uniform
    UPROPERTY(EditAnywhere, Category = "Tree")
    TArray<float> VariantWeights;

    // If true, pick a mesh once in construction; otherwise call PickVariant() manually
    UPROPERTY(EditAnywhere, Category = "Tree")
    bool bPickOnConstruction = true;

    UFUNCTION(BlueprintCallable, Category = "Tree")
    void PickVariant(FRandomStream& RNG);

protected:
    virtual void OnConstruction(const FTransform& Transform) override;
};

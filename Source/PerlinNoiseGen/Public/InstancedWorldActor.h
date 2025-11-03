#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "InstancedWorldActor.generated.h"

// Forward declares (keep heavy includes in the .cpp)
class UHierarchicalInstancedStaticMeshComponent;
class UStaticMesh;
class UMaterialInterface;

/** One selectable mesh variant (e.g., pine, oak) */
USTRUCT(BlueprintType)
struct FMeshVariant
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere) UStaticMesh* Mesh = nullptr;
    UPROPERTY(EditAnywhere) float Weight = 1.f;                      // weighted random
    UPROPERTY(EditAnywhere) UMaterialInterface* Material = nullptr;  // optional override
};

/** A semantic group (Trees, Rocks, Grass…), with N variants (we’ll make 1 HISM per variant) */
USTRUCT(BlueprintType)
struct FInstanceGroup
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere) FName GroupName = "Group";
    UPROPERTY(EditAnywhere) TArray<FMeshVariant> Variants;

    UPROPERTY(EditAnywhere) bool bEnableCollision = false;
    UPROPERTY(EditAnywhere) int32 InstanceStartCullDistance = 0;
    UPROPERTY(EditAnywhere) int32 InstanceEndCullDistance = 0;
};

/** Runtime row: wraps the inner array so it’s UPROPERTY-friendly */
USTRUCT()
struct FInstanceGroupRuntime
{
    GENERATED_BODY()

    // One HISM component per variant in the matching FInstanceGroup::Variants
    UPROPERTY(Transient)
    TArray<TObjectPtr<UHierarchicalInstancedStaticMeshComponent>> HISMs;
};

UCLASS()
class PERLINNOISEGEN_API AInstancedWorldActor : public AActor
{
    GENERATED_BODY()

public:
    AInstancedWorldActor();

    /** Authoring time: define groups and their variants in the editor */
    UPROPERTY(EditAnywhere, Category = "Instancer")
    TArray<FInstanceGroup> Groups;

    /** Add an instance into a group; VariantIndex < 0 => weighted random */
    UFUNCTION(BlueprintCallable, Category = "Instancer")
    int32 AddInstance(int32 GroupIndex, const FTransform& WorldXf, int32 VariantIndex = -1);

    /** Clear all instances from all groups */
    UFUNCTION(CallInEditor, BlueprintCallable, Category = "Instancer")
    void ClearAll();

    virtual void OnConstruction(const FTransform& Transform) override;

private:
    /** Runtime components (mirrors Groups by index). Replaces the nested TArray. */
    UPROPERTY(Transient)
    TArray<FInstanceGroupRuntime> Runtime;

    int32 PickVariantIndex(const FInstanceGroup& G, FRandomStream& RNG) const;
    void RebuildComponents();
};

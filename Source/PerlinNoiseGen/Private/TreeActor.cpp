#include "TreeActor.h"
#include "Components/StaticMeshComponent.h"

ATreeActor::ATreeActor()
{
    PrimaryActorTick.bCanEverTick = false;
    MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
    SetRootComponent(MeshComp);
    MeshComp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics); // or NoCollision for visuals only
}

static int32 PickWeightedIndex(const TArray<float>& Weights, int32 Count, FRandomStream& RNG)
{
    if (Weights.Num() != Count || Count == 0) return RNG.RandRange(0, Count - 1);
    float sum = 0.f; for (float w : Weights) sum += FMath::Max(0.f, w);
    if (sum <= KINDA_SMALL_NUMBER) return RNG.RandRange(0, Count - 1);
    float r = RNG.FRandRange(0.f, sum), acc = 0.f;
    for (int32 i = 0; i < Count; ++i) { acc += FMath::Max(0.f, Weights[i]); if (r <= acc) return i; }
    return Count - 1;
}

void ATreeActor::PickVariant(FRandomStream& RNG)
{
    if (MeshVariants.Num() == 0) return;
    const int32 idx = PickWeightedIndex(VariantWeights, MeshVariants.Num(), RNG);
    if (MeshVariants[idx]) MeshComp->SetStaticMesh(MeshVariants[idx]);
}

void ATreeActor::OnConstruction(const FTransform& Transform)
{
    if (bPickOnConstruction)
    {
        // deterministic-ish random per actor placement
        FRandomStream RNG(GetUniqueID());
        PickVariant(RNG);
    }
}

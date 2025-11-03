#include "InstancedWorldActor.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"

AInstancedWorldActor::AInstancedWorldActor()
{
    PrimaryActorTick.bCanEverTick = false;
    auto* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(Root);
}

void AInstancedWorldActor::OnConstruction(const FTransform&) { RebuildComponents(); }

void AInstancedWorldActor::RebuildComponents()
{
    // destroy old
    for (auto& Row : Runtime)
        for (TObjectPtr<UHierarchicalInstancedStaticMeshComponent>& C : Row.HISMs)
            if (C) C->DestroyComponent();
    Runtime.Reset();

    // build new
    Runtime.SetNum(Groups.Num());
    for (int32 gi = 0; gi < Groups.Num(); ++gi)
    {
        const FInstanceGroup& G = Groups[gi];
        auto& Row = Runtime[gi].HISMs;
        Row.Reset(G.Variants.Num());

        for (int32 vi = 0; vi < G.Variants.Num(); ++vi)
        {
            const FMeshVariant& V = G.Variants[vi];
            if (!V.Mesh) { Row.Add(nullptr); continue; }

            FName CompName = *FString::Printf(TEXT("%s_%d"), *G.GroupName.ToString(), vi);
            auto* HISM = NewObject<UHierarchicalInstancedStaticMeshComponent>(this, CompName);
            HISM->SetMobility(EComponentMobility::Movable);
            HISM->SetStaticMesh(V.Mesh);
            HISM->AttachToComponent(GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);

            if (V.Material) HISM->SetMaterial(0, V.Material);
            HISM->SetCollisionEnabled(G.bEnableCollision ? ECollisionEnabled::QueryOnly
                : ECollisionEnabled::NoCollision);

            if (G.InstanceEndCullDistance > 0)
            {
                HISM->InstanceStartCullDistance = G.InstanceStartCullDistance;
                HISM->InstanceEndCullDistance = G.InstanceEndCullDistance;
            }

            HISM->RegisterComponent();
            Row.Add(HISM);
        }
    }
}

int32 AInstancedWorldActor::AddInstance(int32 GroupIndex, const FTransform& WorldXf, int32 VariantIndex)
{
    if (!Runtime.IsValidIndex(GroupIndex)) return INDEX_NONE;

    int32 UseVariant = VariantIndex;
    if (UseVariant < 0)
    {
        FRandomStream RNG(GetTypeHash(WorldXf.GetLocation()) ^ GetTypeHash(GroupIndex));
        UseVariant = PickVariantIndex(Groups[GroupIndex], RNG);
    }

    auto& Row = Runtime[GroupIndex].HISMs;
    if (!Row.IsValidIndex(UseVariant)) return INDEX_NONE;
    UHierarchicalInstancedStaticMeshComponent* HISM = Row[UseVariant].Get();
    if (!HISM) return INDEX_NONE;

    return HISM->AddInstance(WorldXf, /*bWorldSpace=*/true);
}

int32 AInstancedWorldActor::PickVariantIndex(const FInstanceGroup& G, FRandomStream& RNG) const
{
    if (G.Variants.Num() == 0) return 0;
    float sum = 0.f; for (const auto& V : G.Variants) sum += FMath::Max(0.f, V.Weight);
    if (sum <= KINDA_SMALL_NUMBER) return RNG.RandRange(0, G.Variants.Num() - 1);

    float r = RNG.FRandRange(0.f, sum), acc = 0.f;
    for (int32 i = 0; i < G.Variants.Num(); ++i)
    {
        acc += FMath::Max(0.f, G.Variants[i].Weight);
        if (r <= acc) return i;
    }
    return G.Variants.Num() - 1;
}

void AInstancedWorldActor::ClearAll()
{
    for (auto& Row : Runtime)
        for (TObjectPtr<UHierarchicalInstancedStaticMeshComponent>& HISM : Row.HISMs)
            if (HISM) HISM->ClearInstances();
}

#include "MapManagerComponent.h"

#include "MapWidget.h"
#include "MapTextureGenerator.h"
#include "NoiseTerrainActor.h"

#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "EngineUtils.h" // TActorIterator
#include "MapIconInterface.h"

UMapManagerComponent::UMapManagerComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickInterval = 0.f;
}

void UMapManagerComponent::BeginPlay()
{
    Super::BeginPlay();



    MapGen = NewObject<UMapTextureGenerator>(this);

    EnsureTerrainFound();
    EnsureWidgetCreated();

    // Build once at start (or remove this and only build when opened)
    RebuildMap();

    // Build initial icon list
    RefreshIconActorList();
    UpdateIconWidgets();

    if (MapWidget)
    {
        MapWidget->SetVisibility(ESlateVisibility::Hidden);
    }
}

void UMapManagerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (MapWidget)
    {
        MapWidget->RemoveFromParent();
        MapWidget = nullptr;
    }

    Super::EndPlay(EndPlayReason);
}

void UMapManagerComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    // Arrow update (your existing logic)
    if (bMapVisible && MapWidget && MapGen && TerrainActor)
    {
        APlayerController* PC = Cast<APlayerController>(GetOwner());
        if (PC)
        {
            APawn* P = PC->GetPawn();
            if (P)
            {
                float U, V;
                if (MapGen->WorldToMapUV(TerrainActor, P->GetActorLocation().X, P->GetActorLocation().Y, U, V))
                {
                    MapWidget->UpdatePlayerArrowTransform(P->GetActorRotation().Yaw, U, V);
                }
            }
        }
    }

    // Icon update throttled
    if (bUpdateIconsOnlyWhenMapVisible && !bMapVisible)
    {
        return;
    }

    IconRefreshAccumulator += DeltaTime;
    if (IconRefreshAccumulator >= IconRefreshInterval)
    {
        IconRefreshAccumulator = 0.f;

        // Refresh list sometimes (handles spawned/destroyed actors cleanly)
        RefreshIconActorList();
        UpdateIconWidgets();
    }
}

void UMapManagerComponent::RefreshIconActorList()
{
    UWorld* World = GetWorld();
    if (!World) return;

    IconActors.Reset();

    for (TActorIterator<AActor> It(World); It; ++It)
    {
        AActor* A = *It;
        if (!IsValid(A)) continue;

        if (A->GetClass()->ImplementsInterface(UMapIconInterface::StaticClass()))
        {
            IconActors.Add(A);
        }
    }
}

void UMapManagerComponent::UpdateIconWidgets()
{
    if (!MapWidget || !MapGen || !TerrainActor) return;

    // Remove icons for actors that are no longer valid
    // (simple approach: clear and rebuild; fine for dozens/hundreds; optimize if needed)
    MapWidget->ClearAllActorIcons();

    for (TWeakObjectPtr<AActor> WeakA : IconActors)
    {
        AActor* A = WeakA.Get();
        if (!IsValid(A)) continue;

        // Get icon texture + size from interface
        UTexture2D* IconTex = IMapIconInterface::Execute_GetMapIconTexture(A);
        if (!IconTex) continue;

        FVector2D IconSize = IMapIconInterface::Execute_GetMapIconSize(A);
        if (IconSize.X <= 0.f || IconSize.Y <= 0.f)
        {
            IconSize = FVector2D(16.f, 16.f);
        }

        // World location:
        FVector WorldLoc = A->GetActorLocation();

        // If your interface supplies custom location, use it when implemented
        // (If you don’t want this feature, delete these 3 lines)
        const FVector Override = IMapIconInterface::Execute_GetMapIconWorldLocation(A);
        if (!Override.IsNearlyZero()) WorldLoc = Override;

        float U, V;
        if (MapGen->WorldToMapUV(TerrainActor, WorldLoc.X, WorldLoc.Y, U, V))
        {
            MapWidget->EnsureActorIcon(A, IconTex, IconSize);
            MapWidget->SetActorIconPosition(A, U, V);
        }
    }
}

void UMapManagerComponent::EnsureTerrainFound()
{
    if (TerrainActor) return;

    UWorld* World = GetWorld();
    if (!World) return;

    TerrainActor = Cast<ANoiseTerrainActor>(
        UGameplayStatics::GetActorOfClass(World, ANoiseTerrainActor::StaticClass())
    );
}

void UMapManagerComponent::EnsureWidgetCreated()
{
    if (MapWidget || !MapWidgetClass) return;

    APlayerController* PC = Cast<APlayerController>(GetOwner());
    if (!PC) return;

    MapWidget = CreateWidget<UMapWidget>(PC, MapWidgetClass);
    if (MapWidget)
    {
        MapWidget->AddToViewport(100);
        MapWidget->SetArrowYawOffset(ArrowYawOffsetDegrees);
    }
}


void UMapManagerComponent::RebuildMap()
{
    EnsureTerrainFound();
    EnsureWidgetCreated();

    if (!MapGen || !TerrainActor || !MapWidget) return;

    UTexture2D* Tex = MapGen->GenerateMapTexture(TerrainActor, MapSettings);
    if (Tex)
    {
        
        MapWidget->SetMapTexture(Tex);
    }

    // Whenever map is rebuilt, refresh icon list and update once
    RefreshIconActorList();
    UpdateIconWidgets();
}

void UMapManagerComponent::ShowMap()
{
    EnsureWidgetCreated();
    if (!MapWidget) return;

    // Rebuild on open so it matches terrain regen at runtime
    RebuildMap();

    MapWidget->SetVisibility(ESlateVisibility::Visible);
    bMapVisible = true;

    // Update immediately on open
    RefreshIconActorList();
    UpdateIconWidgets();
}

void UMapManagerComponent::HideMap()
{
    if (!MapWidget) return;

    MapWidget->SetVisibility(ESlateVisibility::Hidden);
    bMapVisible = false;

}

void UMapManagerComponent::ToggleMap()
{
    UE_LOG(LogTemp, Warning, TEXT("Hello World"));
    if (bMapVisible) HideMap();
    else ShowMap();
}

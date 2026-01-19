#include "MapManagerComponent.h"

#include "MapWidget.h"
#include "MapTextureGenerator.h"
#include "NoiseTerrainActor.h"

#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"

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

    if (!bMapVisible || !MapWidget || !MapGen || !TerrainActor) return;

    APlayerController* PC = Cast<APlayerController>(GetOwner());
    if (!PC) return;

    APawn* P = PC->GetPawn();
    if (!P) return;

    const float Yaw = P->GetActorRotation().Yaw;

    float U, V;
    if (MapGen && TerrainActor && PC && P &&
        MapGen->WorldToMapUV(TerrainActor, P->GetActorLocation().X, P->GetActorLocation().Y, U, V))
    {
        MapWidget->UpdatePlayerArrowTransform(P->GetActorRotation().Yaw, U, V);
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

void UMapManagerComponent::ApplyInputMode(bool bShow)
{
    APlayerController* PC = Cast<APlayerController>(GetOwner());
    if (!PC) return;

    if (bShow)
    {
        PC->bShowMouseCursor = false;

        if (bUIOnlyWhileMapOpen && MapWidget)
        {
            FInputModeGameAndUI Mode;
            Mode.SetWidgetToFocus(MapWidget->TakeWidget());
            PC->SetInputMode(Mode);
        }
    }
    else
    {
        PC->bShowMouseCursor = false;

        if (bUIOnlyWhileMapOpen)
        {
            FInputModeGameOnly Mode;
            PC->SetInputMode(Mode);
        }
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
}

void UMapManagerComponent::ShowMap()
{
    EnsureWidgetCreated();
    if (!MapWidget) return;

    // Rebuild on open so it matches terrain regen at runtime
    RebuildMap();

    MapWidget->SetVisibility(ESlateVisibility::Visible);
    bMapVisible = true;

    ApplyInputMode(true);
}

void UMapManagerComponent::HideMap()
{
    if (!MapWidget) return;

    MapWidget->SetVisibility(ESlateVisibility::Hidden);
    bMapVisible = false;

    ApplyInputMode(false);
}

void UMapManagerComponent::ToggleMap()
{
    UE_LOG(LogTemp, Warning, TEXT("Hello World"));
    if (bMapVisible) HideMap();
    else ShowMap();
}

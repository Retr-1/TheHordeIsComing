#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MapTextureGenerator.h"
#include "MapIconInterface.h"
#include "MapManagerComponent.generated.h"

class UMapWidget;
class UMapTextureGenerator;
class ANoiseTerrainActor;

UCLASS(ClassGroup = (UI), meta = (BlueprintSpawnableComponent))
class PERLINNOISEGEN_API UMapManagerComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UMapManagerComponent();

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    UFUNCTION(BlueprintCallable, Category = "Map")
    void ToggleMap();

    UFUNCTION(BlueprintCallable, Category = "Map")
    void ShowMap();

    UFUNCTION(BlueprintCallable, Category = "Map")
    void HideMap();

    UFUNCTION(BlueprintCallable, Category = "Map")
    bool IsMapVisible() const { return bMapVisible; }

    UFUNCTION(BlueprintCallable, Category = "Map")
    void RebuildMap();

    UPROPERTY(EditAnywhere, Category = "Map|Icons")
    float IconRefreshInterval = 0.5f;

    UPROPERTY(EditAnywhere, Category = "Map|Icons")
    bool bUpdateIconsOnlyWhenMapVisible = true;

    float IconRefreshAccumulator = 0.f;

    UPROPERTY()
    TArray<TWeakObjectPtr<AActor>> IconActors;

    void RefreshIconActorList();
    void UpdateIconWidgets();

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map")
    TSubclassOf<UMapWidget> MapWidgetClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map")
    FMapGenSettings MapSettings;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map")
    float ArrowYawOffsetDegrees = 0.f;

    // Optional explicit terrain reference; if null, auto-find first in world
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map")
    ANoiseTerrainActor* TerrainActor = nullptr;

    // If true: switch to UI-only input while map open
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map|Input")
    bool bUIOnlyWhileMapOpen = true;

private:
    void EnsureWidgetCreated();
    void EnsureTerrainFound();
    void ApplyInputMode(bool bShow);

private:
    UPROPERTY()
    UMapWidget* MapWidget = nullptr;

    UPROPERTY()
    UMapTextureGenerator* MapGen = nullptr;

    bool bMapVisible = false;
};

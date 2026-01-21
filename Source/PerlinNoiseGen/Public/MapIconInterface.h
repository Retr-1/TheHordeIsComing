#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Engine/Texture2D.h"
#include "MapIconInterface.generated.h"

UINTERFACE(BlueprintType)
class PERLINNOISEGEN_API UMapIconInterface : public UInterface
{
    GENERATED_BODY()
};

class PERLINNOISEGEN_API IMapIconInterface
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Map")
    UTexture2D* GetMapIconTexture() const;
    virtual UTexture2D* GetMapIconTexture_Implementation() const { return nullptr; }

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Map")
    FVector2D GetMapIconSize() const;
    virtual FVector2D GetMapIconSize_Implementation() const { return FVector2D(16.f, 16.f); }

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Map")
    FVector GetMapIconWorldLocation() const;
    virtual FVector GetMapIconWorldLocation_Implementation() const { return FVector::ZeroVector; }

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Map")
    FLinearColor GetMapIconTint() const;

    virtual FLinearColor GetMapIconTint_Implementation() const
    {
        return FLinearColor::White; // no tint by default
    }
};

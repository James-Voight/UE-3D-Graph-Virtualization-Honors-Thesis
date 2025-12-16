#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PlacementManager.generated.h"

UCLASS(Blueprintable)
class VRDATAVIZ_API APlacementManager : public AActor
{
    GENERATED_BODY()

public:
    APlacementManager();

protected:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Placement")
    float MaxTraceDistance;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Placement")
    TEnumAsByte<ECollisionChannel> TraceChannel;
    
    UPROPERTY()
    class APreviewPlacementActor* OwnedPreview;

public:
    UFUNCTION(BlueprintCallable, Category = "Placement")
    bool TracePlacementLocation(const FVector& Start, const FVector& Direction, FVector& OutLocation, FVector& OutNormal);

    UFUNCTION(BlueprintCallable, Category = "Placement")
    class APreviewPlacementActor* EnsurePreviewActor(UWorld* World);

    UFUNCTION(BlueprintCallable, Category = "Placement")
    void UpdatePreviewTransform(APreviewPlacementActor* Preview, const FVector& Location, const FVector& Normal);
};


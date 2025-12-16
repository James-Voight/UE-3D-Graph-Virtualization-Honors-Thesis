#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "ChartAdjustLibrary.generated.h"

UCLASS()
class VRDATAVIZ_API UChartAdjustLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Data|Charts")
    static void SetActorScale(AActor* ChartActor, const FVector& NewUnitScale);

    UFUNCTION(BlueprintCallable, Category = "Data|Charts")
    static void SetActorRotation(AActor* ChartActor, const FRotator& NewRotation);

    UFUNCTION(BlueprintCallable, Category = "Data|Charts")
    static void SetAxisRanges(AActor* ChartActor, bool bUseCustom, float XMin, float XMax, float YMin, float YMax, float ZMin, float ZMax);
};


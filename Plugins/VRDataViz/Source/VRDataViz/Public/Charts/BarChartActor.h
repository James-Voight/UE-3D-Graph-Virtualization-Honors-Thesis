#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/DataTable.h"
#include "BarChartActor.generated.h"

USTRUCT(BlueprintType)
struct FVRBarData : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 XIndex = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 YIndex = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float Value = 0.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FString XLabel;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FString YLabel;
};

UCLASS()
class VRDATAVIZ_API ABarChartActor : public AActor
{
    GENERATED_BODY()

public:
    ABarChartActor();

protected:
    virtual void BeginPlay() override;

private:
    UPROPERTY() USceneComponent* Root;
    UPROPERTY() TArray<FVRBarData> BarPoints;
    UPROPERTY() TArray<FVRBarData> RuntimeBarPoints;
    UPROPERTY() TArray<AActor*> SpawnedChildren;

    void LoadBarData();
    void GenerateBars();
    void ClearChildrenActors();

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data") UDataTable* BarDataTable;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance") FVector BarSize = FVector(100.0f, 100.0f, 100.0f);
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance") FVector UnitScale = FVector(150.0f, 150.0f, 1.0f);
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance") FRotator AdditionalRotation = FRotator::ZeroRotator;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Axes") bool bUseCustomRange = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Axes", meta=(EditCondition="bUseCustomRange")) float ZMin = 0.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Axes", meta=(EditCondition="bUseCustomRange")) float ZMax = 1.0f;
    UFUNCTION(BlueprintCallable, Category = "Chart") void Rebuild();
    void SetRuntimeData(const TArray<FVRBarData>& InData) { RuntimeBarPoints = InData; }
};


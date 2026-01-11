#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/DataTable.h"
#include "ScatterActor.generated.h"

USTRUCT(BlueprintType)
struct FVRScatterData : public FTableRowBase
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float X = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float Y = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float Z = 0;
};

UCLASS()
class VRDATAVIZ_API AScatterActor : public AActor
{
    GENERATED_BODY()

public:
    AScatterActor();

protected:
    virtual void BeginPlay() override;

private:
    UPROPERTY() USceneComponent* Root;
    UPROPERTY() TArray<FVector> DataPoints;
    UPROPERTY() float AxisMinX = 0.0f;
    UPROPERTY() float AxisMaxX = 0.0f;
    UPROPERTY() float AxisMinY = 0.0f;
    UPROPERTY() float AxisMaxY = 0.0f;
    UPROPERTY() float AxisMinZ = 0.0f;
    UPROPERTY() float AxisMaxZ = 0.0f;
    UPROPERTY() float AxisStepX = 1.0f;
    UPROPERTY() float AxisStepY = 1.0f;
    UPROPERTY() float AxisStepZ = 1.0f;

    void LoadSampleData();
    void GenerateScatterplot();
    void GenerateGridlinesAndLabels();

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data") UDataTable* ScatterDataTable;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Axes") bool bUseCustomRange = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Axes", meta=(EditCondition="bUseCustomRange")) float XMin = 0.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Axes", meta=(EditCondition="bUseCustomRange")) float XMax = 1.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Axes", meta=(EditCondition="bUseCustomRange")) float YMin = 0.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Axes", meta=(EditCondition="bUseCustomRange")) float YMax = 1.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Axes", meta=(EditCondition="bUseCustomRange")) float ZMin = 0.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Axes", meta=(EditCondition="bUseCustomRange")) float ZMax = 1.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance") FVector GraphScale = FVector(100.f, 100.f, 100.f);
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance") float PointScale = 0.5f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance") float TextScale = 1.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance") FRotator AdditionalRotation = FRotator::ZeroRotator;
    UFUNCTION(BlueprintCallable, Category = "Chart") void Rebuild();
    
    // Track spawned child actors for cleanup
    UPROPERTY() TArray<AActor*> SpawnedChildren;
    void ClearChildrenActors();
};


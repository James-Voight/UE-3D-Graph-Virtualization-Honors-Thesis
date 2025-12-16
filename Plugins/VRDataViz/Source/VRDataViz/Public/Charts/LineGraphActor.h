#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/DataTable.h"
#include "LineGraphActor.generated.h"

USTRUCT(BlueprintType)
struct FVRLineData : public FTableRowBase
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float X = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float Y = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float Z = 0;
};

UCLASS()
class VRDATAVIZ_API ALineGraphActor : public AActor
{
    GENERATED_BODY()

public:
    ALineGraphActor();

protected:
    virtual void BeginPlay() override;

private:
    UPROPERTY() USceneComponent* Root;
    UPROPERTY() TArray<FVector> DataPoints;
    UPROPERTY() FVector GraphOrigin;
    UPROPERTY() UStaticMesh* CylinderMesh;

    // Computed from data
    float MinX, MaxX, MinY, MaxY, MinZ, MaxZ;

    void LoadData();
    void GeneratePoints();
    void GenerateLines();
    void GenerateAxes();
    void GenerateGridlines();
    void CreateLineSegmentCylinder(const FVector& Start, const FVector& End, const FLinearColor& Color);
    FVector MapDataToWorld(const FVector& In) const;

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data") UDataTable* LineDataTable;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Axes") bool bUseCustomRange = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Axes", meta=(EditCondition="bUseCustomRange")) float XMin = 0.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Axes", meta=(EditCondition="bUseCustomRange")) float XMax = 1.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Axes", meta=(EditCondition="bUseCustomRange")) float YMin = 0.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Axes", meta=(EditCondition="bUseCustomRange")) float YMax = 1.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Axes", meta=(EditCondition="bUseCustomRange")) float ZMin = 0.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Axes", meta=(EditCondition="bUseCustomRange")) float ZMax = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance") FVector GraphSize = FVector(100.0f, 100.0f, 100.0f);
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance") FRotator AdditionalRotation = FRotator::ZeroRotator;

    UFUNCTION(BlueprintCallable, Category = "Chart") void Rebuild();
    
    // Track spawned child actors for cleanup
    UPROPERTY() TArray<AActor*> SpawnedChildren;
    void ClearChildrenActors();
};


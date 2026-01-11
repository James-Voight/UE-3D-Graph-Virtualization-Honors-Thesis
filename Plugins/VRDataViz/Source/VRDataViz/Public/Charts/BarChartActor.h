#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/DataTable.h"
#include "BarChartActor.generated.h"

class UMaterialInterface;
class ABarActor;

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
    UPROPERTY() UMaterialInterface* BarColorMaterial;

    void LoadBarData();
    void GenerateBars();
    void GenerateZAxisTicks(float MaxValue, const FVector& AxisOrigin, const FQuat& GraphRotation, float ScaledBarWidth, float ScaledBarDepth);
    void ClearChildrenActors();
    FLinearColor GetBarColor(int32 BarIndex);

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data")
    UDataTable* BarDataTable;

    // Overall scale multiplier (X scales width/spacing, Y scales depth/spacing, Z scales height)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scale")
    FVector GraphScale = FVector(1.0f, 1.0f, 1.0f);

    // Base grid spacing (distance between bar centers) - scaled by GraphScale.X/Y
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layout")
    float CellSizeX = 150.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layout")
    float CellSizeY = 150.0f;

    // Base bar dimensions - scaled by GraphScale.X/Y
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bar Size")
    float BarWidth = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bar Size")
    float BarDepth = 100.0f;

    // Height multiplier: bar height = Value * HeightScale * GraphScale.Z
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bar Size")
    float HeightScale = 100.0f;

    // Label appearance
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Labels")
    float TextWorldSize = 50.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Labels")
    float TextScale = 1.0f;

    // Show value labels on top of each bar
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Labels")
    bool bShowValueLabels = true;

    // Use unique colors for each bar (deterministic - won't change on rebuild)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
    bool bUniqueBarColors = true;

    // Bar color (used when bUniqueBarColors is false)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance", meta=(EditCondition="!bUniqueBarColors"))
    FLinearColor BarColor = FLinearColor::Blue;

    // Random seed for bar colors (change this to get different color palettes)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance", meta=(EditCondition="bUniqueBarColors"))
    int32 ColorSeed = 12345;

    // Optional rotation for entire chart
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
    FRotator AdditionalRotation = FRotator::ZeroRotator;

    // Z-axis range override
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Z Axis")
    bool bUseCustomRange = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Z Axis", meta=(EditCondition="bUseCustomRange"))
    float ZMin = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Z Axis", meta=(EditCondition="bUseCustomRange"))
    float ZMax = 100.0f;

    // Number of desired tick marks on Z axis
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Z Axis")
    int32 ZTickCount = 5;

    UFUNCTION(BlueprintCallable, Category = "Chart")
    void Rebuild();

    void SetRuntimeData(const TArray<FVRBarData>& InData) { RuntimeBarPoints = InData; }
};

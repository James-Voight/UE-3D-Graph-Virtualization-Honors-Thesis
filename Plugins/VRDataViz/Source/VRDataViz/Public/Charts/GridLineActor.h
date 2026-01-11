#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GridLineActor.generated.h"

class UMaterialInterface;
class AStaticMeshActor;

UCLASS()
class VRDATAVIZ_API AGridLineActor : public AActor
{
    GENERATED_BODY()

public:
    AGridLineActor();

    UFUNCTION(BlueprintCallable, Category = "Chart")
    void InitializeLine(const FVector& InStart, const FVector& InEnd, const FLinearColor& InColor = FLinearColor::Black, float InThickness = 1.5f);
    
    UFUNCTION(BlueprintCallable, Category = "Chart")
    void InitializePlane(const FVector& Center, const FVector& Size, const FRotator& Rotation, const FLinearColor& InColor = FLinearColor::Black, float InThickness = 0.1f);

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    void SpawnLineGeometry();
    void SpawnPlaneGeometry(const FVector& Center, const FVector& Size, const FRotator& Rotation);

    UPROPERTY()
    UStaticMesh* CubeMesh;
    UPROPERTY()
    UStaticMesh* PlaneMesh;
    UPROPERTY()
    UMaterialInterface* ColorMaterial;
    UPROPERTY()
    TArray<AStaticMeshActor*> SpawnedMeshes;

    FVector Start;
    FVector End;
    FLinearColor Color;
    float Thickness;
};

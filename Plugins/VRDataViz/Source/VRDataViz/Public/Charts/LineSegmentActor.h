#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LineSegmentActor.generated.h"

class AStaticMeshActor;

UCLASS()
class VRDATAVIZ_API ALineSegmentActor : public AActor
{
    GENERATED_BODY()

public:
    ALineSegmentActor();

    UFUNCTION(BlueprintCallable, Category = "Chart")
    void InitializeSegment(const FVector& InStart, const FVector& InEnd, const FLinearColor& InColor = FLinearColor::Red, float InThickness = 2.0f);

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    void SpawnCylinderGeometry();

    UPROPERTY()
    UStaticMesh* CylinderMesh;
    UPROPERTY()
    AStaticMeshActor* SegmentMeshActor = nullptr;

    FVector Start;
    FVector End;
    FLinearColor Color;
    float Thickness;
};


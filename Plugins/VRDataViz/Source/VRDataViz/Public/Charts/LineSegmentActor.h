#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LineSegmentActor.generated.h"

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

private:
    void SpawnCylinderGeometry();

    UPROPERTY()
    UStaticMesh* CylinderMesh;

    FVector Start;
    FVector End;
    FLinearColor Color;
    float Thickness;
};


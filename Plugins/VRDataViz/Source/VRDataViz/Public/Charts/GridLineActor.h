#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GridLineActor.generated.h"

UCLASS()
class VRDATAVIZ_API AGridLineActor : public AActor
{
    GENERATED_BODY()

public:
    AGridLineActor();

    UFUNCTION(BlueprintCallable, Category = "Chart")
    void InitializeLine(const FVector& InStart, const FVector& InEnd, const FLinearColor& InColor = FLinearColor::Gray, float InThickness = 1.5f);

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


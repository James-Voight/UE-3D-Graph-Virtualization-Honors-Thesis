#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BarActor.generated.h"

UCLASS()
class VRDATAVIZ_API ABarActor : public AActor
{
    GENERATED_BODY()

public:
    ABarActor();

    // Initialize bar with world position, rotation, and dimensions in world units
    // Width/Depth/Height are in world units (e.g., 100 = 100 unreal units)
    void InitializeBar(
        const FVector& WorldLocation,
        const FQuat& WorldRotation,
        float Width,
        float Depth,
        float Height,
        UMaterialInterface* BaseMaterial,
        const FLinearColor& Color
    );

private:
    UPROPERTY()
    USceneComponent* Root;

    UPROPERTY()
    UStaticMeshComponent* MeshComp;

    UPROPERTY()
    UStaticMesh* CubeMesh;
};

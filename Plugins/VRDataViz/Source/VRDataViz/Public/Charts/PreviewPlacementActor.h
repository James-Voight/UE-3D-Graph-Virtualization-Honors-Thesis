#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PreviewPlacementActor.generated.h"

UCLASS()
class VRDATAVIZ_API APreviewPlacementActor : public AActor
{
    GENERATED_BODY()

public:
    APreviewPlacementActor();

    UFUNCTION(BlueprintCallable, Category = "Placement")
    void SetPreviewTransform(const FTransform& InTransform);

    UFUNCTION(BlueprintCallable, Category = "Placement")
    void SetPreviewActive(bool bActive);

protected:
    virtual void BeginPlay() override;

private:
    UPROPERTY()
    class USceneComponent* Root;

    UPROPERTY()
    class UStaticMeshComponent* Mesh;
};


#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ScatterPointActor.generated.h"

UCLASS()
class VRDATAVIZ_API AScatterPointActor : public AActor
{
    GENERATED_BODY()

public:
    AScatterPointActor();

    UFUNCTION(BlueprintCallable, Category = "Chart")
    void InitializePoint(const FVector& InWorldLocation, const FLinearColor& InColor = FLinearColor::White, float InScale = 1.0f);

protected:
    virtual void BeginPlay() override;

private:
    UPROPERTY()
    class USceneComponent* Root;
    UPROPERTY()
    class UStaticMeshComponent* Mesh;
};


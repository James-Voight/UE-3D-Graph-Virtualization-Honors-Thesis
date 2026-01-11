#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AxisTickActor.generated.h"

UCLASS()
class VRDATAVIZ_API AAxisTickActor : public AActor
{
    GENERATED_BODY()

public:
    AAxisTickActor();

    UFUNCTION(BlueprintCallable, Category = "Chart")
    void InitializeTick(const FVector& InWorldLocation, const FString& InLabel, const FRotator& InTextRotation = FRotator::ZeroRotator);
    UFUNCTION(BlueprintCallable, Category = "Chart")
    void SetFaceCamera(bool bInFaceCamera) { bFaceCamera = bInFaceCamera; }

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;

private:
    UPROPERTY()
    class USceneComponent* Root;

    UPROPERTY()
    class UTextRenderComponent* Text;
    UPROPERTY()
    bool bFaceCamera = true;
};


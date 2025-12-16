#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WorldUIPanelActor.generated.h"

class UWidgetComponent;

UCLASS(Blueprintable)
class VRDATAVIZ_API AWorldUIPanelActor : public AActor
{
    GENERATED_BODY()

public:
    AWorldUIPanelActor();

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
    UWidgetComponent* WidgetComponent;

    virtual void BeginPlay() override;

public:
    UFUNCTION(BlueprintCallable, Category = "UI")
    void SetWidgetClass(TSubclassOf<UUserWidget> InWidgetClass);

    UFUNCTION(BlueprintCallable, Category = "UI")
    UWidgetComponent* GetWidgetComponent() const { return WidgetComponent; }
};


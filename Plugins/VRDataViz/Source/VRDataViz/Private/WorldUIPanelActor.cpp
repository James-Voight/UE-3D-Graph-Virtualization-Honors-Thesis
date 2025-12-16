#include "WorldUIPanelActor.h"
#include "Components/WidgetComponent.h"
#include "Blueprint/UserWidget.h"
#include "Engine/Engine.h"

AWorldUIPanelActor::AWorldUIPanelActor()
{
    PrimaryActorTick.bCanEverTick = false;

    WidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("WidgetComponent"));
    SetRootComponent(WidgetComponent);

    WidgetComponent->SetWidgetSpace(EWidgetSpace::World);
    WidgetComponent->SetDrawSize(FVector2D(1200.0f, 900.0f)); // Increased size to fit all buttons
    WidgetComponent->SetPivot(FVector2D(0.5f, 0.5f));
    WidgetComponent->SetTwoSided(true);
    WidgetComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    WidgetComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
    WidgetComponent->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
    WidgetComponent->SetGenerateOverlapEvents(false);
}

void AWorldUIPanelActor::SetWidgetClass(TSubclassOf<UUserWidget> InWidgetClass)
{
    if (WidgetComponent)
    {
        WidgetComponent->SetWidgetClass(InWidgetClass);
    }
}

void AWorldUIPanelActor::BeginPlay()
{
    Super::BeginPlay();

    if (WidgetComponent)
    {
        // Make sure collision is correct for mouse hit testing
        WidgetComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
        WidgetComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
        WidgetComponent->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
        WidgetComponent->SetGenerateOverlapEvents(false);

        // If your version exposes this, it's the "Receive Hardware Input" flag
        // (If it doesn't compile, we'll flip it in the Blueprint instead.)
        WidgetComponent->SetWindowFocusable(true);

        UE_LOG(LogTemp, Log, TEXT("WorldUIPanelActor::BeginPlay - WidgetComponent ready. Widget: %s"),
            WidgetComponent->GetWidget() ? *WidgetComponent->GetWidget()->GetName() : TEXT("None"));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("WorldUIPanelActor::BeginPlay - WidgetComponent is NULL"));
    }
}


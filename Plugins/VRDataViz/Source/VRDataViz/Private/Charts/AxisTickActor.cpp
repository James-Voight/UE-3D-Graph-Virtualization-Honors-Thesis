#include "Charts/AxisTickActor.h"
#include "Components/TextRenderComponent.h"

AAxisTickActor::AAxisTickActor()
{
    PrimaryActorTick.bCanEverTick = false;
    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    RootComponent = Root;

    Text = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Text"));
    Text->SetupAttachment(RootComponent);
    Text->SetWorldSize(80.f);
    Text->SetHorizontalAlignment(EHTA_Center);
}

void AAxisTickActor::InitializeTick(const FVector& InWorldLocation, const FString& InLabel, const FRotator& InTextRotation)
{
    SetActorLocation(InWorldLocation);
    SetActorRotation(InTextRotation);
    if (Text)
    {
        Text->SetText(FText::FromString(InLabel));
    }

    // Prevent text from scaling with the parent chart by setting inverse scale
    AActor* ParentActor = GetAttachParentActor();
    if (ParentActor)
    {
        FVector ParentScale = ParentActor->GetActorScale3D();
        FVector InverseScale = FVector(1.0f / ParentScale.X, 1.0f / ParentScale.Y, 1.0f / ParentScale.Z);
        SetActorScale3D(InverseScale);
    }
}

void AAxisTickActor::BeginPlay()
{
    Super::BeginPlay();
}


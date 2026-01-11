#include "Charts/AxisTickActor.h"
#include "Components/TextRenderComponent.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"

AAxisTickActor::AAxisTickActor()
{
    PrimaryActorTick.bCanEverTick = true;
    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    RootComponent = Root;

    Text = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Text"));
    Text->SetupAttachment(RootComponent);
    Text->SetWorldSize(20.f);
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

    // Keep text uniformly scaled relative to the parent chart
    AActor* ParentActor = GetAttachParentActor();
    if (ParentActor)
    {
        FVector ParentScale = ParentActor->GetActorScale3D();
        const float Uniform = FMath::Max3(ParentScale.X, ParentScale.Y, ParentScale.Z);
        FVector UniformScale = FVector(Uniform / ParentScale.X, Uniform / ParentScale.Y, Uniform / ParentScale.Z);
        SetActorScale3D(UniformScale);
    }
}

void AAxisTickActor::BeginPlay()
{
    Super::BeginPlay();
}

void AAxisTickActor::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (!bFaceCamera)
    {
        return;
    }

    if (UWorld* World = GetWorld())
    {
        if (APlayerController* PC = World->GetFirstPlayerController())
        {
            if (PC->PlayerCameraManager)
            {
                const FVector CamLoc = PC->PlayerCameraManager->GetCameraLocation();
                const FVector ToCam = (CamLoc - GetActorLocation()).GetSafeNormal();
                SetActorRotation(ToCam.Rotation());
            }
        }
    }
}


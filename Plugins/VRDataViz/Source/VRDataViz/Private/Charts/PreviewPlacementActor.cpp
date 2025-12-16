#include "Charts/PreviewPlacementActor.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"

APreviewPlacementActor::APreviewPlacementActor()
{
    PrimaryActorTick.bCanEverTick = false;
    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    RootComponent = Root;

    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
    Mesh->SetupAttachment(RootComponent);
    Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Mesh->SetRenderCustomDepth(true);
    Mesh->SetCustomDepthStencilValue(1);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> MeshAsset(TEXT("/Engine/BasicShapes/Cube"));
    if (MeshAsset.Succeeded())
    {
        Mesh->SetStaticMesh(MeshAsset.Object);
        Mesh->SetWorldScale3D(FVector(0.5f));
    }
}

void APreviewPlacementActor::SetPreviewTransform(const FTransform& InTransform)
{
    SetActorTransform(InTransform);
}

void APreviewPlacementActor::SetPreviewActive(bool bActive)
{
    SetActorHiddenInGame(!bActive);
}

void APreviewPlacementActor::BeginPlay()
{
    Super::BeginPlay();
}



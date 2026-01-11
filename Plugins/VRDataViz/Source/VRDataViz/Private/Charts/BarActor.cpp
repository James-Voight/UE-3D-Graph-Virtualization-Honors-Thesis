#include "Charts/BarActor.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

ABarActor::ABarActor()
{
    PrimaryActorTick.bCanEverTick = false;

    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    RootComponent = Root;

    MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
    MeshComp->SetupAttachment(RootComponent);
    MeshComp->SetMobility(EComponentMobility::Movable);
    MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    // Load cube mesh in constructor (safe for ConstructorHelpers)
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeAsset(TEXT("/Engine/BasicShapes/Cube"));
    if (CubeAsset.Succeeded())
    {
        CubeMesh = CubeAsset.Object;
        MeshComp->SetStaticMesh(CubeMesh);
    }
}

void ABarActor::InitializeBar(
    const FVector& WorldLocation,
    const FQuat& WorldRotation,
    float Width,
    float Depth,
    float Height,
    UMaterialInterface* BaseMaterial,
    const FLinearColor& Color)
{
    SetActorLocation(WorldLocation);
    SetActorRotation(WorldRotation);

    if (!MeshComp || !CubeMesh) return;

    MeshComp->SetStaticMesh(CubeMesh);

    // UE5 default cube is 100x100x100 units (from -50 to +50 on each axis)
    // Scale factor = desired size / 100
    const float ScaleX = FMath::Max(Width, 1.0f) / 100.0f;
    const float ScaleY = FMath::Max(Depth, 1.0f) / 100.0f;
    const float ScaleZ = FMath::Max(Height, 1.0f) / 100.0f;

    MeshComp->SetWorldScale3D(FVector(ScaleX, ScaleY, ScaleZ));

    // Apply material and color
    UMaterialInterface* MatToUse = BaseMaterial;
    if (!MatToUse)
    {
        MatToUse = MeshComp->GetMaterial(0);
    }
    if (!MatToUse)
    {
        MatToUse = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial"));
    }

    if (MatToUse)
    {
        MeshComp->SetMaterial(0, MatToUse);

        if (UMaterialInstanceDynamic* DynMat = MeshComp->CreateDynamicMaterialInstance(0, MatToUse))
        {
            // Try common parameter names
            DynMat->SetVectorParameterValue(TEXT("BaseColor"), Color);
            DynMat->SetVectorParameterValue(TEXT("Color"), Color);
        }
    }
}

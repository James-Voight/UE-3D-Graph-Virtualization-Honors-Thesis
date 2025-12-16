#include "Charts/GridLineActor.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

AGridLineActor::AGridLineActor()
{
    PrimaryActorTick.bCanEverTick = false;
    Start = FVector::ZeroVector;
    End = FVector::ZeroVector;
    Color = FLinearColor::Gray;
    Thickness = 1.5f;

    // Load cylinder mesh in constructor
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMeshAsset(TEXT("/Engine/BasicShapes/Cylinder"));
    if (CylinderMeshAsset.Succeeded())
    {
        CylinderMesh = CylinderMeshAsset.Object;
    }
}

void AGridLineActor::InitializeLine(const FVector& InStart, const FVector& InEnd, const FLinearColor& InColor, float InThickness)
{
    Start = InStart;
    End = InEnd;
    Color = InColor;
    Thickness = InThickness;

    // Spawn cylinder geometry immediately
    SpawnCylinderGeometry();
}

void AGridLineActor::SpawnCylinderGeometry()
{
    if (!GetWorld()) return;

    // Calculate the midpoint, direction, and length
    FVector Direction = End - Start;
    float Length = Direction.Size();
    if (Length < KINDA_SMALL_NUMBER) return;

    Direction.Normalize();
    FVector MidPoint = (Start + End) * 0.5f;

    // Calculate rotation to align cylinder along the line
    FRotator Rotation = Direction.ToOrientationRotator();

    // Spawn cylinder actor
    FActorSpawnParameters Params;
    Params.Owner = this;
    AStaticMeshActor* CylinderActor = GetWorld()->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), FTransform(Rotation, MidPoint), Params);

    if (CylinderActor && CylinderActor->GetStaticMeshComponent())
    {
        // Set mobility to Movable so it can be attached to the chart actor
        CylinderActor->GetStaticMeshComponent()->SetMobility(EComponentMobility::Movable);
        CylinderActor->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);

        // Use cylinder mesh (loaded in constructor)
        if (CylinderMesh)
        {
            CylinderActor->GetStaticMeshComponent()->SetStaticMesh(CylinderMesh);
        }

        // Scale: X/Y for thickness, Z for length
        float ScaleFactor = Thickness / 50.0f; // Base cylinder radius is 50 units
        CylinderActor->GetStaticMeshComponent()->SetWorldScale3D(FVector(ScaleFactor, ScaleFactor, Length / 200.0f)); // Base cylinder height is 200 units
        CylinderActor->GetStaticMeshComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

        // Apply color
        if (UMaterialInterface* Mat = CylinderActor->GetStaticMeshComponent()->GetMaterial(0))
        {
            UMaterialInstanceDynamic* DynMat = CylinderActor->GetStaticMeshComponent()->CreateDynamicMaterialInstance(0, Mat);
            if (DynMat)
            {
                DynMat->SetVectorParameterValue(TEXT("BaseColor"), Color);
                DynMat->SetVectorParameterValue(TEXT("Color"), Color);
                DynMat->SetScalarParameterValue(TEXT("Opacity"), 1.0f);
                DynMat->SetScalarParameterValue(TEXT("Metallic"), 0.0f);
                DynMat->SetScalarParameterValue(TEXT("Roughness"), 0.8f);
            }
        }
    }
}

void AGridLineActor::BeginPlay()
{
    Super::BeginPlay();
    // Geometry is already spawned in InitializeLine, no need for debug lines
}


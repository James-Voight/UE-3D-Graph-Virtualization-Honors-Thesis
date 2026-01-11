#include "Charts/LineSegmentActor.h"
#include "Engine/World.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"
#include "Math/RotationMatrix.h"

ALineSegmentActor::ALineSegmentActor()
{
    PrimaryActorTick.bCanEverTick = false;
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    RootComponent->SetMobility(EComponentMobility::Movable);
    Start = FVector::ZeroVector;
    End = FVector::ZeroVector;
    Color = FLinearColor::Red;
    Thickness = 2.0f;

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMeshAsset(TEXT("/Engine/BasicShapes/Cylinder"));
    if (CylinderMeshAsset.Succeeded())
    {
        CylinderMesh = CylinderMeshAsset.Object;
    }
}

void ALineSegmentActor::InitializeSegment(const FVector& InStart, const FVector& InEnd, const FLinearColor& InColor, float InThickness)
{
    Start = InStart;
    End = InEnd;
    Color = InColor;
    Thickness = InThickness;

    SpawnCylinderGeometry();
}

void ALineSegmentActor::SpawnCylinderGeometry()
{
    if (!GetWorld() || !CylinderMesh) return;

    if (SegmentMeshActor && IsValid(SegmentMeshActor))
    {
        SegmentMeshActor->Destroy();
        SegmentMeshActor = nullptr;
    }

    FVector Direction = End - Start;
    float Length = Direction.Size();
    if (Length < KINDA_SMALL_NUMBER) return;

    Direction.Normalize();

    // Midpoint of the line segment - cylinder pivot is at center
    FVector MidPoint = (Start + End) * 0.5f;

    // Align cylinder Z-axis with line direction
    FRotator Rotation = FRotationMatrix::MakeFromZ(Direction).Rotator();

    FActorSpawnParameters Params;
    Params.Owner = this;
    AStaticMeshActor* CylinderActor = GetWorld()->SpawnActor<AStaticMeshActor>(
        AStaticMeshActor::StaticClass(),
        FTransform(Rotation, MidPoint),
        Params
    );

    if (CylinderActor && CylinderActor->GetStaticMeshComponent())
    {
        SegmentMeshActor = CylinderActor;
        CylinderActor->GetStaticMeshComponent()->SetMobility(EComponentMobility::Movable);
        CylinderActor->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
        CylinderActor->GetStaticMeshComponent()->SetStaticMesh(CylinderMesh);

        // UE5 cylinder: 100 diameter (50 radius), 100 height (from -50 to +50 in Z)
        float RadiusScale = Thickness / 100.0f;
        float HeightScale = Length / 100.0f;
        CylinderActor->GetStaticMeshComponent()->SetWorldScale3D(FVector(RadiusScale, RadiusScale, HeightScale));
        CylinderActor->GetStaticMeshComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

        UMaterialInterface* BaseMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Materials/M_Color"));
        if (!BaseMat)
        {
            BaseMat = CylinderActor->GetStaticMeshComponent()->GetMaterial(0);
        }
        if (!BaseMat)
        {
            BaseMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial"));
        }
        if (BaseMat)
        {
            CylinderActor->GetStaticMeshComponent()->SetMaterial(0, BaseMat);
            UMaterialInstanceDynamic* DynMat = CylinderActor->GetStaticMeshComponent()->CreateDynamicMaterialInstance(0, BaseMat);
            if (DynMat)
            {
                DynMat->SetVectorParameterValue(TEXT("BaseColor"), Color);
                DynMat->SetVectorParameterValue(TEXT("Color"), Color);
            }
        }
    }
}

void ALineSegmentActor::BeginPlay()
{
    Super::BeginPlay();
}

void ALineSegmentActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (SegmentMeshActor && IsValid(SegmentMeshActor))
    {
        SegmentMeshActor->Destroy();
        SegmentMeshActor = nullptr;
    }

    Super::EndPlay(EndPlayReason);
}

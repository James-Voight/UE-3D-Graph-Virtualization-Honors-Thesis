#include "Charts/GridLineActor.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "Engine/StaticMeshActor.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"
#include "Math/RotationMatrix.h"

AGridLineActor::AGridLineActor()
{
    PrimaryActorTick.bCanEverTick = false;
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    RootComponent->SetMobility(EComponentMobility::Movable);
    Start = FVector::ZeroVector;
    End = FVector::ZeroVector;
    Color = FLinearColor::Black;
    Thickness = 1.5f;

    // Load cube mesh in constructor
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMeshAsset(TEXT("/Engine/BasicShapes/Cube"));
    if (CubeMeshAsset.Succeeded())
    {
        CubeMesh = CubeMeshAsset.Object;
    }
    
    // Load plane mesh for grid planes
    static ConstructorHelpers::FObjectFinder<UStaticMesh> PlaneMeshAsset(TEXT("/Engine/BasicShapes/Plane"));
    if (PlaneMeshAsset.Succeeded())
    {
        PlaneMesh = PlaneMeshAsset.Object;
    }

    static ConstructorHelpers::FObjectFinder<UMaterialInterface> ColorMatAsset(TEXT("/Game/Materials/M_Color"));
    if (ColorMatAsset.Succeeded())
    {
        ColorMaterial = ColorMatAsset.Object;
    }
}

void AGridLineActor::InitializeLine(const FVector& InStart, const FVector& InEnd, const FLinearColor& InColor, float InThickness)
{
    Start = InStart;
    End = InEnd;
    Color = InColor;
    Thickness = InThickness;

    // Spawn line geometry immediately
    SpawnLineGeometry();
}

void AGridLineActor::SpawnLineGeometry()
{
    if (!GetWorld()) return;

    // Calculate the midpoint, direction, and length
    FVector Direction = End - Start;
    float Length = Direction.Size();
    if (Length < KINDA_SMALL_NUMBER) return;

    Direction.Normalize();
    FVector MidPoint = (Start + End) * 0.5f;

    // Align cube X axis to the line direction
    FRotator Rotation = FRotationMatrix::MakeFromX(Direction).Rotator();

    // Spawn line actor
    FActorSpawnParameters Params;
    Params.Owner = this;
    AStaticMeshActor* LineActor = GetWorld()->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), FTransform(Rotation, MidPoint), Params);

    if (LineActor && LineActor->GetStaticMeshComponent())
    {
        // Set mobility to Movable so it can be attached to the chart actor
        LineActor->GetStaticMeshComponent()->SetMobility(EComponentMobility::Movable);
        LineActor->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
        SpawnedMeshes.Add(LineActor);

        // Use cube mesh (loaded in constructor)
        if (CubeMesh)
        {
            LineActor->GetStaticMeshComponent()->SetStaticMesh(CubeMesh);
        }

        // Scale: X for length, Y/Z for thickness (Cube is 100 units per side)
        const float LineThickness = FMath::Max(Thickness, 0.01f);
        LineActor->GetStaticMeshComponent()->SetWorldScale3D(FVector(Length / 100.0f, LineThickness / 100.0f, LineThickness / 100.0f));
        LineActor->GetStaticMeshComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

        UMaterialInterface* BaseMat = ColorMaterial;
        if (!BaseMat)
        {
            BaseMat = LineActor->GetStaticMeshComponent()->GetMaterial(0);
        }
        if (!BaseMat)
        {
            BaseMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial"));
        }
        if (BaseMat)
        {
            LineActor->GetStaticMeshComponent()->SetMaterial(0, BaseMat);
            UMaterialInstanceDynamic* DynMat = LineActor->GetStaticMeshComponent()->CreateDynamicMaterialInstance(0, BaseMat);
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

void AGridLineActor::InitializePlane(const FVector& Center, const FVector& Size, const FRotator& Rotation, const FLinearColor& InColor, float InThickness)
{
    Start = Center;
    End = Center + Size;
    Color = InColor;
    Thickness = InThickness;
    
    // Spawn plane geometry immediately
    SpawnPlaneGeometry(Center, Size, Rotation);
}

void AGridLineActor::SpawnPlaneGeometry(const FVector& Center, const FVector& Size, const FRotator& Rotation)
{
    if (!GetWorld() || !PlaneMesh) return;
    
    FActorSpawnParameters Params;
    Params.Owner = this;
    AStaticMeshActor* PlaneActor = GetWorld()->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), FTransform(Rotation, Center), Params);
    
    if (PlaneActor && PlaneActor->GetStaticMeshComponent())
    {
        PlaneActor->GetStaticMeshComponent()->SetMobility(EComponentMobility::Movable);
        PlaneActor->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
        PlaneActor->GetStaticMeshComponent()->SetStaticMesh(PlaneMesh);
        SpawnedMeshes.Add(PlaneActor);
        
        // Scale the plane to the desired size (Plane is 100x100 units by default)
        FVector Scale = FVector(Size.X / 100.0f, Size.Y / 100.0f, Thickness / 100.0f);
        PlaneActor->GetStaticMeshComponent()->SetWorldScale3D(Scale);
        PlaneActor->GetStaticMeshComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        
        UMaterialInterface* BaseMat = ColorMaterial;
        if (!BaseMat)
        {
            BaseMat = PlaneActor->GetStaticMeshComponent()->GetMaterial(0);
        }
        if (!BaseMat)
        {
            BaseMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial"));
        }
        if (BaseMat)
        {
            PlaneActor->GetStaticMeshComponent()->SetMaterial(0, BaseMat);
            UMaterialInstanceDynamic* DynMat = PlaneActor->GetStaticMeshComponent()->CreateDynamicMaterialInstance(0, BaseMat);
            if (DynMat)
            {
                DynMat->SetVectorParameterValue(TEXT("BaseColor"), Color);
                DynMat->SetVectorParameterValue(TEXT("Color"), Color);
                DynMat->SetScalarParameterValue(TEXT("Opacity"), 0.2f);
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

void AGridLineActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    for (AStaticMeshActor* MeshActor : SpawnedMeshes)
    {
        if (IsValid(MeshActor))
        {
            MeshActor->Destroy();
        }
    }
    SpawnedMeshes.Empty();
    Super::EndPlay(EndPlayReason);
}


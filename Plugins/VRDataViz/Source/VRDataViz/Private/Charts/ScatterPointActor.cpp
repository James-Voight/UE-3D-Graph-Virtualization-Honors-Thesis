#include "Charts/ScatterPointActor.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"

AScatterPointActor::AScatterPointActor()
{
    PrimaryActorTick.bCanEverTick = false;
    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    RootComponent = Root;

    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
    Mesh->SetupAttachment(RootComponent);
    Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> MeshAsset(TEXT("/Engine/BasicShapes/Sphere"));
    if (MeshAsset.Succeeded())
    {
        Mesh->SetStaticMesh(MeshAsset.Object);
    }
}

void AScatterPointActor::InitializePoint(const FVector& InWorldLocation, const FLinearColor& InColor, float InScale)
{
    SetActorLocation(InWorldLocation);
    Mesh->SetWorldScale3D(FVector(InScale));
    
    // Try to load the M_Color material from Content/Materials using LoadObject (runtime loading)
    UMaterialInterface* BaseMaterial = nullptr;
    
    // Try to load M_Color material
    BaseMaterial = LoadObject<UMaterial>(nullptr, TEXT("/Game/Materials/M_Color"));
    
    if (!BaseMaterial)
    {
        // Fallback to default engine material if M_Color not found
        BaseMaterial = LoadObject<UMaterial>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial"));
    }
    
    // Always create a dynamic material instance to set the color
    if (BaseMaterial)
    {
        UMaterialInstanceDynamic* DynMat = Mesh->CreateDynamicMaterialInstance(0, BaseMaterial);
        if (DynMat)
        {
            // Set color using BaseColor parameter (most common in UE materials)
            DynMat->SetVectorParameterValue(TEXT("BaseColor"), InColor);
            
            // Also try other common parameter names as fallback
            DynMat->SetVectorParameterValue(TEXT("Color"), InColor);
            DynMat->SetVectorParameterValue(TEXT("Tint"), InColor);
            DynMat->SetVectorParameterValue(TEXT("EmissiveColor"), InColor);
            
            // Ensure full opacity and reasonable material properties
            DynMat->SetScalarParameterValue(TEXT("Opacity"), 1.0f);
            DynMat->SetScalarParameterValue(TEXT("Metallic"), 0.0f);
            DynMat->SetScalarParameterValue(TEXT("Roughness"), 0.5f);
        }
    }
}

void AScatterPointActor::BeginPlay()
{
    Super::BeginPlay();
}


#include "Charts/LineGraphActor.h"
#include "Engine/World.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "Charts/LineSegmentActor.h"
#include "Charts/GridLineActor.h"
#include "Charts/AxisTickActor.h"
#include "Kismet/KismetMathLibrary.h"
#include "UObject/ConstructorHelpers.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"

ALineGraphActor::ALineGraphActor()
{
    PrimaryActorTick.bCanEverTick = false;
    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    RootComponent = Root;

    // Load cylinder mesh in constructor
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMeshAsset(TEXT("/Engine/BasicShapes/Cylinder"));
    if (CylinderMeshAsset.Succeeded())
    {
        CylinderMesh = CylinderMeshAsset.Object;
    }
}

void ALineGraphActor::BeginPlay()
{
    Super::BeginPlay();
    Rebuild();
}

void ALineGraphActor::LoadData()
{
    DataPoints.Empty();
    if (!LineDataTable) return;

    TArray<FVRLineData*> AllRows;
    LineDataTable->GetAllRows(TEXT("VR LineGraph"), AllRows);
    for (FVRLineData* RowPtr : AllRows)
    {
        if (RowPtr)
        {
            DataPoints.Add(FVector(RowPtr->X, RowPtr->Y, RowPtr->Z));
        }
    }
}

void ALineGraphActor::GeneratePoints()
{
    if (DataPoints.Num() == 0) return;

    // Use the computed min/max Z for color mapping
    const float ZRange = MaxZ - MinZ;
    
    for (const FVector& Point : DataPoints)
    {
        FVector WorldLoc = MapDataToWorld(Point);
        float OriginalZ = Point.Z;
        
        // Color mapping: Blue (low) -> Green -> Yellow -> Red (high)
        FLinearColor PointColor = FLinearColor::White;
        if (ZRange > 0.001f)
        {
            float NormalizedZ = (OriginalZ - MinZ) / ZRange;
            FLinearColor Cyan(0.0f, 1.0f, 1.0f, 1.0f); // Define Cyan color
            if (NormalizedZ < 0.33f)
            {
                PointColor = UKismetMathLibrary::LinearColorLerp(FLinearColor::Blue, Cyan, NormalizedZ / 0.33f);
            }
            else if (NormalizedZ < 0.66f)
            {
                PointColor = UKismetMathLibrary::LinearColorLerp(Cyan, FLinearColor::Green, (NormalizedZ - 0.33f) / 0.33f);
            }
            else
            {
                float T = (NormalizedZ - 0.66f) / 0.34f;
                if (T < 0.5f)
                    PointColor = UKismetMathLibrary::LinearColorLerp(FLinearColor::Green, FLinearColor::Yellow, T * 2.0f);
                else
                    PointColor = UKismetMathLibrary::LinearColorLerp(FLinearColor::Yellow, FLinearColor::Red, (T - 0.5f) * 2.0f);
            }
        }
        
        FActorSpawnParameters Params;
        Params.Owner = this;
        AStaticMeshActor* SphereActor = GetWorld()->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), FTransform(FRotator::ZeroRotator, WorldLoc), Params);
        if (SphereActor && SphereActor->GetStaticMeshComponent())
        {
            // Set mobility to Movable so it can be attached to the chart actor
            SphereActor->GetStaticMeshComponent()->SetMobility(EComponentMobility::Movable);
            SphereActor->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
            SphereActor->GetStaticMeshComponent()->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere")));
            SphereActor->GetStaticMeshComponent()->SetWorldScale3D(FVector(0.2f));
            SphereActor->GetStaticMeshComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            // Apply color to material if possible
            if (UMaterialInterface* Mat = SphereActor->GetStaticMeshComponent()->GetMaterial(0))
            {
                UMaterialInstanceDynamic* DynMat = SphereActor->GetStaticMeshComponent()->CreateDynamicMaterialInstance(0, Mat);
                if (DynMat)
                {
                    DynMat->SetVectorParameterValue(TEXT("BaseColor"), PointColor);
                    DynMat->SetVectorParameterValue(TEXT("Color"), PointColor);
                }
            }
            SpawnedChildren.Add(SphereActor);
        }
    }
}

void ALineGraphActor::GenerateLines()
{
    if (DataPoints.Num() < 2) return;

    // Use the computed min/max Z for color mapping
    const float ZRange = MaxZ - MinZ;
    
    for (int32 i = 0; i < DataPoints.Num() - 1; ++i)
    {
        FVector Start = MapDataToWorld(DataPoints[i]);
        FVector End = MapDataToWorld(DataPoints[i + 1]);
        
        // Interpolate color between two points based on average Z
        float AvgZ = (DataPoints[i].Z + DataPoints[i + 1].Z) * 0.5f;
        FLinearColor LineColor = FLinearColor::Red;
        if (ZRange > 0.001f)
        {
            float NormalizedZ = (AvgZ - MinZ) / ZRange;
            FLinearColor Cyan(0.0f, 1.0f, 1.0f, 1.0f); // Define Cyan color
            if (NormalizedZ < 0.33f)
            {
                LineColor = UKismetMathLibrary::LinearColorLerp(FLinearColor::Blue, Cyan, NormalizedZ / 0.33f);
            }
            else if (NormalizedZ < 0.66f)
            {
                LineColor = UKismetMathLibrary::LinearColorLerp(Cyan, FLinearColor::Green, (NormalizedZ - 0.33f) / 0.33f);
            }
            else
            {
                float T = (NormalizedZ - 0.66f) / 0.34f;
                if (T < 0.5f)
                    LineColor = UKismetMathLibrary::LinearColorLerp(FLinearColor::Green, FLinearColor::Yellow, T * 2.0f);
                else
                    LineColor = UKismetMathLibrary::LinearColorLerp(FLinearColor::Yellow, FLinearColor::Red, (T - 0.5f) * 2.0f);
            }
        }
        
        // Create cylinder geometry directly for the line segment
        CreateLineSegmentCylinder(Start, End, LineColor);
    }
}

FVector ALineGraphActor::MapDataToWorld(const FVector& In) const
{
    // normalize each axis to 0..1 based on Min/Max
    auto Norm = [](float V, float A, float B)
    {
        return (B != A) ? (V - A) / (B - A) : 0.0f;
    };

    float nx = Norm(In.X, MinX, MaxX);
    float ny = Norm(In.Y, MinY, MaxY);
    float nz = Norm(In.Z, MinZ, MaxZ);

    // map 0..1 to -0.5..0.5 so the graph is centered around actor
    nx -= 0.5f;
    ny -= 0.5f;
    nz -= 0.5f;

    FVector Half = GraphSize * 0.5f;

    FVector Local(nx * GraphSize.X,
                  ny * GraphSize.Y,
                  nz * GraphSize.Z);

    return GetActorLocation() + Local;
}

void ALineGraphActor::CreateLineSegmentCylinder(const FVector& Start, const FVector& End, const FLinearColor& Color)
{
    // Calculate the direction and length
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
        CylinderActor->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);

        // Use cylinder mesh (loaded in constructor)
        if (CylinderMesh)
        {
            CylinderActor->GetStaticMeshComponent()->SetStaticMesh(CylinderMesh);
        }

        // Set mobility to Movable
        CylinderActor->GetStaticMeshComponent()->SetMobility(EComponentMobility::Movable);

        // Scale: X/Y for thickness, Z for length
        float ScaleFactor = 1.5f / 50.0f; // Base cylinder radius is 50 units
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

        SpawnedChildren.Add(CylinderActor);
    }
}

void ALineGraphActor::ClearChildrenActors()
{
    for (AActor* Child : SpawnedChildren)
    {
        if (IsValid(Child))
        {
            Child->Destroy();
        }
    }
    SpawnedChildren.Empty();
}

void ALineGraphActor::GenerateAxes()
{
    FVector Origin = GetActorLocation();
    FVector Half = GraphSize * 0.5f;

    // Create the three main axis lines from origin
    FActorSpawnParameters AxisParams;
    AxisParams.Owner = this;

    // X axis: from -Half.X to +Half.X
    FVector X0 = Origin + FVector(-Half.X, 0, 0);
    FVector X1 = Origin + FVector( Half.X, 0, 0);

    if (AGridLineActor* XAxis = GetWorld()->SpawnActor<AGridLineActor>(AGridLineActor::StaticClass(), FTransform(X0), AxisParams))
    {
        XAxis->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
        XAxis->InitializeLine(X0, X1, FLinearColor::Red, 2.0f);
        SpawnedChildren.Add(XAxis);
    }

    // Y axis
    FVector Y0 = Origin + FVector(0, -Half.Y, 0);
    FVector Y1 = Origin + FVector(0,  Half.Y, 0);

    if (AGridLineActor* YAxis = GetWorld()->SpawnActor<AGridLineActor>(AGridLineActor::StaticClass(), FTransform(Y0), AxisParams))
    {
        YAxis->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
        YAxis->InitializeLine(Y0, Y1, FLinearColor::Green, 2.0f);
        SpawnedChildren.Add(YAxis);
    }

    // Z axis
    FVector Z0 = Origin + FVector(0, 0, -Half.Z);
    FVector Z1 = Origin + FVector(0, 0,  Half.Z);

    if (AGridLineActor* ZAxis = GetWorld()->SpawnActor<AGridLineActor>(AGridLineActor::StaticClass(), FTransform(Z0), AxisParams))
    {
        ZAxis->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
        ZAxis->InitializeLine(Z0, Z1, FLinearColor::Blue, 2.0f);
        SpawnedChildren.Add(ZAxis);
    }

    // Add axis labels at the ends
    FActorSpawnParameters LabelParams;
    LabelParams.Owner = this;

    // X-axis label
    if (AAxisTickActor* XLabel = GetWorld()->SpawnActor<AAxisTickActor>(AAxisTickActor::StaticClass(), FTransform(X1 + FVector(50, 0, 0)), LabelParams))
    {
        XLabel->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
        XLabel->InitializeTick(X1 + FVector(50, 0, 0), FString::Printf(TEXT("X: %.1f"), MaxX), FRotator(0, 90, 0));
        SpawnedChildren.Add(XLabel);
    }

    // Y-axis label
    if (AAxisTickActor* YLabel = GetWorld()->SpawnActor<AAxisTickActor>(AAxisTickActor::StaticClass(), FTransform(Y1 + FVector(0, 50, 0)), LabelParams))
    {
        YLabel->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
        YLabel->InitializeTick(Y1 + FVector(0, 50, 0), FString::Printf(TEXT("Y: %.1f"), MaxY), FRotator::ZeroRotator);
        SpawnedChildren.Add(YLabel);
    }

    // Z-axis label
    if (AAxisTickActor* ZLabel = GetWorld()->SpawnActor<AAxisTickActor>(AAxisTickActor::StaticClass(), FTransform(Z1 + FVector(0, 0, 50)), LabelParams))
    {
        ZLabel->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
        ZLabel->InitializeTick(Z1 + FVector(0, 0, 50), FString::Printf(TEXT("Z: %.1f"), MaxZ), FRotator(90, 0, 0));
        SpawnedChildren.Add(ZLabel);
    }
}

void ALineGraphActor::Rebuild()
{
    ClearChildrenActors();

    // Load data first
    LoadData();

    if (DataPoints.Num() == 0)
        return;

    // ----- compute min/max -----
    MinX = MaxX = DataPoints[0].X;
    MinY = MaxY = DataPoints[0].Y;
    MinZ = MaxZ = DataPoints[0].Z;

    for (const FVector& P : DataPoints)
    {
        MinX = FMath::Min(MinX, P.X); MaxX = FMath::Max(MaxX, P.X);
        MinY = FMath::Min(MinY, P.Y); MaxY = FMath::Max(MaxY, P.Y);
        MinZ = FMath::Min(MinZ, P.Z); MaxZ = FMath::Max(MaxZ, P.Z);
    }

    // If you have your bUseCustomRange / XMin, etc., you can override here:
    if (bUseCustomRange)
    {
        MinX = XMin; MaxX = XMax;
        MinY = YMin; MaxY = YMax;
        MinZ = ZMin; MaxZ = ZMax;
    }

    GraphOrigin = GetActorLocation();
    GeneratePoints();
    GenerateLines();
    GenerateAxes();
    GenerateGridlines();
}

void ALineGraphActor::GenerateGridlines()
{
    const int32 NumTicks = 4; // Number of tick marks per axis

    FActorSpawnParameters GridParams;
    GridParams.Owner = this;

    // Create tick marks along each axis at regular intervals
    for (int32 i = 0; i <= NumTicks; ++i)
    {
        float t = (NumTicks > 0) ? (float)i / (float)NumTicks : 0.f;

        // X-axis ticks (along the X axis at Y=0, Z=0)
        if (i > 0 && i < NumTicks) // Skip endpoints to avoid overlap with axis labels
        {
            // Create tick position in normalized space and map to world
            FVector TickPos(t - 0.5f, 0.0f, 0.0f); // -0.5 to 0.5 range
            FVector WorldPos = MapDataToWorld(FVector(FMath::Lerp(MinX, MaxX, t), 0.0f, 0.0f));

            // Create small tick line perpendicular to axis
            FVector TickStart = WorldPos + FVector(0, -10, 0);
            FVector TickEnd = WorldPos + FVector(0, 10, 0);

            if (AGridLineActor* Tick = GetWorld()->SpawnActor<AGridLineActor>(AGridLineActor::StaticClass(), FTransform(TickStart), GridParams))
            {
                Tick->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
                Tick->InitializeLine(TickStart, TickEnd, FLinearColor::Gray, 1.0f);
                SpawnedChildren.Add(Tick);
            }

            // Add value label
            FVector LabelPos = WorldPos + FVector(0, 25, 0);
            float DataValue = FMath::Lerp(MinX, MaxX, t);
            if (AAxisTickActor* Label = GetWorld()->SpawnActor<AAxisTickActor>(AAxisTickActor::StaticClass(), FTransform(LabelPos), GridParams))
            {
                Label->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
                Label->InitializeTick(LabelPos, FString::Printf(TEXT("%.1f"), DataValue), FRotator::ZeroRotator);
                SpawnedChildren.Add(Label);
            }
        }

        // Y-axis ticks (along the Y axis at X=0, Z=0)
        if (i > 0 && i < NumTicks)
        {
            FVector WorldPos = MapDataToWorld(FVector(0.0f, FMath::Lerp(MinY, MaxY, t), 0.0f));

            // Create small tick line perpendicular to axis
            FVector TickStart = WorldPos + FVector(-10, 0, 0);
            FVector TickEnd = WorldPos + FVector(10, 0, 0);

            if (AGridLineActor* Tick = GetWorld()->SpawnActor<AGridLineActor>(AGridLineActor::StaticClass(), FTransform(TickStart), GridParams))
            {
                Tick->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
                Tick->InitializeLine(TickStart, TickEnd, FLinearColor::Gray, 1.0f);
                SpawnedChildren.Add(Tick);
            }

            // Add value label
            FVector LabelPos = WorldPos + FVector(-35, 0, 0);
            float DataValue = FMath::Lerp(MinY, MaxY, t);
            if (AAxisTickActor* Label = GetWorld()->SpawnActor<AAxisTickActor>(AAxisTickActor::StaticClass(), FTransform(LabelPos), GridParams))
            {
                Label->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
                Label->InitializeTick(LabelPos, FString::Printf(TEXT("%.1f"), DataValue), FRotator(0, 90, 0));
                SpawnedChildren.Add(Label);
            }
        }

        // Z-axis ticks (along the Z axis at X=0, Y=0)
        if (i > 0 && i < NumTicks)
        {
            FVector WorldPos = MapDataToWorld(FVector(0.0f, 0.0f, FMath::Lerp(MinZ, MaxZ, t)));

            // Create small tick line perpendicular to axis
            FVector TickStart = WorldPos + FVector(0, -10, 0);
            FVector TickEnd = WorldPos + FVector(0, 10, 0);

            if (AGridLineActor* Tick = GetWorld()->SpawnActor<AGridLineActor>(AGridLineActor::StaticClass(), FTransform(TickStart), GridParams))
            {
                Tick->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
                Tick->InitializeLine(TickStart, TickEnd, FLinearColor::Gray, 1.0f);
                SpawnedChildren.Add(Tick);
            }

            // Add value label
            FVector LabelPos = WorldPos + FVector(0, -35, 0);
            float DataValue = FMath::Lerp(MinZ, MaxZ, t);
            if (AAxisTickActor* Label = GetWorld()->SpawnActor<AAxisTickActor>(AAxisTickActor::StaticClass(), FTransform(LabelPos), GridParams))
            {
                Label->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
                Label->InitializeTick(LabelPos, FString::Printf(TEXT("%.1f"), DataValue), FRotator(90, 0, 0));
                SpawnedChildren.Add(Label);
            }
        }
    }
}



#include "Charts/LineGraphActor.h"
#include "Engine/World.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "Charts/LineSegmentActor.h"
#include "Charts/GridLineActor.h"
#include "Charts/AxisTickActor.h"
#include "Charts/GridMath.h"
#include "Kismet/KismetMathLibrary.h"
#include "UObject/ConstructorHelpers.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace
{
    constexpr int32 TargetTicks = 6;
}

ALineGraphActor::ALineGraphActor()
{
    PrimaryActorTick.bCanEverTick = false;
    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    RootComponent = Root;

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMeshAsset(TEXT("/Engine/BasicShapes/Cylinder"));
    if (CylinderMeshAsset.Succeeded())
    {
        CylinderMesh = CylinderMeshAsset.Object;
    }

    static ConstructorHelpers::FObjectFinder<UMaterialInterface> PointMatAsset(TEXT("/Game/Materials/M_Color"));
    if (PointMatAsset.Succeeded())
    {
        PointColorMaterial = PointMatAsset.Object;
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

    const float ColorMinZ = bUseCustomRange ? ZMin : DataMinZ;
    const float ColorMaxZ = bUseCustomRange ? ZMax : DataMaxZ;
    const float ZRange = ColorMaxZ - ColorMinZ;
    
    for (const FVector& Point : DataPoints)
    {
        FVector WorldLoc = MapDataToWorld(Point);
        float OriginalZ = Point.Z;
        
        FLinearColor PointColor = FLinearColor::White;
        if (ZRange > 0.001f)
        {
            float NormalizedZ = (OriginalZ - ColorMinZ) / ZRange;
            FLinearColor Cyan(0.0f, 1.0f, 1.0f, 1.0f);
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
            SphereActor->GetStaticMeshComponent()->SetMobility(EComponentMobility::Movable);
            SphereActor->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
            SphereActor->GetStaticMeshComponent()->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere")));
            SphereActor->GetStaticMeshComponent()->SetWorldScale3D(FVector(PointScale));
            SphereActor->GetStaticMeshComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            
            UMaterialInterface* Mat = PointColorMaterial ? PointColorMaterial : SphereActor->GetStaticMeshComponent()->GetMaterial(0);
            if (!Mat)
            {
                Mat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Materials/M_Color"));
            }
            if (!Mat)
            {
                Mat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial"));
            }
            if (Mat)
            {
                SphereActor->GetStaticMeshComponent()->SetMaterial(0, Mat);
                UMaterialInstanceDynamic* DynMat = SphereActor->GetStaticMeshComponent()->CreateDynamicMaterialInstance(0, Mat);
                if (DynMat)
                {
                    DynMat->SetVectorParameterValue(TEXT("BaseColor"), PointColor);
                    DynMat->SetVectorParameterValue(TEXT("Color"), PointColor);
                    DynMat->SetVectorParameterValue(TEXT("Tint"), PointColor);
                    DynMat->SetVectorParameterValue(TEXT("EmissiveColor"), PointColor);
                }
            }
            SpawnedChildren.Add(SphereActor);
        }
    }
}

void ALineGraphActor::GenerateLines()
{
    if (DataPoints.Num() < 2) return;
    
    for (int32 i = 0; i < DataPoints.Num() - 1; ++i)
    {
        FVector Start = MapDataToWorld(DataPoints[i]);
        FVector End = MapDataToWorld(DataPoints[i + 1]);
        
        // Black lines connecting the colored data points
        CreateLineSegmentCylinder(Start, End, FLinearColor::Black);
    }
}

FVector ALineGraphActor::MapDataToWorld(const FVector& In) const
{
    const FQuat GraphRotation = GetActorQuat() * AdditionalRotation.Quaternion();
    const FVector Local = In * GraphScale;
    return GraphOrigin + GraphRotation.RotateVector(Local);
}

void ALineGraphActor::CreateLineSegmentCylinder(const FVector& Start, const FVector& End, const FLinearColor& Color)
{
    FVector Direction = End - Start;
    float Length = Direction.Size();
    if (Length < KINDA_SMALL_NUMBER) return;

    // Line thickness relative to point size
    float LineThickness = FMath::Max(2.0f, PointScale * 20.0f);

    FActorSpawnParameters Params;
    Params.Owner = this;
    if (ALineSegmentActor* Segment = GetWorld()->SpawnActor<ALineSegmentActor>(ALineSegmentActor::StaticClass(), FTransform::Identity, Params))
    {
        Segment->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
        Segment->InitializeSegment(Start, End, Color, LineThickness);
        SpawnedChildren.Add(Segment);
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
    FActorSpawnParameters AxisParams;
    AxisParams.Owner = this;
    const FQuat GraphRotation = GetActorQuat() * AdditionalRotation.Quaternion();

    FVector X0 = MapDataToWorld(FVector(AxisMinX, 0, 0));
    FVector X1 = MapDataToWorld(FVector(AxisMaxX, 0, 0));

    if (AGridLineActor* XAxis = GetWorld()->SpawnActor<AGridLineActor>(AGridLineActor::StaticClass(), FTransform(X0), AxisParams))
    {
        XAxis->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
        XAxis->InitializeLine(X0, X1, FLinearColor::Red, 2.5f);
        SpawnedChildren.Add(XAxis);
    }

    FVector Y0 = MapDataToWorld(FVector(0, AxisMinY, 0));
    FVector Y1 = MapDataToWorld(FVector(0, AxisMaxY, 0));

    if (AGridLineActor* YAxis = GetWorld()->SpawnActor<AGridLineActor>(AGridLineActor::StaticClass(), FTransform(Y0), AxisParams))
    {
        YAxis->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
        YAxis->InitializeLine(Y0, Y1, FLinearColor::Green, 2.5f);
        SpawnedChildren.Add(YAxis);
    }

    FVector Z0 = MapDataToWorld(FVector(0, 0, AxisMinZ));
    FVector Z1 = MapDataToWorld(FVector(0, 0, AxisMaxZ));

    if (AGridLineActor* ZAxis = GetWorld()->SpawnActor<AGridLineActor>(AGridLineActor::StaticClass(), FTransform(Z0), AxisParams))
    {
        ZAxis->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
        ZAxis->InitializeLine(Z0, Z1, FLinearColor::Blue, 2.5f);
        SpawnedChildren.Add(ZAxis);
    }

    FActorSpawnParameters LabelParams;
    LabelParams.Owner = this;

    FVector XLabelPos = X1 + GraphRotation.RotateVector(FVector(50, 0, 0));
    if (AAxisTickActor* XLabel = GetWorld()->SpawnActor<AAxisTickActor>(AAxisTickActor::StaticClass(), FTransform(XLabelPos), LabelParams))
    {
        XLabel->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
        XLabel->InitializeTick(XLabelPos, FString::Printf(TEXT("X: %.1f"), AxisMaxX), (GraphRotation * FRotator(0, 90, 0).Quaternion()).Rotator());
        SpawnedChildren.Add(XLabel);
    }

    FVector YLabelPos = Y1 + GraphRotation.RotateVector(FVector(0, 50, 0));
    if (AAxisTickActor* YLabel = GetWorld()->SpawnActor<AAxisTickActor>(AAxisTickActor::StaticClass(), FTransform(YLabelPos), LabelParams))
    {
        YLabel->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
        YLabel->InitializeTick(YLabelPos, FString::Printf(TEXT("Y: %.1f"), AxisMaxY), (GraphRotation * FRotator::ZeroRotator.Quaternion()).Rotator());
        SpawnedChildren.Add(YLabel);
    }

    FVector ZLabelPos = Z1 + GraphRotation.RotateVector(FVector(0, 0, 50));
    if (AAxisTickActor* ZLabel = GetWorld()->SpawnActor<AAxisTickActor>(AAxisTickActor::StaticClass(), FTransform(ZLabelPos), LabelParams))
    {
        ZLabel->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
        ZLabel->InitializeTick(ZLabelPos, FString::Printf(TEXT("Z: %.1f"), AxisMaxZ), (GraphRotation * FRotator(90, 0, 0).Quaternion()).Rotator());
        SpawnedChildren.Add(ZLabel);
    }
}

void ALineGraphActor::Rebuild()
{
    ClearChildrenActors();
    LoadData();

    if (DataPoints.Num() == 0)
        return;

    DataMinX = DataMaxX = DataPoints[0].X;
    DataMinY = DataMaxY = DataPoints[0].Y;
    DataMinZ = DataMaxZ = DataPoints[0].Z;

    for (const FVector& P : DataPoints)
    {
        DataMinX = FMath::Min(DataMinX, P.X); DataMaxX = FMath::Max(DataMaxX, P.X);
        DataMinY = FMath::Min(DataMinY, P.Y); DataMaxY = FMath::Max(DataMaxY, P.Y);
        DataMinZ = FMath::Min(DataMinZ, P.Z); DataMaxZ = FMath::Max(DataMaxZ, P.Z);
    }

    const float RangeMinX = bUseCustomRange ? XMin : DataMinX;
    const float RangeMaxX = bUseCustomRange ? XMax : DataMaxX;
    const float RangeMinY = bUseCustomRange ? YMin : DataMinY;
    const float RangeMaxY = bUseCustomRange ? YMax : DataMaxY;
    const float RangeMinZ = bUseCustomRange ? ZMin : DataMinZ;
    const float RangeMaxZ = bUseCustomRange ? ZMax : DataMaxZ;

    const FAxisGridConfig XConfig = DataVizGrid::ComputeAxisGrid(RangeMinX, RangeMaxX, TargetTicks);
    const FAxisGridConfig YConfig = DataVizGrid::ComputeAxisGrid(RangeMinY, RangeMaxY, TargetTicks);
    const FAxisGridConfig ZConfig = DataVizGrid::ComputeAxisGrid(RangeMinZ, RangeMaxZ, TargetTicks);

    AxisMinX = XConfig.AxisMin; AxisMaxX = XConfig.AxisMax; AxisStepX = XConfig.TickStep;
    AxisMinY = YConfig.AxisMin; AxisMaxY = YConfig.AxisMax; AxisStepY = YConfig.TickStep;
    AxisMinZ = ZConfig.AxisMin; AxisMaxZ = ZConfig.AxisMax; AxisStepZ = ZConfig.TickStep;

    GraphOrigin = GetActorLocation();
    GeneratePoints();
    GenerateLines();
    GenerateAxes();
    GenerateGridlines();
}

void ALineGraphActor::GenerateGridlines()
{
    FActorSpawnParameters GridParams;
    GridParams.Owner = this;
    const FQuat GraphRotation = GetActorQuat() * AdditionalRotation.Quaternion();

    auto SpawnLine = [this, &GridParams](const FVector& Start, const FVector& End)
    {
        if (AGridLineActor* Edge = GetWorld()->SpawnActor<AGridLineActor>(AGridLineActor::StaticClass(), FTransform(Start), GridParams))
        {
            Edge->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
            Edge->InitializeLine(Start, End, FLinearColor::Black, 2.0f);
            SpawnedChildren.Add(Edge);
        }
    };

    auto ForEachTick = [](float Start, float End, float Step, TFunctionRef<void(float)> Fn)
    {
        if (Step <= KINDA_SMALL_NUMBER) return;
        const float Limit = End + Step * 0.5f;
        for (float V = Start; V <= Limit; V += Step)
        {
            Fn(V);
        }
    };

    ForEachTick(AxisMinX, AxisMaxX, AxisStepX, [&](float XVal)
    {
        FVector XPos = MapDataToWorld(FVector(XVal, 0.0f, 0.0f));
        SpawnLine(MapDataToWorld(FVector(XVal, AxisMinY, 0.0f)), MapDataToWorld(FVector(XVal, AxisMaxY, 0.0f)));
        SpawnLine(MapDataToWorld(FVector(XVal, 0.0f, AxisMinZ)), MapDataToWorld(FVector(XVal, 0.0f, AxisMaxZ)));
        
        FVector XTickStart = XPos + GraphRotation.RotateVector(FVector(0, -20, 0));
        FVector XTickEnd = XPos + GraphRotation.RotateVector(FVector(0, 20, 0));
        if (AGridLineActor* Tick = GetWorld()->SpawnActor<AGridLineActor>(AGridLineActor::StaticClass(), FTransform(XTickStart), GridParams))
        {
            Tick->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
            Tick->InitializeLine(XTickStart, XTickEnd, FLinearColor::Black, 2.0f);
            SpawnedChildren.Add(Tick);
        }
        
        FVector XLabelPos = XPos + GraphRotation.RotateVector(FVector(0, 30, 0));
        if (AAxisTickActor* Label = GetWorld()->SpawnActor<AAxisTickActor>(AAxisTickActor::StaticClass(), FTransform(XLabelPos), GridParams))
        {
            Label->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
            Label->InitializeTick(XLabelPos, FString::SanitizeFloat(XVal), (GraphRotation * FRotator(0, 90, 0).Quaternion()).Rotator());
            Label->SetActorScale3D(FVector(TextScale));
            SpawnedChildren.Add(Label);
        }
    });

    ForEachTick(AxisMinY, AxisMaxY, AxisStepY, [&](float YVal)
    {
        FVector YPos = MapDataToWorld(FVector(0.0f, YVal, 0.0f));
        SpawnLine(MapDataToWorld(FVector(AxisMinX, YVal, 0.0f)), MapDataToWorld(FVector(AxisMaxX, YVal, 0.0f)));
        SpawnLine(MapDataToWorld(FVector(0.0f, YVal, AxisMinZ)), MapDataToWorld(FVector(0.0f, YVal, AxisMaxZ)));
        
        FVector YTickStart = YPos + GraphRotation.RotateVector(FVector(-20, 0, 0));
        FVector YTickEnd = YPos + GraphRotation.RotateVector(FVector(20, 0, 0));
        if (AGridLineActor* Tick = GetWorld()->SpawnActor<AGridLineActor>(AGridLineActor::StaticClass(), FTransform(YTickStart), GridParams))
        {
            Tick->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
            Tick->InitializeLine(YTickStart, YTickEnd, FLinearColor::Black, 2.0f);
            SpawnedChildren.Add(Tick);
        }
        
        FVector YLabelPos = YPos + GraphRotation.RotateVector(FVector(-35, 0, 0));
        if (AAxisTickActor* Label = GetWorld()->SpawnActor<AAxisTickActor>(AAxisTickActor::StaticClass(), FTransform(YLabelPos), GridParams))
        {
            Label->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
            Label->InitializeTick(YLabelPos, FString::SanitizeFloat(YVal), (GraphRotation * FRotator::ZeroRotator.Quaternion()).Rotator());
            Label->SetActorScale3D(FVector(TextScale));
            SpawnedChildren.Add(Label);
        }
    });

    ForEachTick(AxisMinZ, AxisMaxZ, AxisStepZ, [&](float ZVal)
    {
        FVector ZPos = MapDataToWorld(FVector(0.0f, 0.0f, ZVal));
        SpawnLine(MapDataToWorld(FVector(AxisMinX, 0.0f, ZVal)), MapDataToWorld(FVector(AxisMaxX, 0.0f, ZVal)));
        SpawnLine(MapDataToWorld(FVector(0.0f, AxisMinY, ZVal)), MapDataToWorld(FVector(0.0f, AxisMaxY, ZVal)));
        
        FVector ZTickStart = ZPos + GraphRotation.RotateVector(FVector(0, -20, 0));
        FVector ZTickEnd = ZPos + GraphRotation.RotateVector(FVector(0, 20, 0));
        if (AGridLineActor* Tick = GetWorld()->SpawnActor<AGridLineActor>(AGridLineActor::StaticClass(), FTransform(ZTickStart), GridParams))
        {
            Tick->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
            Tick->InitializeLine(ZTickStart, ZTickEnd, FLinearColor::Black, 2.0f);
            SpawnedChildren.Add(Tick);
        }
        
        FVector ZLabelPos = ZPos + GraphRotation.RotateVector(FVector(0, -35, 0));
        if (AAxisTickActor* Label = GetWorld()->SpawnActor<AAxisTickActor>(AAxisTickActor::StaticClass(), FTransform(ZLabelPos), GridParams))
        {
            Label->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
            Label->InitializeTick(ZLabelPos, FString::SanitizeFloat(ZVal), (GraphRotation * FRotator(90, 0, 0).Quaternion()).Rotator());
            Label->SetActorScale3D(FVector(TextScale));
            SpawnedChildren.Add(Label);
        }
    });
}

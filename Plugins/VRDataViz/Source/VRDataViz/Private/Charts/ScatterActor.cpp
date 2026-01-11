#include "Charts/ScatterActor.h"
#include "Engine/World.h"
#include "Charts/ScatterPointActor.h"
#include "Charts/GridLineActor.h"
#include "Charts/AxisTickActor.h"
#include "Charts/GridMath.h"
#include "Kismet/KismetMathLibrary.h"

AScatterActor::AScatterActor()
{
    PrimaryActorTick.bCanEverTick = false;
    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    RootComponent = Root;
}

void AScatterActor::BeginPlay()
{
    Super::BeginPlay();
    LoadSampleData();
    GenerateScatterplot();
}

void AScatterActor::LoadSampleData()
{
    DataPoints.Empty();
    if (!ScatterDataTable) return;

    TArray<FVRScatterData*> AllRows;
    ScatterDataTable->GetAllRows(TEXT("VR Scatter"), AllRows);
    for (FVRScatterData* RowPtr : AllRows)
    {
        if (RowPtr)
        {
            DataPoints.Add(FVector(RowPtr->X, RowPtr->Y, RowPtr->Z));
        }
    }
}

void AScatterActor::GenerateScatterplot()
{
    if (DataPoints.Num() == 0) return;
    
    // Find min/max for axis and color mapping
    float DataMinX = DataPoints[0].X;
    float DataMaxX = DataPoints[0].X;
    float DataMinY = DataPoints[0].Y;
    float DataMaxY = DataPoints[0].Y;
    float DataMinZ = DataPoints[0].Z;
    float DataMaxZ = DataPoints[0].Z;
    for (const FVector& Point : DataPoints)
    {
        DataMinX = FMath::Min(DataMinX, Point.X);
        DataMaxX = FMath::Max(DataMaxX, Point.X);
        DataMinY = FMath::Min(DataMinY, Point.Y);
        DataMaxY = FMath::Max(DataMaxY, Point.Y);
        DataMinZ = FMath::Min(DataMinZ, Point.Z);
        DataMaxZ = FMath::Max(DataMaxZ, Point.Z);
    }

    const float RangeMinX = bUseCustomRange ? XMin : DataMinX;
    const float RangeMaxX = bUseCustomRange ? XMax : DataMaxX;
    const float RangeMinY = bUseCustomRange ? YMin : DataMinY;
    const float RangeMaxY = bUseCustomRange ? YMax : DataMaxY;
    const float RangeMinZ = bUseCustomRange ? ZMin : DataMinZ;
    const float RangeMaxZ = bUseCustomRange ? ZMax : DataMaxZ;

    const FAxisGridConfig XConfig = DataVizGrid::ComputeAxisGrid(RangeMinX, RangeMaxX, 6);
    const FAxisGridConfig YConfig = DataVizGrid::ComputeAxisGrid(RangeMinY, RangeMaxY, 6);
    const FAxisGridConfig ZConfig = DataVizGrid::ComputeAxisGrid(RangeMinZ, RangeMaxZ, 6);

    AxisMinX = XConfig.AxisMin; AxisMaxX = XConfig.AxisMax; AxisStepX = XConfig.TickStep;
    AxisMinY = YConfig.AxisMin; AxisMaxY = YConfig.AxisMax; AxisStepY = YConfig.TickStep;
    AxisMinZ = ZConfig.AxisMin; AxisMaxZ = ZConfig.AxisMax; AxisStepZ = ZConfig.TickStep;

    const float ZColorMin = bUseCustomRange ? ZMin : DataMinZ;
    const float ZColorMax = bUseCustomRange ? ZMax : DataMaxZ;
    const float ZRange = ZColorMax - ZColorMin;

    const FQuat GraphRotation = GetActorQuat() * AdditionalRotation.Quaternion();
    const auto GraphToWorld = [this, GraphRotation](const FVector& Local)
    {
        return GetActorLocation() + GraphRotation.RotateVector(Local * GraphScale);
    };

    // Generate scatter points with color based on Z value
    for (const FVector& Point : DataPoints)
    {
        float OriginalZ = Point.Z;
        const FVector WorldLoc = GraphToWorld(Point);
        
        // Color mapping: Blue (low) -> Green -> Yellow -> Red (high)
        FLinearColor PointColor = FLinearColor::White;
        if (ZRange > 0.001f)
        {
            float NormalizedZ = (OriginalZ - ZColorMin) / ZRange;
            FLinearColor Cyan(0.0f, 1.0f, 1.0f, 1.0f); // Define Cyan color
            if (NormalizedZ < 0.33f)
            {
                // Blue to Cyan
                PointColor = UKismetMathLibrary::LinearColorLerp(FLinearColor::Blue, Cyan, NormalizedZ / 0.33f);
            }
            else if (NormalizedZ < 0.66f)
            {
                // Cyan to Green
                PointColor = UKismetMathLibrary::LinearColorLerp(Cyan, FLinearColor::Green, (NormalizedZ - 0.33f) / 0.33f);
            }
            else
            {
                // Green to Yellow to Red
                float T = (NormalizedZ - 0.66f) / 0.34f;
                if (T < 0.5f)
                    PointColor = UKismetMathLibrary::LinearColorLerp(FLinearColor::Green, FLinearColor::Yellow, T * 2.0f);
                else
                    PointColor = UKismetMathLibrary::LinearColorLerp(FLinearColor::Yellow, FLinearColor::Red, (T - 0.5f) * 2.0f);
            }
        }
        
        FActorSpawnParameters Params;
        Params.Owner = this;
        if (AScatterPointActor* SP = GetWorld()->SpawnActor<AScatterPointActor>(AScatterPointActor::StaticClass(), FTransform(WorldLoc), Params))
        {
            SP->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
            SP->InitializePoint(WorldLoc, PointColor, PointScale);
            SpawnedChildren.Add(SP);
        }
    }

    // Generate gridlines and axis labels
    GenerateGridlinesAndLabels();
}

void AScatterActor::GenerateGridlinesAndLabels()
{
    const FQuat GraphRotation = GetActorQuat() * AdditionalRotation.Quaternion();
    const auto GraphToWorld = [this, GraphRotation](const FVector& Local)
    {
        return GetActorLocation() + GraphRotation.RotateVector(Local * GraphScale);
    };
    const auto RotateLabel = [GraphRotation](const FRotator& LocalRot)
    {
        return (GraphRotation * LocalRot.Quaternion()).Rotator();
    };

    // Generate perpendicular grid rectangles from each tick that stretch across the graph
    FActorSpawnParameters GridParams;
    GridParams.Owner = this;

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
        FVector XPos = GraphToWorld(FVector(XVal, 0.0f, 0.0f));
        SpawnLine(GraphToWorld(FVector(XVal, AxisMinY, 0.0f)), GraphToWorld(FVector(XVal, AxisMaxY, 0.0f)));
        SpawnLine(GraphToWorld(FVector(XVal, 0.0f, AxisMinZ)), GraphToWorld(FVector(XVal, 0.0f, AxisMaxZ)));

        FActorSpawnParameters TickParams;
        TickParams.Owner = this;

        FVector XTickStart = XPos + GraphRotation.RotateVector(FVector(0, -20, 0));
        FVector XTickEnd = XPos + GraphRotation.RotateVector(FVector(0, 20, 0));
        if (AGridLineActor* Tick = GetWorld()->SpawnActor<AGridLineActor>(AGridLineActor::StaticClass(), FTransform(XTickStart), TickParams))
        {
            Tick->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
            Tick->InitializeLine(XTickStart, XTickEnd, FLinearColor::Black, 2.0f);
            SpawnedChildren.Add(Tick);
        }
        FVector XLabelPos = XPos + GraphRotation.RotateVector(FVector(0, 30, 0));
        if (AAxisTickActor* Label = GetWorld()->SpawnActor<AAxisTickActor>(AAxisTickActor::StaticClass(), FTransform(XLabelPos), TickParams))
        {
            Label->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
            Label->InitializeTick(XLabelPos, FString::SanitizeFloat(XVal), RotateLabel(FRotator(0, 90, 0)));
            Label->SetActorScale3D(FVector(TextScale));
            SpawnedChildren.Add(Label);
        }
    });

    ForEachTick(AxisMinY, AxisMaxY, AxisStepY, [&](float YVal)
    {
        FVector YPos = GraphToWorld(FVector(0.0f, YVal, 0.0f));
        SpawnLine(GraphToWorld(FVector(AxisMinX, YVal, 0.0f)), GraphToWorld(FVector(AxisMaxX, YVal, 0.0f)));
        SpawnLine(GraphToWorld(FVector(0.0f, YVal, AxisMinZ)), GraphToWorld(FVector(0.0f, YVal, AxisMaxZ)));

        FActorSpawnParameters TickParams;
        TickParams.Owner = this;

        FVector YTickStart = YPos + GraphRotation.RotateVector(FVector(-20, 0, 0));
        FVector YTickEnd = YPos + GraphRotation.RotateVector(FVector(20, 0, 0));
        if (AGridLineActor* Tick = GetWorld()->SpawnActor<AGridLineActor>(AGridLineActor::StaticClass(), FTransform(YTickStart), TickParams))
        {
            Tick->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
            Tick->InitializeLine(YTickStart, YTickEnd, FLinearColor::Black, 2.0f);
            SpawnedChildren.Add(Tick);
        }
        FVector YLabelPos = YPos + GraphRotation.RotateVector(FVector(-35, 0, 0));
        if (AAxisTickActor* Label = GetWorld()->SpawnActor<AAxisTickActor>(AAxisTickActor::StaticClass(), FTransform(YLabelPos), TickParams))
        {
            Label->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
            Label->InitializeTick(YLabelPos, FString::SanitizeFloat(YVal), RotateLabel(FRotator::ZeroRotator));
            Label->SetActorScale3D(FVector(TextScale));
            SpawnedChildren.Add(Label);
        }
    });

    ForEachTick(AxisMinZ, AxisMaxZ, AxisStepZ, [&](float ZVal)
    {
        FVector ZPos = GraphToWorld(FVector(0.0f, 0.0f, ZVal));
        SpawnLine(GraphToWorld(FVector(AxisMinX, 0.0f, ZVal)), GraphToWorld(FVector(AxisMaxX, 0.0f, ZVal)));
        SpawnLine(GraphToWorld(FVector(0.0f, AxisMinY, ZVal)), GraphToWorld(FVector(0.0f, AxisMaxY, ZVal)));

        FActorSpawnParameters TickParams;
        TickParams.Owner = this;

        FVector ZTickStart = ZPos + GraphRotation.RotateVector(FVector(0, -20, 0));
        FVector ZTickEnd = ZPos + GraphRotation.RotateVector(FVector(0, 20, 0));
        if (AGridLineActor* Tick = GetWorld()->SpawnActor<AGridLineActor>(AGridLineActor::StaticClass(), FTransform(ZTickStart), TickParams))
        {
            Tick->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
            Tick->InitializeLine(ZTickStart, ZTickEnd, FLinearColor::Black, 2.0f);
            SpawnedChildren.Add(Tick);
        }
        FVector ZLabelPos = ZPos + GraphRotation.RotateVector(FVector(0, -35, 0));
        if (AAxisTickActor* Label = GetWorld()->SpawnActor<AAxisTickActor>(AAxisTickActor::StaticClass(), FTransform(ZLabelPos), TickParams))
        {
            Label->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
            Label->InitializeTick(ZLabelPos, FString::SanitizeFloat(ZVal), RotateLabel(FRotator(90, 0, 0)));
            Label->SetActorScale3D(FVector(TextScale));
            SpawnedChildren.Add(Label);
        }
    });
}

void AScatterActor::ClearChildrenActors()
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

void AScatterActor::Rebuild()
{
    ClearChildrenActors();
    LoadSampleData();
    GenerateScatterplot();
}



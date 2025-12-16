#include "Charts/ScatterActor.h"
#include "Engine/World.h"
#include "Charts/ScatterPointActor.h"
#include "Charts/GridLineActor.h"
#include "Charts/AxisTickActor.h"
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

    auto Remap = [](float V, float A, float B){ return (B - A) != 0.f ? (V - A) / (B - A) : 0.f; };
    
    // Find min/max Z for color mapping
    float MinZ = DataPoints[0].Z;
    float MaxZ = DataPoints[0].Z;
    for (const FVector& Point : DataPoints)
    {
        if (bUseCustomRange)
        {
            MinZ = FMath::Min(MinZ, ZMin);
            MaxZ = FMath::Max(MaxZ, ZMax);
        }
        else
        {
            MinZ = FMath::Min(MinZ, Point.Z);
            MaxZ = FMath::Max(MaxZ, Point.Z);
        }
    }
    const float ZRange = MaxZ - MinZ;

    // Generate scatter points with color based on Z value
    for (const FVector& Point : DataPoints)
    {
        FVector P = Point;
        float OriginalZ = Point.Z;
        if (bUseCustomRange)
        {
            P.X = Remap(P.X, XMin, XMax);
            P.Y = Remap(P.Y, YMin, YMax);
            P.Z = Remap(P.Z, ZMin, ZMax);
        }
        P *= UnitScale;
        const FVector WorldLoc = GetActorLocation() + P;
        
        // Color mapping: Blue (low) -> Green -> Yellow -> Red (high)
        FLinearColor PointColor = FLinearColor::White;
        if (ZRange > 0.001f)
        {
            float NormalizedZ = (OriginalZ - MinZ) / ZRange;
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
        if (AScatterPointActor* SP = GetWorld()->SpawnActor<AScatterPointActor>(AScatterPointActor::StaticClass(), FTransform(AdditionalRotation, WorldLoc), Params))
        {
            SP->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
            SP->InitializePoint(WorldLoc, PointColor, 0.5f);
            SpawnedChildren.Add(SP);
        }
    }

    // Generate gridlines and axis labels
    GenerateGridlinesAndLabels();
}

void AScatterActor::GenerateGridlinesAndLabels()
{
    const FVector Origin = GetActorLocation();
    const FVector Scale = UnitScale;
    
    // Calculate data ranges
    float MinX = DataPoints[0].X, MaxX = DataPoints[0].X;
    float MinY = DataPoints[0].Y, MaxY = DataPoints[0].Y;
    float MinZ = DataPoints[0].Z, MaxZ = DataPoints[0].Z;
    
    for (const FVector& Point : DataPoints)
    {
        MinX = FMath::Min(MinX, Point.X); MaxX = FMath::Max(MaxX, Point.X);
        MinY = FMath::Min(MinY, Point.Y); MaxY = FMath::Max(MaxY, Point.Y);
        MinZ = FMath::Min(MinZ, Point.Z); MaxZ = FMath::Max(MaxZ, Point.Z);
    }
    
    if (bUseCustomRange)
    {
        MinX = XMin; MaxX = XMax;
        MinY = YMin; MaxY = YMax;
        MinZ = ZMin; MaxZ = ZMax;
    }
    
    const float RangeX = MaxX - MinX;
    const float RangeY = MaxY - MinY;
    const float RangeZ = MaxZ - MinZ;
    
    // Generate gridlines (5 grid lines per axis)
    const int32 GridCount = 5;
    for (int32 i = 0; i <= GridCount; ++i)
    {
        float T = i / (float)GridCount;
        
        // X-axis gridlines (parallel to YZ plane)
        float XVal = FMath::Lerp(MinX, MaxX, T);
        FVector StartX = Origin + FVector((XVal - MinX) * Scale.X, 0, 0);
        FVector EndX1 = StartX + FVector(0, RangeY * Scale.Y, 0);
        FVector EndX2 = StartX + FVector(0, 0, RangeZ * Scale.Z);
        
        FActorSpawnParameters GridParams;
        GridParams.Owner = this;
        if (AGridLineActor* Grid = GetWorld()->SpawnActor<AGridLineActor>(AGridLineActor::StaticClass(), FTransform(StartX), GridParams))
        {
            Grid->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
            Grid->InitializeLine(StartX, EndX1, FLinearColor::Gray, 1.0f);
            SpawnedChildren.Add(Grid);
        }
        if (AGridLineActor* Grid = GetWorld()->SpawnActor<AGridLineActor>(AGridLineActor::StaticClass(), FTransform(StartX), GridParams))
        {
            Grid->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
            Grid->InitializeLine(StartX, EndX2, FLinearColor::Gray, 1.0f);
            SpawnedChildren.Add(Grid);
        }
        
        // Y-axis gridlines (parallel to XZ plane)
        float YVal = FMath::Lerp(MinY, MaxY, T);
        FVector StartY = Origin + FVector(0, (YVal - MinY) * Scale.Y, 0);
        FVector EndY1 = StartY + FVector(RangeX * Scale.X, 0, 0);
        FVector EndY2 = StartY + FVector(0, 0, RangeZ * Scale.Z);
        
        if (AGridLineActor* Grid = GetWorld()->SpawnActor<AGridLineActor>(AGridLineActor::StaticClass(), FTransform(StartY), GridParams))
        {
            Grid->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
            Grid->InitializeLine(StartY, EndY1, FLinearColor::Gray, 1.0f);
            SpawnedChildren.Add(Grid);
        }
        if (AGridLineActor* Grid = GetWorld()->SpawnActor<AGridLineActor>(AGridLineActor::StaticClass(), FTransform(StartY), GridParams))
        {
            Grid->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
            Grid->InitializeLine(StartY, EndY2, FLinearColor::Gray, 1.0f);
            SpawnedChildren.Add(Grid);
        }
        
        // Z-axis gridlines (parallel to XY plane)
        float ZVal = FMath::Lerp(MinZ, MaxZ, T);
        FVector StartZ = Origin + FVector(0, 0, (ZVal - MinZ) * Scale.Z);
        FVector EndZ1 = StartZ + FVector(RangeX * Scale.X, 0, 0);
        FVector EndZ2 = StartZ + FVector(0, RangeY * Scale.Y, 0);
        
        if (AGridLineActor* Grid = GetWorld()->SpawnActor<AGridLineActor>(AGridLineActor::StaticClass(), FTransform(StartZ), GridParams))
        {
            Grid->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
            Grid->InitializeLine(StartZ, EndZ1, FLinearColor::Gray, 1.0f);
            SpawnedChildren.Add(Grid);
        }
        if (AGridLineActor* Grid = GetWorld()->SpawnActor<AGridLineActor>(AGridLineActor::StaticClass(), FTransform(StartZ), GridParams))
        {
            Grid->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
            Grid->InitializeLine(StartZ, EndZ2, FLinearColor::Gray, 1.0f);
            SpawnedChildren.Add(Grid);
        }
        
        // Axis labels - only show at start, middle, and end (reduce clutter)
        if (i == 0 || i == GridCount / 2 || i == GridCount)
        {
            FString XLabel = FString::Printf(TEXT("%.2f"), XVal);
            FString YLabel = FString::Printf(TEXT("%.2f"), YVal);
            FString ZLabel = FString::Printf(TEXT("%.2f"), ZVal);
            
            FActorSpawnParameters TickParams;
            TickParams.Owner = this;
            if (AAxisTickActor* Tick = GetWorld()->SpawnActor<AAxisTickActor>(AAxisTickActor::StaticClass(), FTransform(StartX + FVector(0, -50, 0)), TickParams))
            {
                Tick->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
                Tick->InitializeTick(StartX + FVector(0, -50, 0), XLabel, FRotator(0, 90, 0));
                SpawnedChildren.Add(Tick);
            }
            if (AAxisTickActor* Tick = GetWorld()->SpawnActor<AAxisTickActor>(AAxisTickActor::StaticClass(), FTransform(StartY + FVector(-50, 0, 0)), TickParams))
            {
                Tick->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
                Tick->InitializeTick(StartY + FVector(-50, 0, 0), YLabel, FRotator::ZeroRotator);
                SpawnedChildren.Add(Tick);
            }
            if (AAxisTickActor* Tick = GetWorld()->SpawnActor<AAxisTickActor>(AAxisTickActor::StaticClass(), FTransform(StartZ + FVector(0, 0, -50)), TickParams))
            {
                Tick->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
                Tick->InitializeTick(StartZ + FVector(0, 0, -50), ZLabel, FRotator(90, 0, 0));
                SpawnedChildren.Add(Tick);
            }
        }
    }
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



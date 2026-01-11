#include "Charts/BarChartActor.h"
#include "Engine/World.h"
#include "Components/TextRenderComponent.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"
#include "Charts/AxisTickActor.h"
#include "Charts/GridLineActor.h"
#include "Charts/BarActor.h"
#include "Charts/GridMath.h"

ABarChartActor::ABarChartActor()
{
    PrimaryActorTick.bCanEverTick = false;
    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    RootComponent = Root;

    static ConstructorHelpers::FObjectFinder<UMaterialInterface> ColorMatAsset(TEXT("/Game/Materials/M_Color"));
    if (ColorMatAsset.Succeeded())
    {
        BarColorMaterial = ColorMatAsset.Object;
    }
}

void ABarChartActor::BeginPlay()
{
    Super::BeginPlay();
    Rebuild();
}

void ABarChartActor::LoadBarData()
{
    BarPoints.Empty();

    if (RuntimeBarPoints.Num() > 0)
    {
        BarPoints = RuntimeBarPoints;
        return;
    }

    if (!BarDataTable) return;

    TArray<FVRBarData*> AllRows;
    BarDataTable->GetAllRows(TEXT("VR Bar Chart"), AllRows);
    for (FVRBarData* RowPtr : AllRows)
    {
        if (RowPtr)
        {
            BarPoints.Add(*RowPtr);
        }
    }
}

FLinearColor ABarChartActor::GetBarColor(int32 BarIndex)
{
    // Use deterministic color based on bar index and seed
    // This ensures colors stay the same across rebuilds
    FRandomStream RandStream(ColorSeed + BarIndex * 7919); // 7919 is a prime for better distribution
    
    // Generate vibrant colors using HSV
    const float Hue = RandStream.FRand() * 360.0f;
    const float Saturation = 0.7f + RandStream.FRand() * 0.3f;
    const float Value = 0.8f + RandStream.FRand() * 0.2f;
    
    return FLinearColor::MakeFromHSV8(
        static_cast<uint8>(Hue / 360.0f * 255.0f),
        static_cast<uint8>(Saturation * 255.0f),
        static_cast<uint8>(Value * 255.0f)
    );
}

void ABarChartActor::GenerateBars()
{
    if (BarPoints.Num() == 0) return;

    // Collect unique labels and find data bounds
    TMap<int32, FString> XIndexToLabel;
    TMap<int32, FString> YIndexToLabel;
    float MaxValue = 0.0f;
    int32 MinXIndex = INT32_MAX;
    int32 MaxXIndex = INT32_MIN;
    int32 MinYIndex = INT32_MAX;
    int32 MaxYIndex = INT32_MIN;
    int32 MaxLabelLength = 1;

    for (const FVRBarData& Row : BarPoints)
    {
        MaxValue = FMath::Max(MaxValue, Row.Value);
        MinXIndex = FMath::Min(MinXIndex, Row.XIndex);
        MaxXIndex = FMath::Max(MaxXIndex, Row.XIndex);
        MinYIndex = FMath::Min(MinYIndex, Row.YIndex);
        MaxYIndex = FMath::Max(MaxYIndex, Row.YIndex);

        if (!XIndexToLabel.Contains(Row.XIndex))
        {
            XIndexToLabel.Add(Row.XIndex, Row.XLabel);
            MaxLabelLength = FMath::Max(MaxLabelLength, Row.XLabel.Len());
        }
        if (!YIndexToLabel.Contains(Row.YIndex))
        {
            YIndexToLabel.Add(Row.YIndex, Row.YLabel);
            MaxLabelLength = FMath::Max(MaxLabelLength, Row.YLabel.Len());
        }
    }

    if (MinXIndex == INT32_MAX) MinXIndex = 0;
    if (MaxXIndex == INT32_MIN) MaxXIndex = 0;
    if (MinYIndex == INT32_MAX) MinYIndex = 0;
    if (MaxYIndex == INT32_MIN) MaxYIndex = 0;

    // Apply GraphScale to all dimensions
    const FVector Scale(
        FMath::Max(GraphScale.X, 0.001f),
        FMath::Max(GraphScale.Y, 0.001f),
        FMath::Max(GraphScale.Z, 0.001f)
    );

    // Scaled dimensions
    const float ScaledCellX = CellSizeX * Scale.X;
    const float ScaledCellY = CellSizeY * Scale.Y;
    const float ScaledBarWidth = BarWidth * Scale.X;
    const float ScaledBarDepth = BarDepth * Scale.Y;
    const float ScaledHeightScale = HeightScale * Scale.Z;
    
    // Text size in world units
    const float EffectiveTextSize = TextWorldSize * TextScale;
    
    // Estimate text width based on character count (rough approximation: width ~= 0.6 * height per char)
    const float EstimatedTextWidth = EffectiveTextSize * 0.6f * MaxLabelLength;
    
    // Padding to prevent overlap
    const float BasePadding = 15.0f * FMath::Max(Scale.X, Scale.Y);
    const float HalfScaledBarWidth = ScaledBarWidth * 0.5f;
    const float HalfScaledBarDepth = ScaledBarDepth * 0.5f;
    
    // Label offset = bar edge + padding + full estimated text width
    const float XLabelOffset = HalfScaledBarDepth + BasePadding + EstimatedTextWidth;
    const float YLabelOffset = HalfScaledBarWidth + BasePadding + EstimatedTextWidth;
    
    // Label height above ground - ensure text is fully above ground
    const float LabelZHeight = EffectiveTextSize + BasePadding;

    const FVector Origin = GetActorLocation();
    const FQuat GraphRotation = GetActorQuat() * AdditionalRotation.Quaternion();

    auto LocalToWorld = [&](const FVector& Local) -> FVector
    {
        return Origin + GraphRotation.RotateVector(Local);
    };

    auto RotateLabel = [&](const FRotator& LocalRot) -> FRotator
    {
        return (GraphRotation * LocalRot.Quaternion()).Rotator();
    };

    auto ConfigureText = [this](AAxisTickActor* Tick)
    {
        if (!Tick) return;
        if (UTextRenderComponent* TextComp = Tick->FindComponentByClass<UTextRenderComponent>())
        {
            TextComp->SetWorldSize(TextWorldSize);
            TextComp->SetHorizontalAlignment(EHTA_Center);
            TextComp->SetVerticalAlignment(EVRTA_TextCenter);
        }
    };

    FActorSpawnParameters SpawnParams;
    SpawnParams.Owner = this;
    
    const FRotator ValueLabelRot(0.0f, 90.0f, 0.0f);

    // Spawn bars and value labels
    int32 BarIndex = 0;
    for (const FVRBarData& Row : BarPoints)
    {
        float ValueForHeight = Row.Value;
        if (bUseCustomRange)
        {
            ValueForHeight = FMath::Clamp(Row.Value, ZMin, ZMax);
        }
        ValueForHeight = FMath::Max(0.0f, ValueForHeight);

        const float BarHeight = ValueForHeight * ScaledHeightScale;

        // Bar center position
        const FVector LocalPos(
            Row.XIndex * ScaledCellX,
            Row.YIndex * ScaledCellY,
            BarHeight * 0.5f
        );
        const FVector WorldPos = LocalToWorld(LocalPos);

        // Get deterministic color for this bar
        const FLinearColor CurrentBarColor = bUniqueBarColors ? GetBarColor(BarIndex) : BarColor;

        if (ABarActor* Bar = GetWorld()->SpawnActor<ABarActor>(ABarActor::StaticClass(), FTransform(GraphRotation, WorldPos), SpawnParams))
        {
            Bar->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
            Bar->InitializeBar(WorldPos, GraphRotation, ScaledBarWidth, ScaledBarDepth, BarHeight, BarColorMaterial, CurrentBarColor);
            SpawnedChildren.Add(Bar);
        }

        // Value label on top of bar
        if (bShowValueLabels)
        {
            const float ValueLabelZ = BarHeight + EffectiveTextSize + BasePadding;
            const FVector LabelLocalPos(
                Row.XIndex * ScaledCellX,
                Row.YIndex * ScaledCellY,
                ValueLabelZ
            );
            const FVector LabelWorldPos = LocalToWorld(LabelLocalPos);

            if (AAxisTickActor* Tick = GetWorld()->SpawnActor<AAxisTickActor>(AAxisTickActor::StaticClass(), FTransform(GraphRotation, LabelWorldPos), SpawnParams))
            {
                Tick->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
                Tick->InitializeTick(LabelWorldPos, FString::Printf(TEXT("%.1f"), Row.Value), RotateLabel(ValueLabelRot));
                Tick->SetFaceCamera(true);
                Tick->SetActorScale3D(FVector(TextScale));
                ConfigureText(Tick);
                SpawnedChildren.Add(Tick);
            }
        }
        
        BarIndex++;
    }

    // X-axis labels (placed along Y edge)
    TArray<int32> SortedXIndices;
    XIndexToLabel.GetKeys(SortedXIndices);
    SortedXIndices.Sort();

    const FRotator XLabelRot(0.0f, 180.0f, 0.0f);

    for (int32 XIdx : SortedXIndices)
    {
        const FString& Label = XIndexToLabel[XIdx];
        
        const FVector LocalLabelPos(
            XIdx * ScaledCellX,
            MinYIndex * ScaledCellY - XLabelOffset,
            LabelZHeight
        );
        const FVector WorldLabelPos = LocalToWorld(LocalLabelPos);

        if (AAxisTickActor* Tick = GetWorld()->SpawnActor<AAxisTickActor>(AAxisTickActor::StaticClass(), FTransform(GraphRotation, WorldLabelPos), SpawnParams))
        {
            Tick->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
            Tick->InitializeTick(WorldLabelPos, Label, RotateLabel(XLabelRot));
            Tick->SetFaceCamera(false);
            Tick->SetActorScale3D(FVector(TextScale));
            ConfigureText(Tick);
            SpawnedChildren.Add(Tick);
        }
    }

    // Y-axis labels (placed along X edge)
    TArray<int32> SortedYIndices;
    YIndexToLabel.GetKeys(SortedYIndices);
    SortedYIndices.Sort();

    const FRotator YLabelRot(0.0f, -90.0f, 0.0f);

    for (int32 YIdx : SortedYIndices)
    {
        const FString& Label = YIndexToLabel[YIdx];
        
        const FVector LocalLabelPos(
            MinXIndex * ScaledCellX - YLabelOffset,
            YIdx * ScaledCellY,
            LabelZHeight
        );
        const FVector WorldLabelPos = LocalToWorld(LocalLabelPos);

        if (AAxisTickActor* Tick = GetWorld()->SpawnActor<AAxisTickActor>(AAxisTickActor::StaticClass(), FTransform(GraphRotation, WorldLabelPos), SpawnParams))
        {
            Tick->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
            Tick->InitializeTick(WorldLabelPos, Label, RotateLabel(YLabelRot));
            Tick->SetFaceCamera(false);
            Tick->SetActorScale3D(FVector(TextScale));
            ConfigureText(Tick);
            SpawnedChildren.Add(Tick);
        }
    }

    // Z-axis ticks (closer to the chart)
    const FVector AxisOrigin(
        MinXIndex * ScaledCellX - HalfScaledBarWidth - BasePadding,
        MinYIndex * ScaledCellY - HalfScaledBarDepth - BasePadding,
        0.0f
    );
    GenerateZAxisTicks(MaxValue, AxisOrigin, GraphRotation, ScaledBarWidth, ScaledBarDepth);
}

void ABarChartActor::GenerateZAxisTicks(float MaxValue, const FVector& AxisOrigin, const FQuat& GraphRotation, float ScaledBarWidth, float ScaledBarDepth)
{
    const FVector Origin = GetActorLocation();
    const FVector Scale(
        FMath::Max(GraphScale.X, 0.001f),
        FMath::Max(GraphScale.Y, 0.001f),
        FMath::Max(GraphScale.Z, 0.001f)
    );
    const float ScaledHeightScale = HeightScale * Scale.Z;
    const float EffectiveTextSize = TextWorldSize * TextScale;
    const float BasePadding = 15.0f * FMath::Max(Scale.X, Scale.Y);

    auto LocalToWorld = [&](const FVector& Local) -> FVector
    {
        return Origin + GraphRotation.RotateVector(Local);
    };

    auto RotateLabel = [&](const FRotator& LocalRot) -> FRotator
    {
        return (GraphRotation * LocalRot.Quaternion()).Rotator();
    };

    auto ConfigureText = [this](AAxisTickActor* Tick)
    {
        if (!Tick) return;
        if (UTextRenderComponent* TextComp = Tick->FindComponentByClass<UTextRenderComponent>())
        {
            TextComp->SetWorldSize(TextWorldSize);
            TextComp->SetHorizontalAlignment(EHTA_Center);
            TextComp->SetVerticalAlignment(EVRTA_TextCenter);
        }
    };

    const float DataMax = bUseCustomRange ? ZMax : MaxValue;
    const FAxisGridConfig ZConfig = DataVizGrid::ComputeAxisGrid(0.0f, DataMax, ZTickCount);

    FActorSpawnParameters SpawnParams;
    SpawnParams.Owner = this;

    const float TickLength = 20.0f * TextScale;
    const FVector TickDir(-1.0f, 0.0f, 0.0f);
    const FRotator ZLabelRot(0.0f, 90.0f, 0.0f);

    for (float Value = ZConfig.AxisMin; Value <= ZConfig.AxisMax + KINDA_SMALL_NUMBER; Value += ZConfig.TickStep)
    {
        if (Value < 0.0f) continue;

        const float WorldZ = Value * ScaledHeightScale;
        const FVector TickPos = FVector(AxisOrigin.X, AxisOrigin.Y, WorldZ);
        const FVector WorldTickPos = LocalToWorld(TickPos);

        const FVector TickStart = WorldTickPos;
        const FVector TickEnd = WorldTickPos + GraphRotation.RotateVector(TickDir * TickLength);

        if (AGridLineActor* TickLine = GetWorld()->SpawnActor<AGridLineActor>(AGridLineActor::StaticClass(), FTransform(TickStart), SpawnParams))
        {
            TickLine->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
            TickLine->InitializeLine(TickStart, TickEnd, FLinearColor::Black, 2.0f);
            SpawnedChildren.Add(TickLine);
        }

        // Z labels: offset by tick + padding + estimated text width for numbers
        const float ZLabelTextWidth = EffectiveTextSize * 0.6f * 5; // Assume max 5 chars for numbers
        const FVector LabelOffset3D = TickDir * (TickLength + BasePadding + ZLabelTextWidth);
        const FVector LabelPos = FVector(AxisOrigin.X, AxisOrigin.Y, WorldZ) + LabelOffset3D;
        const FVector WorldLabelPos = LocalToWorld(LabelPos);

        if (AAxisTickActor* Label = GetWorld()->SpawnActor<AAxisTickActor>(AAxisTickActor::StaticClass(), FTransform(GraphRotation, WorldLabelPos), SpawnParams))
        {
            Label->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
            
            FString LabelText;
            if (FMath::IsNearlyEqual(Value, FMath::RoundToFloat(Value)))
            {
                LabelText = FString::Printf(TEXT("%d"), FMath::RoundToInt(Value));
            }
            else
            {
                LabelText = FString::Printf(TEXT("%.1f"), Value);
            }
            
            Label->InitializeTick(WorldLabelPos, LabelText, RotateLabel(ZLabelRot));
            Label->SetFaceCamera(true);
            Label->SetActorScale3D(FVector(TextScale));
            ConfigureText(Label);
            SpawnedChildren.Add(Label);
        }
    }
}

void ABarChartActor::ClearChildrenActors()
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

void ABarChartActor::Rebuild()
{
    ClearChildrenActors();
    LoadBarData();
    GenerateBars();
}

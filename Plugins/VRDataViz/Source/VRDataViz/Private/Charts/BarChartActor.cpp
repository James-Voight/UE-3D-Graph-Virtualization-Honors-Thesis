#include "Charts/BarChartActor.h"
#include "Engine/World.h"
#include "Engine/TextRenderActor.h"
#include "Engine/StaticMeshActor.h"
#include "Components/TextRenderComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "Charts/AxisTickActor.h"
#include "Charts/GridLineActor.h"

ABarChartActor::ABarChartActor()
{
    PrimaryActorTick.bCanEverTick = false;
    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    RootComponent = Root;
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
    TArray<FVRBarData*> AllRows; BarDataTable->GetAllRows(TEXT("VR Bar Chart"), AllRows);
    for (FVRBarData* RowPtr : AllRows) { if (RowPtr) { BarPoints.Add(*RowPtr); } }
}

void ABarChartActor::GenerateBars()
{
    if (BarPoints.Num() == 0) return;

    TMap<FString, int32> XLabelToIndex;
    TMap<FString, int32> YLabelToIndex;
    float MaxValue = 0.0f;

    for (const FVRBarData& Row : BarPoints)
    {
        MaxValue = FMath::Max(MaxValue, Row.Value);
        if (!XLabelToIndex.Contains(Row.XLabel)) XLabelToIndex.Add(Row.XLabel, Row.XIndex);
        if (!YLabelToIndex.Contains(Row.YLabel)) YLabelToIndex.Add(Row.YLabel, Row.YIndex);

        float NormalizedVal = Row.Value;
        if (bUseCustomRange)
        {
            const float Den = (ZMax - ZMin);
            NormalizedVal = Den != 0.f ? (Row.Value - ZMin) / Den : 0.f;
        }
        const float Height = (NormalizedVal * BarSize.Z);
        FVector SpawnLocation = GetActorLocation() + FVector(-Row.XIndex * UnitScale.X, -Row.YIndex * UnitScale.Y, Height / 2.0f);

        // Spawn actual bar geometry (cube)
        FActorSpawnParameters Params;
        Params.Owner = this;
        AStaticMeshActor* BarActor = GetWorld()->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), FTransform(AdditionalRotation, SpawnLocation), Params);
        if (BarActor && BarActor->GetStaticMeshComponent())
        {
            // Set mobility to Movable so it can be attached to the chart actor
            BarActor->GetStaticMeshComponent()->SetMobility(EComponentMobility::Movable);
            BarActor->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
            BarActor->GetStaticMeshComponent()->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube")));
            BarActor->GetStaticMeshComponent()->SetWorldScale3D(FVector(BarSize.X / 100.0f, BarSize.Y / 100.0f, Height / 100.0f)); // Scale from cm to UE units
            BarActor->GetStaticMeshComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

            // Apply color based on value
            if (UMaterialInterface* Mat = BarActor->GetStaticMeshComponent()->GetMaterial(0))
            {
                UMaterialInstanceDynamic* DynMat = BarActor->GetStaticMeshComponent()->CreateDynamicMaterialInstance(0, Mat);
                if (DynMat)
                {
                    float ColorIntensity = NormalizedVal;
                    FLinearColor BarColor = FLinearColor::Blue;
                    if (ColorIntensity > 0.5f)
                    {
                        BarColor = FMath::Lerp(FLinearColor::Blue, FLinearColor::Red, (ColorIntensity - 0.5f) * 2.0f);
                    }
                    else
                    {
                        BarColor = FMath::Lerp(FLinearColor::Blue, FLinearColor::Green, ColorIntensity * 2.0f);
                    }
                    DynMat->SetVectorParameterValue(TEXT("BaseColor"), BarColor);
                    DynMat->SetVectorParameterValue(TEXT("Color"), BarColor);
                }
            }
            SpawnedChildren.Add(BarActor);
        }

        // Also spawn text label above the bar
        FVector TextLocation = SpawnLocation + FVector(0, 0, Height / 2.0f + 50.0f);
        ATextRenderActor* TextLabel = GetWorld()->SpawnActor<ATextRenderActor>(TextLocation, AdditionalRotation, Params);
        if (TextLabel && TextLabel->GetTextRender())
        {
            TextLabel->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
            TextLabel->GetTextRender()->SetText(FText::FromString(FString::Printf(TEXT("%.1f"), Row.Value)));
            TextLabel->GetTextRender()->SetWorldSize(50.f);
            TextLabel->GetTextRender()->SetHorizontalAlignment(EHorizTextAligment::EHTA_Center);

            // Prevent text from scaling with the chart by setting inverse scale
            FVector ChartScale = GetActorScale3D();
            FVector TextScale = FVector(1.0f / ChartScale.X, 1.0f / ChartScale.Y, 1.0f / ChartScale.Z);
            TextLabel->SetActorScale3D(TextScale);

            SpawnedChildren.Add(TextLabel);
        }
    }

    TArray<FString> UniqueXLabels; XLabelToIndex.GetKeys(UniqueXLabels);
    for (int32 i = 0; i < UniqueXLabels.Num(); ++i)
    {
        FString Label = UniqueXLabels[i];
        FVector XLabelLoc = GetActorLocation() + FVector(300.0f, -i * 150.0f, 0);
        if (AAxisTickActor* Tick = GetWorld()->SpawnActor<AAxisTickActor>(XLabelLoc, FRotator(0, 90, 0)))
        {
            Tick->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
            Tick->InitializeTick(XLabelLoc, Label, FRotator(0, 90, 0));
            SpawnedChildren.Add(Tick);
        }
    }

    TArray<FString> UniqueYLabels; YLabelToIndex.GetKeys(UniqueYLabels);
    for (int32 i = 0; i < UniqueYLabels.Num(); ++i)
    {
        FString Label = UniqueYLabels[i];
        FVector YLabelLoc = GetActorLocation() + FVector(-i * 150.0f, 300.0f, 0);
        if (AAxisTickActor* Tick = GetWorld()->SpawnActor<AAxisTickActor>(YLabelLoc, FRotator::ZeroRotator))
        {
            Tick->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
            Tick->InitializeTick(YLabelLoc, Label, FRotator::ZeroRotator);
            SpawnedChildren.Add(Tick);
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



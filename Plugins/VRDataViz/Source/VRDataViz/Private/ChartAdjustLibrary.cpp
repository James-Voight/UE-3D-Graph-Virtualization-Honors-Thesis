#include "ChartAdjustLibrary.h"
#include "Charts/BarChartActor.h"
#include "Charts/LineGraphActor.h"
#include "Charts/ScatterActor.h"

void UChartAdjustLibrary::SetActorScale(AActor* ChartActor, const FVector& NewUnitScale)
{
    if (ABarChartActor* A = Cast<ABarChartActor>(ChartActor)) { A->GraphScale = NewUnitScale; A->Rebuild(); return; }
    if (ALineGraphActor* A = Cast<ALineGraphActor>(ChartActor)) { A->GraphScale = NewUnitScale; A->Rebuild(); return; }
    if (AScatterActor* A = Cast<AScatterActor>(ChartActor)) { A->GraphScale = NewUnitScale; A->Rebuild(); return; }
}

void UChartAdjustLibrary::SetActorRotation(AActor* ChartActor, const FRotator& NewRotation)
{
    if (ABarChartActor* A = Cast<ABarChartActor>(ChartActor)) { A->AdditionalRotation = NewRotation; A->Rebuild(); return; }
    if (ALineGraphActor* A = Cast<ALineGraphActor>(ChartActor)) { A->AdditionalRotation = NewRotation; A->Rebuild(); return; }
    if (AScatterActor* A = Cast<AScatterActor>(ChartActor)) { A->AdditionalRotation = NewRotation; A->Rebuild(); return; }
}

void UChartAdjustLibrary::SetAxisRanges(AActor* ChartActor, bool bUseCustom, float XMin, float XMax, float YMin, float YMax, float ZMin, float ZMax)
{
    if (ABarChartActor* A = Cast<ABarChartActor>(ChartActor))
    {
        A->bUseCustomRange = bUseCustom;
        A->ZMin = ZMin; A->ZMax = ZMax;
        A->Rebuild();
        return;
    }
    if (ALineGraphActor* A = Cast<ALineGraphActor>(ChartActor))
    {
        A->bUseCustomRange = bUseCustom;
        A->XMin = XMin; A->XMax = XMax; A->YMin = YMin; A->YMax = YMax; A->ZMin = ZMin; A->ZMax = ZMax;
        A->Rebuild();
        return;
    }
    if (AScatterActor* A = Cast<AScatterActor>(ChartActor))
    {
        A->bUseCustomRange = bUseCustom;
        A->XMin = XMin; A->XMax = XMax; A->YMin = YMin; A->YMax = YMax; A->ZMin = ZMin; A->ZMax = ZMax;
        A->Rebuild();
        return;
    }
}

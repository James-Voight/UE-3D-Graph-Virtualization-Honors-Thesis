#include "ChartSpawnLibrary.h"
#include "Engine/World.h"
#include "Engine/DataTable.h"
#include "UObject/Package.h"
#include "UObject/NoExportTypes.h"
#include "Runtime/Launch/Resources/Version.h"
#include "Charts/BarChartActor.h"
#include "Charts/LineGraphActor.h"
#include "Charts/ScatterActor.h"
#include "HAL/PlatformFilemanager.h"

static UDataTable* CreateTransientDataTable(UScriptStruct* RowStruct)
{
    if (!RowStruct)
    {
        return nullptr;
    }

    UPackage* Package = GetTransientPackage();
    UDataTable* DataTable = NewObject<UDataTable>(Package, UDataTable::StaticClass(), NAME_None, RF_Transient);
    DataTable->RowStruct = RowStruct;
    return DataTable;
}

bool UChartSpawnLibrary::ParseCSVToDataTable(const FString& CSVText, UScriptStruct* RowStruct, UDataTable*& OutDataTable)
{
    OutDataTable = nullptr;

    if (CSVText.IsEmpty() || RowStruct == nullptr)
    {
        UE_LOG(LogTemp, Error, TEXT("ParseCSVToDataTable - CSVText is empty or RowStruct is null"));
        return false;
    }

    UDataTable* DataTable = CreateTransientDataTable(RowStruct);
    if (!DataTable)
    {
        UE_LOG(LogTemp, Error, TEXT("ParseCSVToDataTable - Failed to create DataTable"));
        return false;
    }

#if (ENGINE_MAJOR_VERSION >= 5)
    {
        const TArray<FString> Problems = DataTable->CreateTableFromCSVString(CSVText);
        if (Problems.Num() > 0)
        {
            UE_LOG(LogTemp, Error, TEXT("ParseCSVToDataTable - CSV parsing failed with %d problems:"), Problems.Num());
            for (const FString& Problem : Problems)
            {
                UE_LOG(LogTemp, Error, TEXT("  - %s"), *Problem);
            }
            return false;
        }
    }
#else
    if (!DataTable->CreateTableFromCSVString(CSVText))
    {
        UE_LOG(LogTemp, Error, TEXT("ParseCSVToDataTable - CreateTableFromCSVString returned false"));
        return false;
    }
#endif

    OutDataTable = DataTable;
    UE_LOG(LogTemp, Log, TEXT("ParseCSVToDataTable - Success! Created DataTable with %d rows"), DataTable->GetRowNames().Num());
    return true;
}

AActor* UChartSpawnLibrary::SpawnChartFromCSV(UObject* WorldContextObject, EChartType ChartType, const FString& CSVText, const FTransform& SpawnTransform)
{
    if (!WorldContextObject)
    {
        UE_LOG(LogTemp, Error, TEXT("SpawnChartFromCSV - WorldContextObject is null"));
        return nullptr;
    }

    UWorld* World = WorldContextObject->GetWorld();
    if (!World)
    {
        UE_LOG(LogTemp, Error, TEXT("SpawnChartFromCSV - World is null"));
        return nullptr;
    }

    UScriptStruct* RowStruct = nullptr;
    switch (ChartType)
    {
    case EChartType::Bar: RowStruct = FVRBarData::StaticStruct(); break;
    case EChartType::Line: RowStruct = FVRLineData::StaticStruct(); break;
    case EChartType::Scatter: RowStruct = FVRScatterData::StaticStruct(); break;
    default: 
        UE_LOG(LogTemp, Error, TEXT("SpawnChartFromCSV - Invalid ChartType: %d"), (int32)ChartType);
        return nullptr;
    }

    if (!RowStruct)
    {
        UE_LOG(LogTemp, Error, TEXT("SpawnChartFromCSV - RowStruct is null for ChartType: %d"), (int32)ChartType);
        return nullptr;
    }

    UDataTable* DataTable = nullptr;
    if (!ParseCSVToDataTable(CSVText, RowStruct, DataTable))
    {
        UE_LOG(LogTemp, Error, TEXT("SpawnChartFromCSV - ParseCSVToDataTable failed"));
        return nullptr;
    }

    if (!DataTable)
    {
        UE_LOG(LogTemp, Error, TEXT("SpawnChartFromCSV - DataTable is null after parsing"));
        return nullptr;
    }

    FActorSpawnParameters Params;
    AActor* Spawned = nullptr;

    switch (ChartType)
    {
    case EChartType::Bar:
        UE_LOG(LogTemp, Log, TEXT("SpawnChartFromCSV - Attempting to spawn BarChartActor"));
        if (ABarChartActor* Chart = World->SpawnActor<ABarChartActor>(ABarChartActor::StaticClass(), SpawnTransform, Params))
        {
            Chart->BarDataTable = DataTable;
            Chart->Rebuild();
            Spawned = Chart;
            UE_LOG(LogTemp, Warning, TEXT("SpawnChartFromCSV - Successfully spawned BarChartActor"));
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("SpawnChartFromCSV - Failed to spawn BarChartActor"));
        }
        break;
    case EChartType::Line:
        UE_LOG(LogTemp, Log, TEXT("SpawnChartFromCSV - Attempting to spawn LineGraphActor"));
        if (ALineGraphActor* Chart = World->SpawnActor<ALineGraphActor>(ALineGraphActor::StaticClass(), SpawnTransform, Params))
        { 
            Chart->LineDataTable = DataTable; 
            Chart->Rebuild();
            Spawned = Chart;
            UE_LOG(LogTemp, Warning, TEXT("SpawnChartFromCSV - Successfully spawned LineGraphActor"));
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("SpawnChartFromCSV - Failed to spawn LineGraphActor"));
        }
        break;
    case EChartType::Scatter:
        UE_LOG(LogTemp, Log, TEXT("SpawnChartFromCSV - Attempting to spawn ScatterActor"));
        if (AScatterActor* Chart = World->SpawnActor<AScatterActor>(AScatterActor::StaticClass(), SpawnTransform, Params))
        { 
            Chart->ScatterDataTable = DataTable; 
            Chart->Rebuild();
            Spawned = Chart;
            UE_LOG(LogTemp, Warning, TEXT("SpawnChartFromCSV - Successfully spawned ScatterActor"));
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("SpawnChartFromCSV - Failed to spawn ScatterActor"));
        }
        break;
    default: break;
    }

    return Spawned;
}



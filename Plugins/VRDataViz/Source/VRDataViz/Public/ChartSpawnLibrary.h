#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "Engine/DataTable.h"
#include "ChartSpawnLibrary.generated.h"

UENUM(BlueprintType)
enum class EChartType : uint8
{
    Bar     UMETA(DisplayName = "Bar Chart"),
    Line    UMETA(DisplayName = "Line Graph"),
    Scatter UMETA(DisplayName = "Scatter Plot")
};

UCLASS()
class VRDATAVIZ_API UChartSpawnLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Data|Charts")
    static bool ParseCSVToDataTable(const FString& CSVText, UScriptStruct* RowStruct, UDataTable*& OutDataTable);

    UFUNCTION(BlueprintCallable, Category = "Data|Charts")
    static AActor* SpawnChartFromCSV(UObject* WorldContextObject, EChartType ChartType, const FString& CSVText, const FTransform& SpawnTransform);
};


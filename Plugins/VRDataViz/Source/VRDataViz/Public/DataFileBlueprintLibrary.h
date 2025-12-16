#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "DataFileBlueprintLibrary.generated.h"

UCLASS()
class VRDATAVIZ_API UDataFileBlueprintLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Data|Files")
    static void GetCSVFiles(TArray<FString>& OutFilePaths, const FString& SubfolderName = TEXT("DataCharts"));

    UFUNCTION(BlueprintCallable, Category = "Data|Files")
    static bool LoadTextFile(const FString& FilePath, FString& OutText);
};


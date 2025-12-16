#include "DataFileBlueprintLibrary.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "HAL/FileManager.h"

void UDataFileBlueprintLibrary::GetCSVFiles(TArray<FString>& OutFilePaths, const FString& SubfolderName)
{
    OutFilePaths.Reset();

    const FString BaseDir = FPaths::ProjectSavedDir();
    const FString TargetDir = FPaths::Combine(BaseDir, SubfolderName);

    IFileManager& FileManager = IFileManager::Get();
    FString SearchPattern = FPaths::Combine(TargetDir, TEXT("*.csv"));

    TArray<FString> FoundFiles;
    FileManager.FindFiles(FoundFiles, *SearchPattern, true, false);

    OutFilePaths.Reset();
    for (const FString& FileName : FoundFiles)
    {
        OutFilePaths.Add(FPaths::Combine(TargetDir, FileName));
    }
}

bool UDataFileBlueprintLibrary::LoadTextFile(const FString& FilePath, FString& OutText)
{
    OutText.Reset();
    return FFileHelper::LoadFileToString(OutText, *FilePath);
}


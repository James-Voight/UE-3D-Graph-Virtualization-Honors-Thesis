#pragma once

#include "Blueprint/UserWidget.h"
#include "ChartSpawnLibrary.h"
#include "DataVizPanelWidget.generated.h"

UCLASS()
class VRDATAVIZ_API UDataVizPanelWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    virtual void NativeConstruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void NativeDestruct() override;

    UFUNCTION(BlueprintCallable, Category = "DataViz")
    void ListCSVFiles(TArray<FString>& OutFilePaths, const FString& SubfolderName = TEXT("DataCharts"));

    UFUNCTION(BlueprintCallable, Category = "DataViz")
    AActor* GenerateChart(EChartType ChartType, const FString& FilePath, const FTransform& SpawnTransform);

    UFUNCTION(BlueprintCallable, Category = "DataViz|Placement")
    void StartHoverPlacement();

    UFUNCTION(BlueprintCallable, Category = "DataViz|Placement")
    void StartXYZPlacement();

    UFUNCTION(BlueprintCallable, Category = "DataViz|Placement")
    bool UpdateHoverFromView(float Distance = 1000.0f);

    UFUNCTION(BlueprintCallable, Category = "DataViz|Placement")
    bool UpdateHoverFromVRController();

    UFUNCTION(BlueprintCallable, Category = "DataViz|Placement")
    bool IsPlacementActive() const { return PreviewChart != nullptr; }

    UFUNCTION(BlueprintCallable, Category = "DataViz|Placement")
    bool ConfirmPlacement(EChartType ChartType, const FString& FilePath);

    UFUNCTION(BlueprintCallable, Category = "DataViz|Placement")
    void CancelPlacement();

    UFUNCTION(BlueprintCallable, Category = "DataViz|Placement")
    void HandlePlacementInput(); // Called on keyboard click or VR trigger

    UFUNCTION(BlueprintCallable, Category = "DataViz|Preview")
    void SetPreviewRotation(float Yaw, float Pitch, float Roll);

    UFUNCTION(BlueprintCallable, Category = "DataViz|Preview")
    void SetPreviewScale(float Scale);

    UFUNCTION(BlueprintCallable, Category = "DataViz|Preview")
    bool IsVRMode() const;

private:
    UPROPERTY() class APlacementManager* PlacementMgr;
    UPROPERTY() class AActor* PreviewChart; // Actual chart actor for preview
    UPROPERTY() FString PendingFile;
    UPROPERTY() EChartType PendingType;
    UPROPERTY() bool bUseXYZPlacement = false;
    UPROPERTY() FRotator PreviewRotation = FRotator::ZeroRotator;
    UPROPERTY() float PreviewScale = 1.0f;

    // Runtime-created UMG controls
    UPROPERTY() class UComboBoxString* FileCombo;
    UPROPERTY() class UComboBoxString* ChartTypeCombo;
    UPROPERTY() class UEditableTextBox* XBox;
    UPROPERTY() class UEditableTextBox* YBox;
    UPROPERTY() class UEditableTextBox* ZBox;
    UPROPERTY() class UButton* RefreshBtn;
    UPROPERTY() class UButton* PlaceBtn;
    UPROPERTY() class UButton* PlaceXYZBtn;
    UPROPERTY() class UButton* ConfirmBtn;
    UPROPERTY() class UButton* CancelBtn;
    UPROPERTY() class USlider* RotationYawSlider;
    UPROPERTY() class USlider* RotationPitchSlider;
    UPROPERTY() class USlider* RotationRollSlider;
    UPROPERTY() class USlider* ScaleSlider;

    UFUNCTION()
    void RefreshFiles();
    UFUNCTION()
    void OnChartTypeChanged(FString Selected, ESelectInfo::Type SelectionType);
    UFUNCTION()
    void OnPlace();
    UFUNCTION()
    void OnPlaceXYZ();
    UFUNCTION()
    void OnConfirm();
    UFUNCTION()
    void OnCancel();
    UFUNCTION()
    void OnRotationYawChanged(float Value);
    UFUNCTION()
    void OnRotationPitchChanged(float Value);
    UFUNCTION()
    void OnRotationRollChanged(float Value);
    UFUNCTION()
    void OnScaleChanged(float Value);
    FTransform BuildTransformFromInputs() const;
    void UpdatePreviewTransform();
    AActor* CreatePreviewChart();
};


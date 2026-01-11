#include "DataVizPanelWidget.h"
#include "DataFileBlueprintLibrary.h"
#include "PlacementManager.h"
#include "Charts/PreviewPlacementActor.h"
#include "Charts/ScatterActor.h"
#include "Charts/LineGraphActor.h"
#include "Charts/BarChartActor.h"
#include "Charts/GridLineActor.h"
#include "Kismet/GameplayStatics.h"
#include "Blueprint/WidgetTree.h"
#include "Components/VerticalBox.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ComboBoxString.h"
#include "Components/EditableTextBox.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/Slider.h"
#include "Components/SizeBox.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Widgets/SWidget.h"
#include "IXRTrackingSystem.h"
#include "IHeadMountedDisplay.h"
#include "HeadMountedDisplayTypes.h"
#include "Framework/Application/SlateApplication.h"
#include "Input/Events.h"
#include "InputCoreTypes.h"
#include "EngineUtils.h"

void UDataVizPanelWidget::ListCSVFiles(TArray<FString>& OutFilePaths, const FString& SubfolderName)
{
    UDataFileBlueprintLibrary::GetCSVFiles(OutFilePaths, SubfolderName);
}

AActor* UDataVizPanelWidget::GenerateChart(EChartType ChartType, const FString& FilePath, const FTransform& SpawnTransform)
{
    FString Text;
    if (!UDataFileBlueprintLibrary::LoadTextFile(FilePath, Text))
    {
        return nullptr;
    }

    return UChartSpawnLibrary::SpawnChartFromCSV(this, ChartType, Text, SpawnTransform);
}

void UDataVizPanelWidget::NativeConstruct()
{
    Super::NativeConstruct();

    // Make sure the widget itself can receive input
    SetIsEnabled(true);

    UE_LOG(LogTemp, Log, TEXT("DataVizPanelWidget::NativeConstruct - Widget constructed: %s"),
        *GetName());
}

void UDataVizPanelWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);
    
    // Handle keyboard input
    HandleKeyboardInput();
    
    // Continuously update preview position while placement is active
    if (IsPlacementActive())
    {
        if (bUseXYZPlacement)
        {
            // Update location from XYZ text boxes
            const FTransform T = BuildTransformFromInputs();
            if (PreviewChart)
            {
                PreviewChart->SetActorLocation(T.GetLocation());
                UpdatePreviewTransform();
            }
        }
        else if (IsVRMode())
        {
            UpdateHoverFromVRController();
        }
        else
        {
            UpdateHoverFromView(1000.0f);
        }
    }
}

TSharedRef<SWidget> UDataVizPanelWidget::RebuildWidget()
{
    WidgetTree->RootWidget = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
    UVerticalBox* VBox = Cast<UVerticalBox>(WidgetTree->RootWidget);

    FileCombo = WidgetTree->ConstructWidget<UComboBoxString>(UComboBoxString::StaticClass());
    ChartTypeCombo = WidgetTree->ConstructWidget<UComboBoxString>(UComboBoxString::StaticClass());
    XBox = WidgetTree->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass());
    YBox = WidgetTree->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass());
    ZBox = WidgetTree->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass());
    RefreshBtn = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
    PlaceBtn = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
    PlaceXYZBtn = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
    ConfirmBtn = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
    CancelBtn = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
    
    // Rotation and Scale sliders
    RotationYawSlider = WidgetTree->ConstructWidget<USlider>(USlider::StaticClass());
    RotationPitchSlider = WidgetTree->ConstructWidget<USlider>(USlider::StaticClass());
    RotationRollSlider = WidgetTree->ConstructWidget<USlider>(USlider::StaticClass());
    ScaleSlider = WidgetTree->ConstructWidget<USlider>(USlider::StaticClass());
    ScaleXSlider = WidgetTree->ConstructWidget<USlider>(USlider::StaticClass());
    ScaleYSlider = WidgetTree->ConstructWidget<USlider>(USlider::StaticClass());
    ScaleZSlider = WidgetTree->ConstructWidget<USlider>(USlider::StaticClass());
    PointScaleSlider = WidgetTree->ConstructWidget<USlider>(USlider::StaticClass());
    TextScaleSlider = WidgetTree->ConstructWidget<USlider>(USlider::StaticClass());
    UniformScaleToggleBtn = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());

    RotationYawSlider->SetMinValue(0.0f);
    RotationYawSlider->SetMaxValue(360.0f);
    RotationYawSlider->SetValue(0.0f);
    RotationPitchSlider->SetMinValue(0.0f);
    RotationPitchSlider->SetMaxValue(360.0f);
    RotationPitchSlider->SetValue(0.0f);
    RotationRollSlider->SetMinValue(0.0f);
    RotationRollSlider->SetMaxValue(360.0f);
    RotationRollSlider->SetValue(0.0f);
    ScaleSlider->SetMinValue(0.01f);
    ScaleSlider->SetMaxValue(100.0f);
    ScaleSlider->SetValue(1.0f);
    ScaleXSlider->SetMinValue(0.01f);
    ScaleXSlider->SetMaxValue(100.0f);
    ScaleXSlider->SetValue(1.0f);
    ScaleYSlider->SetMinValue(0.01f);
    ScaleYSlider->SetMaxValue(100.0f);
    ScaleYSlider->SetValue(1.0f);
    ScaleZSlider->SetMinValue(0.01f);
    ScaleZSlider->SetMaxValue(100.0f);
    ScaleZSlider->SetValue(1.0f);
    PointScaleSlider->SetMinValue(0.05f);
    PointScaleSlider->SetMaxValue(5.0f);
    PointScaleSlider->SetValue(PreviewPointScale);
    TextScaleSlider->SetMinValue(0.1f);
    TextScaleSlider->SetMaxValue(4.0f);
    TextScaleSlider->SetValue(PreviewTextScale);

    RotationYawSlider->OnValueChanged.AddDynamic(this, &UDataVizPanelWidget::OnRotationYawChanged);
    RotationPitchSlider->OnValueChanged.AddDynamic(this, &UDataVizPanelWidget::OnRotationPitchChanged);
    RotationRollSlider->OnValueChanged.AddDynamic(this, &UDataVizPanelWidget::OnRotationRollChanged);
    ScaleSlider->OnValueChanged.AddDynamic(this, &UDataVizPanelWidget::OnScaleChanged);
    ScaleXSlider->OnValueChanged.AddDynamic(this, &UDataVizPanelWidget::OnScaleXChanged);
    ScaleYSlider->OnValueChanged.AddDynamic(this, &UDataVizPanelWidget::OnScaleYChanged);
    ScaleZSlider->OnValueChanged.AddDynamic(this, &UDataVizPanelWidget::OnScaleZChanged);
    PointScaleSlider->OnValueChanged.AddDynamic(this, &UDataVizPanelWidget::OnPointScaleChanged);
    TextScaleSlider->OnValueChanged.AddDynamic(this, &UDataVizPanelWidget::OnTextScaleChanged);

    // --- File selection row ---
    UTextBlock* FileLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
    FileLabel->SetText(FText::FromString(TEXT("CSV File")));
    VBox->AddChildToVerticalBox(FileLabel);
    VBox->AddChildToVerticalBox(FileCombo);

    // --- Chart type row ---
    UTextBlock* ChartLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
    ChartLabel->SetText(FText::FromString(TEXT("Chart Type")));
    VBox->AddChildToVerticalBox(ChartLabel);
    VBox->AddChildToVerticalBox(ChartTypeCombo);

    // --- Position input rows (X/Y/Z) ---
    auto MakeLabeledTextRow = [this, VBox](const FString& Label, UEditableTextBox* TextBox)
    {
        UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());

        UTextBlock* LabelWidget = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
        LabelWidget->SetText(FText::FromString(Label));

        Row->AddChildToHorizontalBox(LabelWidget);
        Row->AddChildToHorizontalBox(TextBox);

        VBox->AddChildToVerticalBox(Row);
    };

    MakeLabeledTextRow(TEXT("X Position"), XBox);
    MakeLabeledTextRow(TEXT("Y Position"), YBox);
    MakeLabeledTextRow(TEXT("Z Position"), ZBox);

    // --- Rotation controls ---
    auto MakeLabeledSliderRow = [this, VBox](const FString& Label, USlider* Slider) -> UHorizontalBox*
    {
        UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
        UTextBlock* LabelWidget = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
        LabelWidget->SetText(FText::FromString(Label));
        
        // Wrap label in SizeBox to ensure consistent width across all labels
        USizeBox* LabelSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
        LabelSizeBox->SetMinDesiredWidth(150.0f); // Fixed width for all labels
        LabelSizeBox->AddChild(LabelWidget);
        
        // Add label SizeBox to row
        UHorizontalBoxSlot* LabelSlot = Row->AddChildToHorizontalBox(LabelSizeBox);
        if (LabelSlot)
        {
            LabelSlot->SetVerticalAlignment(VAlign_Center);
            LabelSlot->SetPadding(FMargin(0.0f, 0.0f, 10.0f, 0.0f)); // Right padding
        }
        
        // Wrap slider in SizeBox to force minimum width
        USizeBox* SliderSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
        SliderSizeBox->SetMinDesiredWidth(600.0f); // Force slider to be at least 600 pixels wide
        SliderSizeBox->SetMinDesiredHeight(40.0f); // Make it taller too
        SliderSizeBox->AddChild(Slider);
        
        // Add SizeBox (containing slider) to row
        UHorizontalBoxSlot* SliderSlot = Row->AddChildToHorizontalBox(SliderSizeBox);
        if (SliderSlot)
        {
            SliderSlot->SetVerticalAlignment(VAlign_Center);
            SliderSlot->SetPadding(FMargin(0.0f, 10.0f, 0.0f, 10.0f)); // Vertical padding
        }
        
        VBox->AddChildToVerticalBox(Row);
        return Row;
    };

    MakeLabeledSliderRow(TEXT("Rotation Yaw"), RotationYawSlider);
    MakeLabeledSliderRow(TEXT("Rotation Pitch"), RotationPitchSlider);
    MakeLabeledSliderRow(TEXT("Rotation Roll"), RotationRollSlider);
    MakeLabeledSliderRow(TEXT("Scale (Uniform)"), ScaleSlider);
    MakeLabeledSliderRow(TEXT("Scale X"), ScaleXSlider);
    MakeLabeledSliderRow(TEXT("Scale Y"), ScaleYSlider);
    MakeLabeledSliderRow(TEXT("Scale Z"), ScaleZSlider);
    PointScaleRow = MakeLabeledSliderRow(TEXT("Point Scale"), PointScaleSlider);
    MakeLabeledSliderRow(TEXT("Text Scale"), TextScaleSlider);
    
    // Uniform scale toggle button
    UTextBlock* UniformToggleLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
    UniformToggleLabel->SetText(FText::FromString(TEXT("Uniform Scale: ON")));
    UniformScaleToggleBtn->AddChild(UniformToggleLabel);
    UniformScaleToggleBtn->OnClicked.AddDynamic(this, &UDataVizPanelWidget::OnUniformScaleToggle);
    VBox->AddChildToVerticalBox(UniformScaleToggleBtn);

    // --- Buttons with text labels ---
    auto MakeLabeledButton = [this, VBox](const FString& Label, UButton*& OutButton)
    {
        OutButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());

        UTextBlock* ButtonLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
        ButtonLabel->SetText(FText::FromString(Label));

        OutButton->AddChild(ButtonLabel);
        VBox->AddChildToVerticalBox(OutButton);
    };

    MakeLabeledButton(TEXT("Refresh Files"), RefreshBtn);
    MakeLabeledButton(TEXT("Start Placement (Camera)"), PlaceBtn);
    MakeLabeledButton(TEXT("Start Placement (XYZ)"), PlaceXYZBtn);
    MakeLabeledButton(TEXT("Confirm Placement"), ConfirmBtn);
    MakeLabeledButton(TEXT("Cancel Placement"), CancelBtn);

    ChartTypeCombo->AddOption(TEXT("Bar"));
    ChartTypeCombo->AddOption(TEXT("Line"));
    ChartTypeCombo->AddOption(TEXT("Scatter"));
    ChartTypeCombo->OnSelectionChanged.AddDynamic(this, &UDataVizPanelWidget::OnChartTypeChanged);
    ChartTypeCombo->SetSelectedOption(TEXT("Scatter"));
    OnChartTypeChanged(TEXT("Scatter"), ESelectInfo::Direct);

    XBox->SetText(FText::FromString(TEXT("0")));
    YBox->SetText(FText::FromString(TEXT("0")));
    ZBox->SetText(FText::FromString(TEXT("0")));

    RefreshBtn->OnClicked.AddDynamic(this, &UDataVizPanelWidget::RefreshFiles);
    PlaceBtn->OnClicked.AddDynamic(this, &UDataVizPanelWidget::OnPlace);
    PlaceXYZBtn->OnClicked.AddDynamic(this, &UDataVizPanelWidget::OnPlaceXYZ);
    ConfirmBtn->OnClicked.AddDynamic(this, &UDataVizPanelWidget::OnConfirm);
    CancelBtn->OnClicked.AddDynamic(this, &UDataVizPanelWidget::OnCancel);

    RefreshFiles();

    return WidgetTree->RootWidget->TakeWidget();
}

void UDataVizPanelWidget::NativeDestruct()
{
    CancelPlacement();
    Super::NativeDestruct();
}

void UDataVizPanelWidget::RefreshFiles()
{
    TArray<FString> Files; ListCSVFiles(Files, TEXT("DataCharts"));
    if (FileCombo)
    {
        FileCombo->ClearOptions();
        for (const FString& P : Files) { FileCombo->AddOption(P); }
        if (Files.Num() > 0) { FileCombo->SetSelectedOption(Files[0]); }
    }
}

void UDataVizPanelWidget::OnChartTypeChanged(FString Selected, ESelectInfo::Type)
{
    const bool bIsBar = (Selected == TEXT("Bar"));
    const bool bIsLine = (Selected == TEXT("Line"));
    const bool bIsScatter = (Selected == TEXT("Scatter"));
    if (PointScaleRow)
    {
        PointScaleRow->SetVisibility((bIsLine || bIsScatter) ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    }
}

void UDataVizPanelWidget::OnPlace()
{
    UE_LOG(LogTemp, Warning, TEXT("DataVizPanelWidget::OnPlace - Start Placement (Camera) clicked"));
    
    // Error checking: Make sure file is selected
    const FString FilePath = FileCombo ? FileCombo->GetSelectedOption() : FString();
    if (FilePath.IsEmpty())
    {
        UE_LOG(LogTemp, Error, TEXT("OnPlace - No file selected! Please select a CSV file first."));
        return;
    }
    
    // Cancel any existing placement
    if (IsPlacementActive())
    {
        CancelPlacement();
    }
    
    bUseXYZPlacement = false;
    StartHoverPlacement();
}

void UDataVizPanelWidget::OnPlaceXYZ()
{
    UE_LOG(LogTemp, Warning, TEXT("DataVizPanelWidget::OnPlaceXYZ - Start Placement (XYZ) clicked"));
    
    // Error checking: Make sure file is selected
    const FString FilePath = FileCombo ? FileCombo->GetSelectedOption() : FString();
    if (FilePath.IsEmpty())
    {
        UE_LOG(LogTemp, Error, TEXT("OnPlaceXYZ - No file selected! Please select a CSV file first."));
        return;
    }
    
    // Cancel any existing placement
    if (IsPlacementActive())
    {
        CancelPlacement();
    }
    
    bUseXYZPlacement = true;
    StartXYZPlacement();
}

void UDataVizPanelWidget::OnConfirm()
{
    UE_LOG(LogTemp, Warning, TEXT("OnConfirm - Confirm Placement clicked"));
    
    // Error checking: Make sure placement is active
    if (!IsPlacementActive())
    {
        UE_LOG(LogTemp, Warning, TEXT("OnConfirm - No active placement! Please click 'Start Placement' first."));
        return;
    }
    
    // Error checking: Make sure preview chart exists
    if (!PreviewChart || !IsValid(PreviewChart))
    {
        UE_LOG(LogTemp, Error, TEXT("OnConfirm - No Preview chart! Please click 'Start Placement' first."));
        return;
    }
    
    // Error checking: Make sure file is selected
    const FString FilePath = FileCombo ? FileCombo->GetSelectedOption() : FString();
    if (FilePath.IsEmpty())
    {
        UE_LOG(LogTemp, Error, TEXT("OnConfirm - No file selected! Please select a CSV file."));
        return;
    }
    
    EChartType Type = EChartType::Scatter;
    if (ChartTypeCombo)
    {
        const FString Sel = ChartTypeCombo->GetSelectedOption();
        if (Sel == TEXT("Bar")) Type = EChartType::Bar; 
        else if (Sel == TEXT("Line")) Type = EChartType::Line; 
        else Type = EChartType::Scatter;
    }
    
    UE_LOG(LogTemp, Log, TEXT("OnConfirm - FilePath: %s, ChartType: %d"), *FilePath, (int32)Type);
    
    bool bSuccess = ConfirmPlacement(Type, FilePath);
    UE_LOG(LogTemp, Warning, TEXT("OnConfirm - ConfirmPlacement returned: %s"), bSuccess ? TEXT("SUCCESS") : TEXT("FAILED"));
}

void UDataVizPanelWidget::OnCancel()
{
    UE_LOG(LogTemp, Warning, TEXT("OnCancel - Cancel Placement button clicked!"));
    CancelPlacement();
}

void UDataVizPanelWidget::OnRotationYawChanged(float Value)
{
    PreviewRotation.Yaw = Value;
    if (IsPlacementActive())
    {
        UpdatePreviewTransform();
    }
}

void UDataVizPanelWidget::OnRotationPitchChanged(float Value)
{
    PreviewRotation.Pitch = Value;
    if (IsPlacementActive())
    {
        UpdatePreviewTransform();
    }
}

void UDataVizPanelWidget::OnRotationRollChanged(float Value)
{
    PreviewRotation.Roll = Value;
    if (IsPlacementActive())
    {
        UpdatePreviewTransform();
    }
}

void UDataVizPanelWidget::OnScaleChanged(float Value)
{
    PreviewScale = Value; // No clamping - allow any scale
    if (bUniformScale)
    {
        PreviewScaleXYZ = FVector(Value);
        if (ScaleXSlider) ScaleXSlider->SetValue(Value);
        if (ScaleYSlider) ScaleYSlider->SetValue(Value);
        if (ScaleZSlider) ScaleZSlider->SetValue(Value);
    }
    if (IsPlacementActive())
    {
        UpdatePreviewTransform();
    }
}

void UDataVizPanelWidget::OnScaleXChanged(float Value)
{
    PreviewScaleXYZ.X = Value;
    if (bUniformScale)
    {
        PreviewScaleXYZ = FVector(Value);
        PreviewScale = Value;
        if (ScaleSlider) ScaleSlider->SetValue(Value);
        if (ScaleYSlider) ScaleYSlider->SetValue(Value);
        if (ScaleZSlider) ScaleZSlider->SetValue(Value);
    }
    if (IsPlacementActive())
    {
        UpdatePreviewTransform();
    }
}

void UDataVizPanelWidget::OnScaleYChanged(float Value)
{
    PreviewScaleXYZ.Y = Value;
    if (bUniformScale)
    {
        PreviewScaleXYZ = FVector(Value);
        PreviewScale = Value;
        if (ScaleSlider) ScaleSlider->SetValue(Value);
        if (ScaleXSlider) ScaleXSlider->SetValue(Value);
        if (ScaleZSlider) ScaleZSlider->SetValue(Value);
    }
    if (IsPlacementActive())
    {
        UpdatePreviewTransform();
    }
}

void UDataVizPanelWidget::OnScaleZChanged(float Value)
{
    PreviewScaleXYZ.Z = Value;
    if (bUniformScale)
    {
        PreviewScaleXYZ = FVector(Value);
        PreviewScale = Value;
        if (ScaleSlider) ScaleSlider->SetValue(Value);
        if (ScaleXSlider) ScaleXSlider->SetValue(Value);
        if (ScaleYSlider) ScaleYSlider->SetValue(Value);
    }
    if (IsPlacementActive())
    {
        UpdatePreviewTransform();
    }
}

void UDataVizPanelWidget::OnPointScaleChanged(float Value)
{
    PreviewPointScale = Value;
    bPreviewGeometryDirty = true;
    if (IsPlacementActive())
    {
        UpdatePreviewTransform();
    }
}

void UDataVizPanelWidget::OnTextScaleChanged(float Value)
{
    PreviewTextScale = Value;
    bPreviewGeometryDirty = true;
    if (IsPlacementActive())
    {
        UpdatePreviewTransform();
    }
}

void UDataVizPanelWidget::OnUniformScaleToggle()
{
    bUniformScale = !bUniformScale;
    if (UniformScaleToggleBtn)
    {
        TArray<UWidget*> Children = UniformScaleToggleBtn->GetAllChildren();
        for (UWidget* Child : Children)
        {
            if (UTextBlock* TextBlock = Cast<UTextBlock>(Child))
            {
                TextBlock->SetText(FText::FromString(bUniformScale ? TEXT("Uniform Scale: ON") : TEXT("Uniform Scale: OFF")));
                break;
            }
        }
    }
    if (bUniformScale)
    {
        // Sync all scales to the uniform value
        float UniformVal = PreviewScale;
        PreviewScaleXYZ = FVector(UniformVal);
        if (ScaleXSlider) ScaleXSlider->SetValue(UniformVal);
        if (ScaleYSlider) ScaleYSlider->SetValue(UniformVal);
        if (ScaleZSlider) ScaleZSlider->SetValue(UniformVal);
    }
    if (IsPlacementActive())
    {
        UpdatePreviewTransform();
    }
}

bool UDataVizPanelWidget::IsVRMode() const
{
    if (GEngine && GEngine->XRSystem.IsValid())
    {
        return GEngine->XRSystem->IsHeadTrackingAllowed();
    }
    return false;
}

FTransform UDataVizPanelWidget::BuildTransformFromInputs() const
{
    const float X = XBox ? FCString::Atof(*XBox->GetText().ToString()) : 0.f;
    const float Y = YBox ? FCString::Atof(*YBox->GetText().ToString()) : 0.f;
    const float Z = ZBox ? FCString::Atof(*ZBox->GetText().ToString()) : 0.f;
    return FTransform(FRotator::ZeroRotator, FVector(X, Y, Z));
}

void UDataVizPanelWidget::StartHoverPlacement()
{
    CancelPlacement(); // Clear any existing preview
    
    UWorld* World = GetWorld();
    if (!World)
    {
        UE_LOG(LogTemp, Warning, TEXT("StartHoverPlacement - No World"));
        return;
    }
    
    const FString FilePath = FileCombo ? FileCombo->GetSelectedOption() : FString();
    if (FilePath.IsEmpty())
    {
        UE_LOG(LogTemp, Error, TEXT("StartHoverPlacement - No file selected"));
        return;
    }
    
    EChartType Type = EChartType::Scatter;
    if (ChartTypeCombo)
    {
        const FString Sel = ChartTypeCombo->GetSelectedOption();
        if (Sel == TEXT("Bar")) Type = EChartType::Bar;
        else if (Sel == TEXT("Line")) Type = EChartType::Line;
        else Type = EChartType::Scatter;
    }
    
    PendingFile = FilePath;
    PendingType = Type;
    bCancelRequested = false;
    bCancelRequested = false; // Reset cancel flag
    
    // Only create preview if one doesn't already exist
    if (!PreviewChart || !IsValid(PreviewChart))
    {
        // Create actual chart as preview
        PreviewChart = CreatePreviewChart();
        if (!PreviewChart || !IsValid(PreviewChart))
        {
            UE_LOG(LogTemp, Error, TEXT("StartHoverPlacement - Failed to create preview chart"));
            return;
        }
        bPreviewGeometryDirty = true;
    }
    
    // Initialize position
    APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0);
    if (PC && PC->PlayerCameraManager)
    {
        const FVector CamLoc = PC->PlayerCameraManager->GetCameraLocation();
        const FVector CamDir = PC->PlayerCameraManager->GetCameraRotation().Vector();
        const FVector InitialPos = CamLoc + CamDir * 500.0f;
        
        if (IsValid(PreviewChart))
        {
            PreviewChart->SetActorLocation(InitialPos);
            UpdatePreviewTransform();
        }
    }
    else
    {
        // Fallback: place at origin if no camera
        if (IsValid(PreviewChart))
        {
            PreviewChart->SetActorLocation(FVector::ZeroVector);
            UpdatePreviewTransform();
        }
    }
    
    // Create placement manager if it doesn't exist
    if (!PlacementMgr || !IsValid(PlacementMgr))
    {
        FActorSpawnParameters Params;
        Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        PlacementMgr = World->SpawnActor<APlacementManager>(APlacementManager::StaticClass(), FTransform::Identity, Params);
        if (!PlacementMgr)
        {
            UE_LOG(LogTemp, Error, TEXT("StartHoverPlacement - Failed to spawn PlacementManager"));
            // Don't return - we can still work without it, just won't have trace placement
        }
    }
    
    UE_LOG(LogTemp, Log, TEXT("StartHoverPlacement - Preview chart created, ready for placement"));
}

void UDataVizPanelWidget::StartXYZPlacement()
{
    CancelPlacement();
    
    UWorld* World = GetWorld();
    if (!World) return;
    
    const FString FilePath = FileCombo ? FileCombo->GetSelectedOption() : FString();
    if (FilePath.IsEmpty())
    {
        UE_LOG(LogTemp, Error, TEXT("StartXYZPlacement - No file selected"));
        return;
    }
    
    EChartType Type = EChartType::Scatter;
    if (ChartTypeCombo)
    {
        const FString Sel = ChartTypeCombo->GetSelectedOption();
        if (Sel == TEXT("Bar")) Type = EChartType::Bar;
        else if (Sel == TEXT("Line")) Type = EChartType::Line;
        else Type = EChartType::Scatter;
    }
    
    PendingFile = FilePath;
    PendingType = Type;
    
    PreviewChart = CreatePreviewChart();
    if (!PreviewChart || !IsValid(PreviewChart))
    {
        UE_LOG(LogTemp, Error, TEXT("StartXYZPlacement - Failed to create preview chart"));
        return;
    }
    bPreviewGeometryDirty = true;
    
    // Use XYZ coordinates directly
    const FTransform T = BuildTransformFromInputs();
    if (IsValid(PreviewChart))
    {
        PreviewChart->SetActorTransform(T);
        UpdatePreviewTransform();
        UE_LOG(LogTemp, Log, TEXT("StartXYZPlacement - Preview chart created at XYZ: %s"), *T.GetLocation().ToString());
    }
}

bool UDataVizPanelWidget::UpdateHoverFromView(float Distance)
{
    // Only update if preview exists and is valid - don't create new ones
    if (!PreviewChart || !IsValid(PreviewChart) || bUseXYZPlacement || bCancelRequested) return false;
    
    UWorld* World = GetWorld();
    if (!World || !PlacementMgr || !IsValid(PlacementMgr))
    {
        return false;
    }
    
    APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0);
    if (!PC || !PC->PlayerCameraManager)
    {
        return false;
    }
    
    const FVector Start = PC->PlayerCameraManager->GetCameraLocation();
    const FVector Dir = PC->PlayerCameraManager->GetCameraRotation().Vector();
    FVector Loc, Normal;
    
    // Try to trace to a surface
    if (PlacementMgr->TracePlacementLocation(Start, Dir, Loc, Normal))
    {
        PreviewChart->SetActorLocation(Loc);
        PreviewChart->SetActorRotation(Normal.ToOrientationRotator());
        UpdatePreviewTransform();
        return true;
    }
    else
    {
        // Fallback: place in front of camera at default distance
        const FVector FallbackPos = Start + Dir * Distance;
        PreviewChart->SetActorLocation(FallbackPos);
        PreviewChart->SetActorRotation(Dir.ToOrientationRotator());
        UpdatePreviewTransform();
        return true;
    }
}

bool UDataVizPanelWidget::UpdateHoverFromVRController()
{
    if (!PreviewChart || !IsValid(PreviewChart) || !IsVRMode() || bUseXYZPlacement) return false;
    
    UWorld* World = GetWorld();
    if (!World || !GEngine || !GEngine->XRSystem.IsValid()) return false;
    
    // Get VR controller transform
    FVector ControllerLocation;
    FQuat ControllerQuat;
    bool bHasController = false;
    
    // Try to get controller 0 (right hand) transform
    int32 ControllerId = 0; // Right hand controller
    if (GEngine->XRSystem->GetCurrentPose(ControllerId, ControllerQuat, ControllerLocation))
    {
        bHasController = true;
    }
    else
    {
        // Fallback: try to get HMD position and use forward direction
        FVector HMDPosition;
        FQuat HMDQuat;
        if (GEngine->XRSystem->GetCurrentPose(IXRTrackingSystem::HMDDeviceId, HMDQuat, HMDPosition))
        {
            ControllerLocation = HMDPosition;
            ControllerQuat = HMDQuat;
            bHasController = true;
        }
    }
    
    if (bHasController)
    {
        FRotator ControllerRotation = ControllerQuat.Rotator();
        // Trace forward from controller
        const FVector Forward = ControllerRotation.Vector();
        const FVector End = ControllerLocation + Forward * 1000.0f;
        
        FHitResult Hit;
        FCollisionQueryParams Params;
        if (World->LineTraceSingleByChannel(Hit, ControllerLocation, End, ECC_Visibility, Params))
        {
            PreviewChart->SetActorLocation(Hit.ImpactPoint);
            PreviewChart->SetActorRotation(Hit.ImpactNormal.ToOrientationRotator());
        }
        else
        {
            // Place in front of controller
            PreviewChart->SetActorLocation(ControllerLocation + Forward * 500.0f);
            PreviewChart->SetActorRotation(ControllerRotation);
        }
        UpdatePreviewTransform();
        return true;
    }
    
    return false;
}

bool UDataVizPanelWidget::ConfirmPlacement(EChartType ChartType, const FString& FilePath)
{
    // Don't confirm if cancel was requested
    if (bCancelRequested)
    {
        UE_LOG(LogTemp, Warning, TEXT("ConfirmPlacement - Cancel was requested, aborting confirmation"));
        return false;
    }
    
    if (!PreviewChart)
    {
        UE_LOG(LogTemp, Error, TEXT("ConfirmPlacement - No Preview chart"));
        return false;
    }
    
    // Apply final transform with rotation and scale
    UpdatePreviewTransform();
    
    // Remove preview transparency
    TArray<UActorComponent*> Components;
    PreviewChart->GetComponents(Components);
    for (UActorComponent* Comp : Components)
    {
        if (UStaticMeshComponent* MeshComp = Cast<UStaticMeshComponent>(Comp))
        {
            if (UMaterialInterface* Mat = MeshComp->GetMaterial(0))
            {
                UMaterialInstanceDynamic* DynMat = MeshComp->CreateDynamicMaterialInstance(0, Mat);
                if (DynMat)
                {
                    DynMat->SetScalarParameterValue(TEXT("Opacity"), 1.0f);
                }
            }
        }
    }
    
    // Mark the chart as persistent so it saves with the level
    if (PreviewChart)
    {
#if WITH_EDITOR
        PreviewChart->SetActorLabel(FString::Printf(TEXT("Chart_%s_%d"), ChartType == EChartType::Bar ? TEXT("Bar") : (ChartType == EChartType::Line ? TEXT("Line") : TEXT("Scatter")), FMath::RandRange(1000, 9999)));
#endif
        // Make sure it's marked for save - not editor-only
        PreviewChart->bIsEditorOnlyActor = false;
    }
    
    // Clear preview state but keep the chart (it's now the actual placed chart)
    AActor* PlacedChart = PreviewChart;
    PreviewChart = nullptr;
    if (PlacementMgr) { PlacementMgr->Destroy(); PlacementMgr = nullptr; }
    if (VisualGuideActor && IsValid(VisualGuideActor))
    {
        VisualGuideActor->Destroy();
        VisualGuideActor = nullptr;
    }
    bXAxisMode = false;
    bYAxisMode = false;
    bZAxisMode = false;
    bGrabMode = false;
    bUseXYZPlacement = false;
    bCancelRequested = false;
    
    UE_LOG(LogTemp, Warning, TEXT("ConfirmPlacement - SUCCESS! Chart placed: %s"), PlacedChart ? *PlacedChart->GetName() : TEXT("NULL"));
    return true;
}

void UDataVizPanelWidget::HandlePlacementInput()
{
    // Called on keyboard click or VR trigger - same as Confirm
    // BUT: Only confirm if placement is active AND preview chart exists AND cancel was not requested
    if (!IsPlacementActive() || bCancelRequested)
    {
        UE_LOG(LogTemp, Log, TEXT("HandlePlacementInput - Placement not active or cancel requested, ignoring input"));
        return;
    }
    
    if (!PreviewChart || !IsValid(PreviewChart))
    {
        UE_LOG(LogTemp, Log, TEXT("HandlePlacementInput - PreviewChart is null or invalid, ignoring input"));
        return;
    }
    
    UE_LOG(LogTemp, Log, TEXT("HandlePlacementInput - Confirming placement"));
    const FString FilePath = FileCombo ? FileCombo->GetSelectedOption() : FString();
    EChartType Type = EChartType::Scatter;
    if (ChartTypeCombo)
    {
        const FString Sel = ChartTypeCombo->GetSelectedOption();
        if (Sel == TEXT("Bar")) Type = EChartType::Bar;
        else if (Sel == TEXT("Line")) Type = EChartType::Line;
        else Type = EChartType::Scatter;
    }
    ConfirmPlacement(Type, FilePath);
}

void UDataVizPanelWidget::CancelPlacement()
{
    UE_LOG(LogTemp, Warning, TEXT("CancelPlacement - Starting cancellation"));
    
    // Set cancel flag first to prevent any confirmations
    bCancelRequested = true;
    
    // Destroy visual guide if it exists
    if (VisualGuideActor && IsValid(VisualGuideActor))
    {
        VisualGuideActor->Destroy();
        VisualGuideActor = nullptr;
    }
    
    // Reset keyboard shortcut states
    bXAxisMode = false;
    bYAxisMode = false;
    bZAxisMode = false;
    bGrabMode = false;
    
    // Destroy preview chart if it exists - this is critical to prevent it from being placed
    if (PreviewChart)
    {
        UE_LOG(LogTemp, Warning, TEXT("CancelPlacement - Destroying preview chart: %s"), IsValid(PreviewChart) ? *PreviewChart->GetName() : TEXT("INVALID"));
        if (IsValid(PreviewChart))
        {
            if (UWorld* World = GetWorld())
            {
                for (TActorIterator<AActor> It(World); It; ++It)
                {
                    AActor* Candidate = *It;
                    if (IsValid(Candidate) && Candidate->GetOwner() == PreviewChart)
                    {
                        Candidate->Destroy();
                    }
                }
            }

            TArray<AActor*> AttachedActors;
            PreviewChart->GetAttachedActors(AttachedActors);
            for (AActor* Attached : AttachedActors)
            {
                if (IsValid(Attached))
                {
                    Attached->Destroy();
                }
            }

            // Force destroy immediately - mark for destruction
            PreviewChart->Destroy();
        }
        PreviewChart = nullptr;
    }
    
    // Destroy placement manager if it exists
    if (PlacementMgr && IsValid(PlacementMgr))
    {
        UE_LOG(LogTemp, Warning, TEXT("CancelPlacement - Destroying placement manager"));
        PlacementMgr->Destroy();
        PlacementMgr = nullptr;
    }
    
    // Clear pending file
    PendingFile.Empty();
    bPreviewGeometryDirty = false;
    
    // Verify cancellation
    if (PreviewChart != nullptr)
    {
        UE_LOG(LogTemp, Error, TEXT("CancelPlacement - WARNING: PreviewChart still exists after cancellation! Force clearing."));
        PreviewChart = nullptr;
    }
    
    UE_LOG(LogTemp, Warning, TEXT("CancelPlacement - Cancellation complete. IsPlacementActive: %d"), IsPlacementActive() ? 1 : 0);
}

AActor* UDataVizPanelWidget::CreatePreviewChart()
{
    UWorld* World = GetWorld();
    if (!World)
    {
        UE_LOG(LogTemp, Error, TEXT("CreatePreviewChart - No World"));
        return nullptr;
    }
    
    if (PendingFile.IsEmpty())
    {
        UE_LOG(LogTemp, Error, TEXT("CreatePreviewChart - No file selected"));
        return nullptr;
    }
    
    FString Text;
    if (!UDataFileBlueprintLibrary::LoadTextFile(PendingFile, Text))
    {
        UE_LOG(LogTemp, Error, TEXT("CreatePreviewChart - Failed to load file: %s"), *PendingFile);
        return nullptr;
    }
    
    if (Text.IsEmpty())
    {
        UE_LOG(LogTemp, Error, TEXT("CreatePreviewChart - File is empty: %s"), *PendingFile);
        return nullptr;
    }
    
    // Determine initial spawn location
    FVector InitialLocation = FVector::ZeroVector;
    if (bUseXYZPlacement)
    {
        // Use XYZ coordinates from text boxes
        const FTransform T = BuildTransformFromInputs();
        InitialLocation = T.GetLocation();
    }
    else
    {
        // For camera placement, spawn in front of camera
        APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0);
        if (PC && PC->PlayerCameraManager)
        {
            const FVector CamLoc = PC->PlayerCameraManager->GetCameraLocation();
            const FVector CamDir = PC->PlayerCameraManager->GetCameraRotation().Vector();
            InitialLocation = CamLoc + CamDir * 500.0f;
        }
    }
    
    // Spawn unscaled/unrotated; graph actors will apply rotation/scale internally
    FTransform InitialTransform(FRotator::ZeroRotator, InitialLocation, FVector::OneVector);
    
    // Spawn chart with safe parameters
    AActor* Chart = UChartSpawnLibrary::SpawnChartFromCSV(this, PendingType, Text, InitialTransform);
    
    if (Chart && IsValid(Chart))
    {
        // Make it semi-transparent to indicate it's a preview
        TArray<UActorComponent*> Components;
        Chart->GetComponents(Components);
        for (UActorComponent* Comp : Components)
        {
            if (UStaticMeshComponent* MeshComp = Cast<UStaticMeshComponent>(Comp))
            {
                if (UMaterialInterface* Mat = MeshComp->GetMaterial(0))
                {
                    UMaterialInstanceDynamic* DynMat = MeshComp->CreateDynamicMaterialInstance(0, Mat);
                    if (DynMat)
                    {
                        DynMat->SetScalarParameterValue(TEXT("Opacity"), 0.7f);
                    }
                }
            }
        }
    }
    
    return Chart;
}

void UDataVizPanelWidget::UpdatePreviewTransform()
{
    if (!PreviewChart || !IsValid(PreviewChart))
    {
        return;
    }
    
    // Apply scale to the chart (no clamping - allow any scale)
    FVector FinalScale = bUniformScale ? FVector(PreviewScale) : PreviewScaleXYZ;
    PreviewChart->SetActorScale3D(FVector::OneVector);
    
    const FRotator CurrentActorRotation = PreviewChart->GetActorRotation();
    const bool bScaleChanged = !FinalScale.Equals(LastPreviewScaleXYZ, KINDA_SMALL_NUMBER);
    const bool bRotationChanged = !PreviewRotation.Equals(LastPreviewRotation, KINDA_SMALL_NUMBER) || !CurrentActorRotation.Equals(LastPreviewActorRotation, KINDA_SMALL_NUMBER);
    bPreviewGeometryDirty = bPreviewGeometryDirty || bScaleChanged || bRotationChanged;

    if (bPreviewGeometryDirty)
    {
        if (AScatterActor* Scatter = Cast<AScatterActor>(PreviewChart))
        {
            if (IsValid(Scatter))
            {
                Scatter->GraphScale = FinalScale * 100.0f; // Centimeters per data unit
                Scatter->PointScale = PreviewPointScale;
                Scatter->TextScale = PreviewTextScale;
                Scatter->AdditionalRotation = PreviewRotation;
                Scatter->Rebuild();
            }
        }
        else if (ALineGraphActor* Line = Cast<ALineGraphActor>(PreviewChart))
        {
            if (IsValid(Line))
            {
                Line->GraphScale = FinalScale * 100.0f; // Centimeters per data unit
                Line->PointScale = PreviewPointScale;
                Line->TextScale = PreviewTextScale;
                Line->AdditionalRotation = PreviewRotation;
                Line->Rebuild();
            }
        }
        else if (ABarChartActor* Bar = Cast<ABarChartActor>(PreviewChart))
        {
            if (IsValid(Bar))
            {
                Bar->GraphScale = FinalScale;
                Bar->TextScale = PreviewTextScale;
                Bar->AdditionalRotation = PreviewRotation;
                Bar->Rebuild();
            }
        }
        LastPreviewRotation = PreviewRotation;
        LastPreviewActorRotation = CurrentActorRotation;
        LastPreviewScaleXYZ = FinalScale;
        bPreviewGeometryDirty = false;
    }
}

void UDataVizPanelWidget::SetPreviewRotation(float Yaw, float Pitch, float Roll)
{
    PreviewRotation = FRotator(Pitch, Yaw, Roll);
    UpdatePreviewTransform();
}

void UDataVizPanelWidget::SetPreviewScale(float Scale)
{
    PreviewScale = Scale;
    UpdatePreviewTransform();
}

void UDataVizPanelWidget::HandleKeyboardInput()
{
    if (!GetWorld()) return;
    
    APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
    if (!PC) return;
    
    // Check for key presses using FKey
    bool bXPressed = PC->WasInputKeyJustPressed(FKey(TEXT("X")));
    bool bYPressed = PC->WasInputKeyJustPressed(FKey(TEXT("Y")));
    bool bZPressed = PC->WasInputKeyJustPressed(FKey(TEXT("Z")));
    bool bGPressed = PC->WasInputKeyJustPressed(FKey(TEXT("G")));
    bool bRPressed = PC->WasInputKeyJustPressed(FKey(TEXT("R")));
    bool bEPressed = PC->WasInputKeyJustPressed(FKey(TEXT("E")));
    bool bQPressed = PC->WasInputKeyJustPressed(FKey(TEXT("Q")));
    
    // Handle axis mode toggles
    if (bXPressed && IsPlacementActive())
    {
        bXAxisMode = !bXAxisMode;
        bYAxisMode = false;
        bZAxisMode = false;
        bGrabMode = false;
        UpdateVisualGuide();
    }
    if (bYPressed && IsPlacementActive())
    {
        bYAxisMode = !bYAxisMode;
        bXAxisMode = false;
        bZAxisMode = false;
        bGrabMode = false;
        UpdateVisualGuide();
    }
    if (bZPressed && IsPlacementActive())
    {
        bZAxisMode = !bZAxisMode;
        bXAxisMode = false;
        bYAxisMode = false;
        bGrabMode = false;
        UpdateVisualGuide();
    }
    if (bGPressed && IsPlacementActive() && (bXAxisMode || bYAxisMode || bZAxisMode))
    {
        bGrabMode = !bGrabMode;
        if (bGrabMode)
        {
            // Enter grab mode - mouse movement will move the graph
            PC->bShowMouseCursor = true;
            PC->bEnableClickEvents = true;
            PC->bEnableMouseOverEvents = true;
        }
        else
        {
            UpdateVisualGuide();
        }
    }
    
    // Handle rotation shortcuts (R for roll, E for pitch, Q for yaw)
    if (bRPressed && IsPlacementActive())
    {
        // Toggle roll rotation mode
    }
    if (bEPressed && IsPlacementActive())
    {
        // Toggle pitch rotation mode
    }
    if (bQPressed && IsPlacementActive())
    {
        // Toggle yaw rotation mode
    }
    
    // Handle grab mode mouse movement
    if (bGrabMode && IsPlacementActive() && PreviewChart)
    {
        float MouseDeltaX, MouseDeltaY;
        PC->GetInputMouseDelta(MouseDeltaX, MouseDeltaY);
        
        float MoveSpeed = 10.0f;
        FVector MoveDelta = FVector::ZeroVector;
        if (bXAxisMode) MoveDelta = FVector(MouseDeltaX * MoveSpeed, 0, 0);
        else if (bYAxisMode) MoveDelta = FVector(0, MouseDeltaX * MoveSpeed, 0);
        else if (bZAxisMode) MoveDelta = FVector(0, 0, MouseDeltaY * MoveSpeed);
        
        if (MoveDelta.SizeSquared() > 0.001f)
        {
            PreviewChart->AddActorWorldOffset(MoveDelta);
            UpdatePreviewTransform();
        }
    }
}

void UDataVizPanelWidget::UpdateVisualGuide()
{
    if (!GetWorld() || !IsPlacementActive() || !PreviewChart) return;
    
    // Destroy existing visual guide
    if (VisualGuideActor && IsValid(VisualGuideActor))
    {
        VisualGuideActor->Destroy();
        VisualGuideActor = nullptr;
    }
    
    // Create new visual guide based on active axis mode
    if (bXAxisMode || bYAxisMode || bZAxisMode)
    {
        FVector GuideStart = PreviewChart->GetActorLocation();
        FVector GuideEnd = GuideStart;
        FLinearColor GuideColor = FLinearColor::Red;
        
        if (bXAxisMode)
        {
            GuideEnd = GuideStart + FVector(5000.0f, 0, 0);
            GuideColor = FLinearColor::Red;
        }
        else if (bYAxisMode)
        {
            GuideEnd = GuideStart + FVector(0, 5000.0f, 0);
            GuideColor = FLinearColor::Green;
        }
        else if (bZAxisMode)
        {
            GuideEnd = GuideStart + FVector(0, 0, 5000.0f);
            GuideColor = FLinearColor::Blue;
        }
        
        // Spawn a visual guide line
        FActorSpawnParameters Params;
        Params.Owner = PreviewChart;
        if (AGridLineActor* Guide = GetWorld()->SpawnActor<AGridLineActor>(AGridLineActor::StaticClass(), FTransform(GuideStart), Params))
        {
            Guide->AttachToActor(PreviewChart, FAttachmentTransformRules::KeepWorldTransform);
            Guide->InitializeLine(GuideStart, GuideEnd, GuideColor, 5.0f);
            VisualGuideActor = Guide;
        }
    }
}

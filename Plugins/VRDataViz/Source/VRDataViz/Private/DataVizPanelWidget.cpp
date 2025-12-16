#include "DataVizPanelWidget.h"
#include "DataFileBlueprintLibrary.h"
#include "PlacementManager.h"
#include "Charts/PreviewPlacementActor.h"
#include "Charts/ScatterActor.h"
#include "Charts/LineGraphActor.h"
#include "Charts/BarChartActor.h"
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
#include "IXRTrackingSystem.h"
#include "IHeadMountedDisplay.h"
#include "HeadMountedDisplayTypes.h"

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

    RotationYawSlider->SetMinValue(0.0f);
    RotationYawSlider->SetMaxValue(360.0f);
    RotationYawSlider->SetValue(0.0f);
    RotationPitchSlider->SetMinValue(0.0f);
    RotationPitchSlider->SetMaxValue(360.0f);
    RotationPitchSlider->SetValue(0.0f);
    RotationRollSlider->SetMinValue(0.0f);
    RotationRollSlider->SetMaxValue(360.0f);
    RotationRollSlider->SetValue(0.0f);
    ScaleSlider->SetMinValue(0.1f);
    ScaleSlider->SetMaxValue(5.0f);
    ScaleSlider->SetValue(1.0f);

    RotationYawSlider->OnValueChanged.AddDynamic(this, &UDataVizPanelWidget::OnRotationYawChanged);
    RotationPitchSlider->OnValueChanged.AddDynamic(this, &UDataVizPanelWidget::OnRotationPitchChanged);
    RotationRollSlider->OnValueChanged.AddDynamic(this, &UDataVizPanelWidget::OnRotationRollChanged);
    ScaleSlider->OnValueChanged.AddDynamic(this, &UDataVizPanelWidget::OnScaleChanged);

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
    auto MakeLabeledSliderRow = [this, VBox](const FString& Label, USlider* Slider)
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
    };

    MakeLabeledSliderRow(TEXT("Rotation Yaw"), RotationYawSlider);
    MakeLabeledSliderRow(TEXT("Rotation Pitch"), RotationPitchSlider);
    MakeLabeledSliderRow(TEXT("Rotation Roll"), RotationRollSlider);
    MakeLabeledSliderRow(TEXT("Scale"), ScaleSlider);

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
    // No-op placeholder for now
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
    UE_LOG(LogTemp, Error, TEXT("OnCancel - Cancel Placement button clicked!"));
    
    // Set a flag to prevent HandlePlacementInput from confirming
    // Cancel placement first
    CancelPlacement();
    
    // Make absolutely sure preview is destroyed and placement is inactive
    if (PreviewChart != nullptr)
    {
        UE_LOG(LogTemp, Error, TEXT("OnCancel - Force destroying PreviewChart"));
        if (IsValid(PreviewChart))
        {
            PreviewChart->Destroy();
        }
        PreviewChart = nullptr;
    }
    
    UE_LOG(LogTemp, Error, TEXT("OnCancel - CancelPlacement() called. PreviewChart is now: %s, IsPlacementActive: %d"), 
        (PreviewChart == nullptr) ? TEXT("NULL") : TEXT("NOT NULL"),
        IsPlacementActive() ? 1 : 0);
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
    PreviewScale = FMath::Clamp(Value, 0.1f, 10.0f); // Clamp to safe range
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
    
    // Create actual chart as preview
    PreviewChart = CreatePreviewChart();
    if (!PreviewChart || !IsValid(PreviewChart))
    {
        UE_LOG(LogTemp, Error, TEXT("StartHoverPlacement - Failed to create preview chart"));
        return;
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
    if (!PreviewChart || !IsValid(PreviewChart) || bUseXYZPlacement) return false;
    
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
    
    // Clear preview state but keep the chart
    PreviewChart = nullptr;
    if (PlacementMgr) { PlacementMgr->Destroy(); PlacementMgr = nullptr; }
    
    UE_LOG(LogTemp, Warning, TEXT("ConfirmPlacement - SUCCESS! Chart placed"));
    return true;
}

void UDataVizPanelWidget::HandlePlacementInput()
{
    // Called on keyboard click or VR trigger - same as Confirm
    // BUT: Only confirm if placement is active AND preview chart exists
    if (!IsPlacementActive())
    {
        UE_LOG(LogTemp, Log, TEXT("HandlePlacementInput - Placement not active, ignoring input"));
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
    
    // Destroy preview chart if it exists
    if (PreviewChart && IsValid(PreviewChart))
    {
        UE_LOG(LogTemp, Warning, TEXT("CancelPlacement - Destroying preview chart: %s"), *PreviewChart->GetName());
        PreviewChart->Destroy();
        PreviewChart = nullptr;
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("CancelPlacement - PreviewChart was null or invalid"));
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
    
    // Double-check that preview is cleared
    if (PreviewChart != nullptr)
    {
        UE_LOG(LogTemp, Error, TEXT("CancelPlacement - WARNING: PreviewChart still exists after cancellation!"));
        PreviewChart = nullptr; // Force clear
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
    
    // Clamp scale to safe range
    const float SafeScale = FMath::Clamp(PreviewScale, 0.1f, 10.0f);
    FTransform InitialTransform(PreviewRotation, InitialLocation, FVector(SafeScale));
    
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
    
    // Apply rotation from sliders
    PreviewChart->SetActorRotation(PreviewRotation);
    
    // Apply uniform scale to the chart
    const float SafeScale = FMath::Clamp(PreviewScale, 0.1f, 10.0f);
    PreviewChart->SetActorScale3D(FVector(SafeScale));
    
    // Update chart-specific rotation properties
    if (AScatterActor* Scatter = Cast<AScatterActor>(PreviewChart))
    {
        if (IsValid(Scatter))
        {
            Scatter->AdditionalRotation = PreviewRotation;
        }
    }
    else if (ALineGraphActor* Line = Cast<ALineGraphActor>(PreviewChart))
    {
        if (IsValid(Line))
        {
            Line->AdditionalRotation = PreviewRotation;
        }
    }
    else if (ABarChartActor* Bar = Cast<ABarChartActor>(PreviewChart))
    {
        if (IsValid(Bar))
        {
            Bar->AdditionalRotation = PreviewRotation;
        }
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


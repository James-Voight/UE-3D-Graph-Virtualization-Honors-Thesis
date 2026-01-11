# VRDataViz Plugin

In-world 3D UI panel and data loading utilities for runtime chart visualization in UE5.

## Install
- Copy the `VRDataViz` folder into your project's `Plugins/` directory.
- Open the project; enable the plugin if prompted.

## Data Folder
- Place CSV files in: `Saved/DataCharts/` (create the folder if it doesn't exist).
- Use `UDataFileBlueprintLibrary::GetCSVFiles` to enumerate available CSVs.

## Actors
- `AWorldUIPanelActor`: Actor with `UWidgetComponent` set to World Space. Assign your UMG Widget Blueprint as the class.
- `APlacementManager`: Utility actor with line trace for hover placement; returns hit location/normal.

## Blueprint Functions
- `GetCSVFiles(out FilePaths, SubfolderName)`: Lists `*.csv` under `Saved/SubfolderName` (default `DataCharts`).
- `LoadTextFile(FilePath, out Text)`: Reads text file content.

## Usage
1. Create a Widget Blueprint for the panel UI (buttons, combo box, etc.).
2. Place `AWorldUIPanelActor` in the level and set its Widget Class to your Widget BP.
3. In the UI, call `GetCSVFiles` to populate a list. On selection, call `LoadTextFile` and parse.
4. For world placement, use `APlacementManager.TracePlacementLocation(Start, Direction, out Location, out Normal)` then spawn/confirm.

## Customization
- At runtime you can adjust charts via `UChartAdjustLibrary`:
  - `SetActorScale(ChartActor, NewScale)` – scales chart extents.
  - `SetActorRotation(ChartActor, NewRotation)` – rotates drawing.
  - `SetAxisRanges(ChartActor, bUseCustom, XMin, XMax, YMin, YMax, ZMin, ZMax)` – remap data domain, e.g., Z 0–1 or -100–100.
  - After updates, actors rebuild automatically.

## Notes
- Designed for runtime and packaged builds; avoids desktop file dialogs.
- For VR interaction later, add a `UWidgetInteractionComponent` on the controller.


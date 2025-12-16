#include "PlacementManager.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "Charts/PreviewPlacementActor.h"

APlacementManager::APlacementManager()
{
    PrimaryActorTick.bCanEverTick = false;
    MaxTraceDistance = 2000.0f;
    TraceChannel = ECC_Visibility;
    OwnedPreview = nullptr;
}

bool APlacementManager::TracePlacementLocation(const FVector& Start, const FVector& Direction, FVector& OutLocation, FVector& OutNormal)
{
    if (!GetWorld())
    {
        return false;
    }

    const FVector End = Start + Direction.GetSafeNormal() * MaxTraceDistance;

    FHitResult Hit;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(PlacementTrace), false, this);
    if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, TraceChannel, Params))
    {
        OutLocation = Hit.ImpactPoint;
        OutNormal = Hit.ImpactNormal;
        return true;
    }

    return false;
}

APreviewPlacementActor* APlacementManager::EnsurePreviewActor(UWorld* World)
{
    if (!World) return nullptr;
    if (IsValid(OwnedPreview)) return OwnedPreview;
    FActorSpawnParameters Params; Params.Owner = this;
    OwnedPreview = World->SpawnActor<APreviewPlacementActor>(APreviewPlacementActor::StaticClass(), FTransform::Identity, Params);
    return OwnedPreview;
}

void APlacementManager::UpdatePreviewTransform(APreviewPlacementActor* Preview, const FVector& Location, const FVector& Normal)
{
    if (!Preview) return;
    const FRotator Align = Normal.ToOrientationRotator();
    FTransform T(Align, Location);
    Preview->SetPreviewTransform(T);
}


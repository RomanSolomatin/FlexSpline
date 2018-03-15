/*****************************************************************************
* Copyright (C) 2017 Oliver Hawk - All Rights Reserved
*
* @Author       Oliver Hawk
* @EMail        *************************
* @Package      Flex Spline
******************************************************************************/

#include "FlexSplinePrivatePCH.h"
#include "FlexSplineActor.h"
#include "Components/SplineComponent.h"
#include "Components/SplineMeshComponent.h"
#include "Components/ArrowComponent.h"
#include "Components/TextRenderComponent.h"
#include "Kismet/KismetMathLibrary.h"

// Helper aliases, for terser code
static const auto StaticMeshClass = UStaticMeshComponent::StaticClass();
static const auto SplineMeshClass = USplineMeshComponent::StaticClass();
static const auto LocalSpace      = ESplineCoordinateSpace::Local;
static const auto WorldSpace      = ESplineCoordinateSpace::World;

//////////////////////////////////////////////////////////////////////////
// STATIC HELPERS
static FColor GetColorForArrow(int32 MeshIndex)
{
    static const TArray<FColor> colors = {
          FColor::Orange
        , FColor::Green
        , FColor::Blue
        , FColor::Red
        , FColor::Emerald
        , FColor::Magenta
        , FColor::Cyan
        , FColor::Yellow
        , FColor::Purple
        , FColor::Turquoise
        , FColor::Silver
    };

    MeshIndex = FMath::Clamp(MeshIndex, 0, colors.Num()-1);
    return colors[MeshIndex];
}

static float RandomizeFloat(float InFloat, int32 Index, FName Layername)
{
    const int32 seed  = GetTypeHash(InFloat) / 2
                      + GetTypeHash(Layername) / 2
                      + static_cast<int32>(InFloat)
                      - Index;
    return ( InFloat * UKismetMathLibrary::RandomFloatInRangeFromStream(-1.f, 1.f, FRandomStream(seed)) );
}

static FVector RandomizeVector(const FVector& InVec, int32 Index, FName Layername)
{
    const float randX = (InVec.X != 0.f) ? RandomizeFloat(InVec.X, Index, Layername) : 0.f;
    const float randY = (InVec.Y != 0.f) ? RandomizeFloat(InVec.Y, Index, Layername) : 0.f;
    const float randZ = (InVec.Z != 0.f) ? RandomizeFloat(InVec.Z, Index, Layername) : 0.f;

    return FVector(randX, randY, randZ);
}

static FRotator RandomizeRotator(const FRotator& InRot, int32 Index, FName Layername)
{
    // Maps rotator values onto a vector, randomizes, then reverses back to rotator
    const FVector vecFromRot = RandomizeVector(FVector(InRot.Pitch, InRot.Yaw, InRot.Roll), Index, Layername);
    return FRotator(vecFromRot.X, vecFromRot.Y, vecFromRot.Z);
}

static uint32 GeneratePointHashValue(const USplineComponent* const SplineComp, int32 Index)
{
    return SplineComp != nullptr
         ? GetTypeHash(SplineComp->GetLocationAtSplinePoint(Index, LocalSpace))
         : 0;
}

static float FSeededRand(int32 Seed)
{
    return UKismetMathLibrary::RandomFloatInRangeFromStream(0.f, 1.f, FRandomStream((Seed + 1) * 13));
}

static UClass* GetMeshType(EFlexSplineMeshType MeshType)
{
    switch (MeshType)
    {
    case EFlexSplineMeshType::SplineMesh: return SplineMeshClass;
    case EFlexSplineMeshType::StaticMesh: return StaticMeshClass;
    default: return nullptr;
    }
}

static bool CanRenderFromSpawnChance(const FSplineMeshInitData& MeshInitData, int32 CurrentIndex)
{
    const float spawnChance = MeshInitData.RenderInfo.SpawnChance;
    const int32 spawnSeed   = GetTypeHash(MeshInitData.MeshComponentsArray[CurrentIndex]->GetName()) * spawnChance;

    if (MeshInitData.RenderInfo.bRandomizeSpawnChance)
    {
        return spawnChance > FSeededRand(spawnSeed);
    }

    // Compare index-spawn-chance-ratio and see if it has changed from ratio of last index
    const float interval     = 1.f / FMath::Clamp(spawnChance, 0.00001f, 1.f);
    const int32 currentRatio = static_cast<int32>(CurrentIndex / interval);
    const int32 lastRatio    = CurrentIndex <= 0
                             ? spawnChance > 0.f ? 1 : 0 // edge case first index
                             : static_cast<int32>(((CurrentIndex - 1) / interval));

    return currentRatio != lastRatio;
}

static ESplineMeshAxis::Type ToSplineAxis(EFlexSplineAxis FlexSplineAxis)
{
    return static_cast<ESplineMeshAxis::Type>( static_cast<uint8>(FlexSplineAxis) );
}

void DestroyMeshComponent(FSplineMeshInitData& MeshInitData, int32 Index)
{
    StaticMeshWeakPtr mesh = MeshInitData.MeshComponentsArray[Index];
    if (mesh.IsValid())
    {
        mesh->DestroyComponent();
    }
    MeshInitData.MeshComponentsArray.RemoveAt(Index);
}


//////////////////////////////////////////////////////////////////////////
// STRUCT FUNCTIONS
FSplineMeshInitData::~FSplineMeshInitData()
{
    for (StaticMeshWeakPtr mesh : MeshComponentsArray)
    {
        if (mesh.IsValid())
        {
            mesh->ConditionalBeginDestroy();
        }
    }
    for (ArrowWeakPtr arrow : ArrowSplineUpIndicatorArray)
    {
        if (arrow.IsValid())
        {
            arrow->ConditionalBeginDestroy();
        }
    }
}


//////////////////////////////////////////////////////////////////////////
// CONSTRUCTOR + BASE INTERFACE + GETTER
AFlexSplineActor::AFlexSplineActor()
    : Super()
    , CollisionActiveConfig(EFlexGlobalConfigType::Nowhere)
    , SynchronizeConfig(EFlexGlobalConfigType::Custom)
    , LoopConfig(EFlexGlobalConfigType::Custom)
    , bShowPointNumbers(false)
    , PointNumberSize(125.f)
    , UpDirectionArrowSize(3.f)
    , UpDirectionArrowOffset(25.f)
    , TextRenderColor(FColor::Cyan)
{
    PrimaryActorTick.bCanEverTick = false;

    // Spline Component
    SplineComponent = CreateDefaultSubobject<USplineComponent>(TEXT("Spline"));
    SplineComponent->SetMobility(EComponentMobility::Static);
    RootComponent = SplineComponent;
}

void AFlexSplineActor::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);

    // FlexSpline construction for editor builds here
    ConstructSplineMesh();
}

void AFlexSplineActor::PreInitializeComponents()
{
    Super::PreInitializeComponents();

    // FlexSpline construction for cooked builds here because it has no OnConstrcution
#if !WITH_EDITOR
    ConstructSplineMesh();
#endif
}

int32 AFlexSplineActor::GetMeshCountForType(EFlexSplineMeshType MeshType) const
{
    int32 count = 0;
    for (const TTuple<FName, FSplineMeshInitData>& meshInitDataPair : MeshDataInitMap)
    {
        const FSplineMeshInitData& meshInitData = meshInitDataPair.Value;
        if ( meshInitData.MeshInfo.MeshType == MeshType && TEST_BIT(meshInitData.GeneralInfo, EFlexGeneralFlags::Active) )
        {
            count++;
        }
    }
    return count;
}


//////////////////////////////////////////////////////////////////////////
// FLEX SPLINE FUNCTIONALITY
void AFlexSplineActor::ConstructSplineMesh()
{
    // Get all indices that were deleted, if any
    TArray<int32> deletedIndices;
    GetDeletedIndices(deletedIndices);

    InitializeNewMeshData();

    // Check if number of spline points and point data align, add or remove data accordingly
    AddPointDataEntries();
    RemovePointDataEntries(deletedIndices);

    // Check if number of spline points and meshes align, add or remove meshes accordingly
    InitDataAddMeshes();
    InitDataRemoveMeshes(deletedIndices);

    // Update the spline itself with the gathered data
    UpdatePointData();
    UpdateMeshComponents();
    UpdateDebugInformation();
}

void AFlexSplineActor::InitializeNewMeshData()
{
    const int32 meshInitMapNum = MeshDataInitMap.Num();

    for (TTuple<FName, FSplineMeshInitData>& meshInitDataPair : MeshDataInitMap)
    {
        FSplineMeshInitData& meshInitData = meshInitDataPair.Value;
        if (!meshInitData.IsInitialized())
        {
            // Generate name for new entry
            for (int32 index = 0; index < meshInitMapNum; index++)
            {
                const FName newLayerName = FName(*FString(TEXT("Layer " + FString::FromInt(index))));
                if ( !MeshDataInitMap.Contains(newLayerName) && newLayerName != LastUsedKey )
                {
                    meshInitDataPair.Key = newLayerName;
                    LastUsedKey = newLayerName;
                    break;
                }
            }

            // Init data from template
            meshInitData = MeshDataTemplate;
            meshInitData.Initialize();
        }
    }
}

void AFlexSplineActor::AddPointDataEntries()
{
    const int32 pointDataArraySize = PointDataArray.Num();
    const int32 numberOfSplinePoints = SplineComponent->GetNumberOfSplinePoints();

    for (int32 i = pointDataArraySize; i < numberOfSplinePoints; i++)
    {
        FSplinePointData newPointData;

        // Create text renderer to show point index in editor
        UTextRenderComponent* newTextRender = NewObject<UTextRenderComponent>(RootComponent);
        newTextRender->RegisterComponent();
        newTextRender->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);
        newTextRender->SetWorldSize(PointNumberSize);
        newTextRender->SetHiddenInGame(true);
        newTextRender->SetTextRenderColor(TextRenderColor);
        newPointData.IndexTextRenderer = newTextRender;

        // Save entry
        PointDataArray.Add(newPointData);
    }
}

void AFlexSplineActor::RemovePointDataEntries(const TArray<int32>& DeletedIndices)
{
    // Use gathered indices to clean up and delete redundant data
    for (const int32 index : DeletedIndices)
    {
        const FSplinePointData& pointData = PointDataArray[index];

        // Remove this point's text-render
        UTextRenderComponent* indexText = pointData.IndexTextRenderer;
        if (indexText)
        {
            indexText->DestroyComponent();
        }

        // Remove arrows
        for (TTuple<FName, FSplineMeshInitData>& meshInitDataPair : MeshDataInitMap)
        {
            FSplineMeshInitData& meshInitData = meshInitDataPair.Value;
            ArrowWeakPtr arrow                = meshInitData.ArrowSplineUpIndicatorArray[index];
            if (arrow.IsValid())
            {
                meshInitData.ArrowSplineUpIndicatorArray.Remove(arrow);
                arrow->DestroyComponent();
            }
        }

        PointDataArray.RemoveAt(index);
    }
}

void AFlexSplineActor::InitDataAddMeshes()
{
    // Each mesh-init data stores all spline mesh components of its type, their location scattered across all spline points
    // Here we add spline meshes until it has as many meshes as there are spline points
    for (TTuple<FName, FSplineMeshInitData>& meshInitDataPair : MeshDataInitMap)
    {
        FSplineMeshInitData& meshInitData = meshInitDataPair.Value;
        UClass* meshType                  = GetMeshType(meshInitData.MeshInfo.MeshType);
        const int32 numberOfSplinePoints  = SplineComponent->GetNumberOfSplinePoints();
        const int32 numberOfSplineMeshes  = meshInitData.MeshComponentsArray.Num();

        if (numberOfSplineMeshes < numberOfSplinePoints)
        {
            for (int32 i = numberOfSplineMeshes; i < numberOfSplinePoints; i++)
            {
                CreateMeshComponent(meshType, meshInitData);
                CreateArrrowComponent(meshInitData);
            }
        }
    }
}

void AFlexSplineActor::InitDataRemoveMeshes(const TArray<int32>& DeletedIndices)
{
    for (const int32 index : DeletedIndices)
    {
        // Remove all spline Meshes at this spline index (which was removed)
        for (TTuple<FName, FSplineMeshInitData>& meshInitDataPair : MeshDataInitMap)
        {
            FSplineMeshInitData& meshInitData = meshInitDataPair.Value;
            const int32 numberOfSplinePoints  = SplineComponent->GetNumberOfSplinePoints();
            const int32 numberOfSplineMeshes  = meshInitData.MeshComponentsArray.Num();

            if (numberOfSplineMeshes > numberOfSplinePoints)
            {
                DestroyMeshComponent(meshInitData, index);
            }
        }
    }
}

void AFlexSplineActor::UpdatePointData()
{
    const int32 pointDataArraySize = PointDataArray.Num();
    for (int32 index = 0; index < pointDataArraySize; index++)
    {
        FSplinePointData& pointData = PointDataArray[index];

        // Update ID
        pointData.ID = GeneratePointHashValue(SplineComponent, index);
    }
}

void AFlexSplineActor::UpdateDebugInformation()
{
    const int32 pointDataArraySize = PointDataArray.Num();
    for (int32 index = 0; index < pointDataArraySize; index++)
    {
        const FSplinePointData& pointData = PointDataArray[index];

        // Update text renderer
        UTextRenderComponent* textRenderer = pointData.IndexTextRenderer;
        if (textRenderer)
        {
            const FRotator splineRotation = SplineComponent->GetRotationAtSplinePoint(index, LocalSpace);
            textRenderer->SetWorldLocation(GetTextPosition(index));
            textRenderer->SetText(FText::AsNumber(index));
            textRenderer->SetTextRenderColor(TextRenderColor);
            textRenderer->SetRelativeRotation(FRotator(0.f, -splineRotation.Yaw, 0.f));
            textRenderer->SetWorldSize(PointNumberSize);
            textRenderer->SetVisibility(bShowPointNumbers);
        }

        // Update up-vector-arrow
        int32 meshInitIndex = 0;
        for (auto& meshInitDataPair : MeshDataInitMap)
        {
            const FSplineMeshInitData& meshInitData = meshInitDataPair.Value;
            const USplineMeshComponent* splineMesh  = Cast<USplineMeshComponent>(meshInitData.MeshComponentsArray[index].Get());
            UArrowComponent* arrow                  = meshInitData.ArrowSplineUpIndicatorArray[index].Get();

            if (meshInitData.UpVectorInfo.bShowUpDirection
                && splineMesh
                && arrow
                && MeshDataInitMap.Num() > 0
                && index != pointDataArraySize - 1)
            {
                arrow->SetRelativeRotation(splineMesh->GetSplineUpDir().Rotation());
                arrow->SetWorldLocation(textRenderer->GetComponentLocation() + textRenderer->GetUpVector() * UpDirectionArrowOffset);
                arrow->SetArrowColor(GetColorForArrow(meshInitIndex));
                arrow->ArrowSize = UpDirectionArrowSize;
                arrow->SetVisibility(true);
            }
            else if (arrow)
            {
                arrow->SetVisibility(false);
            }
            meshInitIndex++;
        }
    }
}

void AFlexSplineActor::UpdateMeshComponents()
{
    const int32 numSplinePoints = SplineComponent->GetNumberOfSplinePoints();

    // Update all meshes for the current mesh initializer
    for (TTuple<FName, FSplineMeshInitData>& meshInitDataPair : MeshDataInitMap)
    {
        FSplineMeshInitData& meshInitData = meshInitDataPair.Value;
        UClass* configuredMeshType        = GetMeshType(meshInitData.MeshInfo.MeshType);

        for (int32 index = 0; index < numSplinePoints; index++)
        {
            UStaticMeshComponent* meshComp = meshInitData.MeshComponentsArray[index].Get();
            UClass* meshType               = meshComp->GetClass();

            // Replace mesh if type has changed
            if (configuredMeshType != meshType)
            {
                DestroyMeshComponent(meshInitData, index);
                CreateMeshComponent(configuredMeshType, meshInitData, index);
                meshComp = meshInitData.MeshComponentsArray[index].Get();
                meshType = meshComp->GetClass();
            }

            // Update mesh settings
            const int32 finalIndex = numSplinePoints - 1;

            if (!TEST_BIT(meshInitData.GeneralInfo, EFlexGeneralFlags::Active) // Inactive
                || index == finalIndex && !GetCanLoop(meshInitData)            // No loop, so cut out last mesh 
                || !CanRenderFromSpawnChance(meshInitData, index)              // Spawn chance too low
                || !CanRenderFromMode(meshInitData, index, finalIndex))        // Render-Mode check
            {
                meshComp->SetVisibility(false);
                meshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            }
            else
            {
                // Update type agnostic mesh settings
                meshComp->SetCollisionProfileName(meshInitData.PhysicsInfo.CollisionProfileName);
                meshComp->SetVisibility(true);
                meshComp->SetCollisionEnabled(GetCollisionEnabled(meshInitData));
                meshComp->bGenerateOverlapEvents = meshInitData.PhysicsInfo.bGenerateOverlapEvent;
                meshComp->SetMobility(EComponentMobility::Movable); // <- Required for SetStaticMesh to work correctly
                meshComp->SetStaticMesh(meshInitData.MeshInfo.Mesh);
                meshComp->SetMobility(EComponentMobility::Static);
                meshComp->SetMaterial(0, meshInitData.MeshInfo.MeshMaterial);

                // Update type dependent mesh settings
                if (meshType == SplineMeshClass)
                {
                    USplineMeshComponent* splineMeshComp = Cast<USplineMeshComponent>(meshComp);
                    UpdateSplineMesh(meshInitData, splineMeshComp, index);
                }
                else if (meshType == StaticMeshClass)
                {
                    UpdateStaticMesh(meshInitData, meshComp, index);
                }
            }
        }
    }
}

void AFlexSplineActor::UpdateSplineMesh(const FSplineMeshInitData& MeshInitData, USplineMeshComponent* SplineMesh, int32 CurrentIndex)
{
    if (SplineMesh)
    {
        const FName layerName             = GetLayerName(MeshInitData);
        const FSplinePointData& pointData = PointDataArray[CurrentIndex];
        const bool bSync                  = GetCanSynchronize(pointData) && (CurrentIndex > 0);
        auto&& previousPointData          = bSync ? PointDataArray[CurrentIndex - 1] : FSplinePointData();

        const FVector randScale         = MeshInitData.ScaleInfo.bUseUniformScaleRandomOffset
                                        ? FVector(RandomizeFloat(MeshInitData.ScaleInfo.UniformScaleRandomOffset, CurrentIndex, layerName))
                                        : RandomizeVector(MeshInitData.ScaleInfo.ScaleRandomOffset, CurrentIndex, layerName);
        const FVector2D randScale2D     = FVector2D(randScale.Y, randScale.Z);
        const FVector meshInitScale     = MeshInitData.ScaleInfo.bUseUniformScale
                                        ? FVector(1.f, MeshInitData.ScaleInfo.UniformScale, MeshInitData.ScaleInfo.UniformScale)
                                        : MeshInitData.ScaleInfo.Scale;
        const FVector2D meshInitScale2D = FVector2D(meshInitScale.Y, meshInitScale.Z) + randScale2D;
        const FRotator randRotator      = RandomizeRotator(MeshInitData.RotationInfo.RotationRandomOffset, CurrentIndex, layerName);

        // Set spline params
        SetSplineMeshLocation(MeshInitData, SplineMesh, CurrentIndex);
        SplineMesh->SetSplineUpDir(CalculateUpDirection(MeshInitData, pointData, CurrentIndex), LocalSpace);
        SplineMesh->SetForwardAxis(ToSplineAxis(MeshInitData.MeshInfo.MeshForwardAxis));
        SplineMesh->SetRelativeRotation(MeshInitData.RotationInfo.Rotation + randRotator);
        SplineMesh->SetRelativeScale3D(FVector(meshInitScale.X + randScale.X, SplineMesh->RelativeScale3D.Y, SplineMesh->RelativeScale3D.Z));

        // Apply spline point data (or sync with previous point if demanded)
        SplineMesh->SetStartRoll(bSync ? previousPointData.EndRoll : pointData.StartRoll);
        SplineMesh->SetEndRoll(pointData.EndRoll);
        SplineMesh->SetStartScale((bSync ? previousPointData.EndScale : (pointData.StartScale)) * meshInitScale2D);
        SplineMesh->SetEndScale(pointData.EndScale * meshInitScale2D);
    }
}

void AFlexSplineActor::UpdateStaticMesh(const FSplineMeshInitData& MeshInitData, UStaticMeshComponent* StaticMesh, int32 CurrentIndex)
{
    if (StaticMesh)
    {
        const FSplinePointData& pointData = PointDataArray[CurrentIndex];

        // Apply mesh-init configurations
        StaticMesh->SetRelativeLocation(CalculateLocation(MeshInitData, pointData, CurrentIndex));
        StaticMesh->SetRelativeRotation(CalculateRotation(MeshInitData, pointData, CurrentIndex));
        StaticMesh->SetRelativeScale3D(CalculateScale(MeshInitData, pointData, CurrentIndex));
    }
}

FName AFlexSplineActor::GetLayerName(const FSplineMeshInitData& MeshInitData) const
{
    const FName* result = MeshDataInitMap.FindKey(MeshInitData);
    return (result) ? (*result) : (TEXT(""));
}


//////////////////////////////////////////////////////////////////////////
// HELPERS
void AFlexSplineActor::GetDeletedIndices(TArray<int32>& OutIndexArray) const
{
    OutIndexArray.Empty();
    const int32 pointDataArraySize   = PointDataArray.Num();
    const int32 numberOfSplinePoints = SplineComponent->GetNumberOfSplinePoints();

    if (pointDataArraySize > numberOfSplinePoints)
    {
        // Move along a spline-point-data-pair and store all indices that do not match(CHECK FRONT)
        int32 dataCounter        = 0;
        int32 splinePointCounter = 0;
        for (; splinePointCounter < numberOfSplinePoints; splinePointCounter++, dataCounter++)
        {
            uint32 dataID        = PointDataArray[dataCounter].ID;
            const uint32 pointID = GeneratePointHashValue(SplineComponent, splinePointCounter);

            while (dataID != pointID)
            {
                OutIndexArray.AddUnique(dataCounter);
                dataCounter++;
                dataID = PointDataArray[dataCounter].ID;
            }
        }

        // Store all indices from deleted spline points at the end of the spline(CHECK BACK)
        const int32 lastSplineIndex = numberOfSplinePoints + OutIndexArray.Num() - 1;
        const int32 lastDataIndex   = PointDataArray.Num() - 1;
        for (dataCounter = lastDataIndex; dataCounter > lastSplineIndex; dataCounter--)
        {
            OutIndexArray.AddUnique(dataCounter);
        }
        // Put highest values on top, so highest indices are deleted first to avoid out-of-range-exception
        OutIndexArray.Sort();
        Algo::Reverse(OutIndexArray);
    }
}

FVector AFlexSplineActor::GetTextPosition(int32 Index) const
{
    // Return top of the highest bounding box from all meshes than can be found at this point
    const int32 pointArrayMax         = PointDataArray.Num() - 1;
    const FVector splinePointLocation = SplineComponent->GetLocationAtSplinePoint(Index, WorldSpace);
    float highestPoint                = splinePointLocation.Z;

    for (const TTuple<FName, FSplineMeshInitData>& meshInitDataPair : MeshDataInitMap)
    {
        const FSplineMeshInitData& meshInitData = meshInitDataPair.Value;
        const StaticMeshWeakPtr mesh            = meshInitData.MeshComponentsArray[ (pointArrayMax == Index && Index > 0 && !GetCanLoop(meshInitData))
                                                ? Index - 1
                                                : Index ];

        if (mesh.IsValid() && mesh->IsVisible())
        {
            const float max = mesh->Bounds.GetBox().Max.Z;
            highestPoint    = FMath::Max(max, highestPoint);
        }
    }

    return FVector(splinePointLocation.X, splinePointLocation.Y, highestPoint);
}

bool AFlexSplineActor::CanRenderFromMode(const FSplineMeshInitData& MeshInitData, int32 CurrentIndex, int32 FinalIndex) const
{
    bool result = false;
    // When not looping, the final index should be one point earlier
    FinalIndex = !GetCanLoop(MeshInitData) ? FinalIndex - 1 : FinalIndex;
    FinalIndex = FMath::Max<int32>(0, FinalIndex);

    if (TEST_BIT(MeshInitData.RenderInfo.RenderMode, EFlexSplineRenderMode::Middle))
    {
        result = (CurrentIndex != 0) && (CurrentIndex != FinalIndex);
    }
    if (!result && TEST_BIT(MeshInitData.RenderInfo.RenderMode, EFlexSplineRenderMode::Head))
    {
        result = (CurrentIndex == 0);
    }
    if (!result && TEST_BIT(MeshInitData.RenderInfo.RenderMode, EFlexSplineRenderMode::Tail))
    {
        result = (CurrentIndex == FinalIndex);
    }
    if (!result && TEST_BIT(MeshInitData.RenderInfo.RenderMode, EFlexSplineRenderMode::Custom))
    {
        result = MeshInitData.RenderInfo.RenderModeCustomIndices.Contains(CurrentIndex);
    }

    return result;
}

ECollisionEnabled::Type AFlexSplineActor::GetCollisionEnabled(const FSplineMeshInitData& MeshInitData) const
{
    switch (CollisionActiveConfig)
    {
    case EFlexGlobalConfigType::Everywhere: return ECollisionEnabled::QueryAndPhysics;
    case EFlexGlobalConfigType::Nowhere:    return ECollisionEnabled::NoCollision;
    case EFlexGlobalConfigType::Custom:     return MeshInitData.PhysicsInfo.Collision;
    default: return ECollisionEnabled::NoCollision;
    }
}

bool AFlexSplineActor::GetCanLoop(const FSplineMeshInitData& MeshInitData) const
{
    switch (LoopConfig)
    {
    case EFlexGlobalConfigType::Everywhere: return true;
    case EFlexGlobalConfigType::Nowhere:    return false;
    case EFlexGlobalConfigType::Custom:     return TEST_BIT(MeshInitData.GeneralInfo, EFlexGeneralFlags::Loop);
    default: return false;
    }
}

bool AFlexSplineActor::GetCanSynchronize(const FSplinePointData& PointData) const
{
    switch (SynchronizeConfig)
    {
    case EFlexGlobalConfigType::Everywhere: return true;
    case EFlexGlobalConfigType::Nowhere:    return false;
    case EFlexGlobalConfigType::Custom:     return PointData.bSynchroniseWithPrevious;
    default: return false;
    }
}

FVector AFlexSplineActor::CalculateLocation(const FSplineMeshInitData& MeshInitData, const FSplinePointData& PointData, const int32 Index) const
{
    const FVector splinePointLocation = SplineComponent->GetLocationAtSplinePoint(Index, LocalSpace);
    FVector meshInitLocation          = MeshInitData.LocationInfo.Location;
    FVector pointDataLocationOffset   = PointData.SMLocationOffset;
    FVector randomizedVector          = RandomizeVector(MeshInitData.LocationInfo.LocationRandomOffset, Index, GetLayerName(MeshInitData));

    if (MeshInitData.LocationInfo.CoordinateSystem == EFlexCoordinateSystem::SplinePoint)
    {
        const FRotator coordSystem  = SplineComponent->GetDirectionAtSplinePoint(Index, LocalSpace).Rotation();
        // Rotate all values around new local coordinate system
        meshInitLocation        = coordSystem.RotateVector(meshInitLocation);
        pointDataLocationOffset = coordSystem.RotateVector(pointDataLocationOffset);
        randomizedVector        = coordSystem.RotateVector(randomizedVector);
    }

    return splinePointLocation + meshInitLocation + pointDataLocationOffset + randomizedVector;
}

FRotator AFlexSplineActor::CalculateRotation(const FSplineMeshInitData& MeshInitData, const FSplinePointData& PointData, const int32 Index) const
{
    const FRotator meshInitRotation    = MeshInitData.RotationInfo.Rotation;
    const FRotator randomRotation      = RandomizeRotator(MeshInitData.RotationInfo.RotationRandomOffset, Index, GetLayerName(MeshInitData));
    const FRotator pointDataRotation   = PointData.SMRotation;
    const FRotator splinePointRotation = MeshInitData.RotationInfo.CoordinateSystem == EFlexCoordinateSystem::SplinePoint
                                       ? SplineComponent->GetRotationAtSplinePoint(Index, LocalSpace)
                                       : FRotator::ZeroRotator;

    return meshInitRotation + randomRotation + pointDataRotation + splinePointRotation;
}

FVector AFlexSplineActor::CalculateScale(const FSplineMeshInitData& MeshInitData, const FSplinePointData& PointData, const int32 Index) const
{
    const FName layerName          = GetLayerName(MeshInitData);
    const FVector randomScale      = MeshInitData.ScaleInfo.bUseUniformScaleRandomOffset
                                   ? FVector(RandomizeFloat(MeshInitData.ScaleInfo.UniformScaleRandomOffset, Index, layerName))
                                   : RandomizeVector(MeshInitData.ScaleInfo.ScaleRandomOffset, Index, layerName);
    const FVector pointDataScale   = PointData.SMScale;
    const FVector splinePointScale = SplineComponent->GetScaleAtSplinePoint(Index);
    const FVector meshInitScale    = MeshInitData.ScaleInfo.bUseUniformScale
                                   ? FVector(MeshInitData.ScaleInfo.UniformScale)
                                   : MeshInitData.ScaleInfo.Scale;

    return (meshInitScale * splinePointScale) + pointDataScale + randomScale;
}

FVector AFlexSplineActor::CalculateUpDirection(const FSplineMeshInitData& MeshInitData, const FSplinePointData& PointData, const int32 Index) const
{
    FVector meshInitUpDir = MeshInitData.UpVectorInfo.CustomMeshUpDirection;
    FVector pointUpDir    = PointData.CustomPointUpDirection;

    if (MeshInitData.UpVectorInfo.CoordinateSystem == EFlexCoordinateSystem::SplinePoint)
    {
        // Convert vectors to be local to spline point
        const int32 nextIndex            = (Index + 1 < SplineComponent->GetNumberOfSplinePoints()) ? (Index + 1) : Index;
        const int32 previousIndex        = (Index > 0)                                              ? (Index - 1) : Index;
        const FVector nextIndexDirection = SplineComponent->GetDirectionAtSplinePoint(nextIndex, LocalSpace);
        const FVector prevIndexDirection = SplineComponent->GetDirectionAtSplinePoint(previousIndex, LocalSpace);
        const FRotator coordSystem       = FMath::Lerp(prevIndexDirection, nextIndexDirection, 0.5f).Rotation();
        meshInitUpDir                    = coordSystem.RotateVector(meshInitUpDir);
        pointUpDir                       = coordSystem.RotateVector(pointUpDir);
    }

    return meshInitUpDir + pointUpDir;
}

void AFlexSplineActor::SetSplineMeshLocation(const FSplineMeshInitData& MeshInitData, USplineMeshComponent* OutSplineMesh, int32 Index)
{
    if (OutSplineMesh)
    {
        const FSplinePointData& pointData = PointDataArray[Index];
        const int32 nextIndex             = (Index + 1) % SplineComponent->GetNumberOfSplinePoints(); // Need to account for looping here
        const bool bSync                  = GetCanSynchronize(pointData) && (Index > 0);
        auto&& previousPointData          = bSync ? PointDataArray[Index - 1] : FSplinePointData();
        const FName layerName             = GetLayerName(MeshInitData);

        const FVector startTangent             = SplineComponent->GetTangentAtSplinePoint(Index, LocalSpace);
        const FVector endTangent               = SplineComponent->GetTangentAtSplinePoint(nextIndex, LocalSpace);
        FVector startLocation                  = SplineComponent->GetLocationAtSplinePoint(Index, LocalSpace);
        FVector endLocation                    = SplineComponent->GetLocationAtSplinePoint(nextIndex, LocalSpace);
        const FVector randomVectorCurrentIndex = RandomizeVector(MeshInitData.LocationInfo.LocationRandomOffset, Index, layerName);
        const FVector randomVectorNextIndex    = RandomizeVector(MeshInitData.LocationInfo.LocationRandomOffset, nextIndex, layerName);

        if (MeshInitData.LocationInfo.CoordinateSystem == EFlexCoordinateSystem::SplinePoint)
        {
            OutSplineMesh->SetRelativeLocation(FVector::ZeroVector); // Needs to be unset in this config
            const FRotator currentIndexCoordSystem            = SplineComponent->GetDirectionAtSplinePoint(Index, LocalSpace).Rotation();
            const FRotator nextIndexCoordSystem               = SplineComponent->GetDirectionAtSplinePoint(nextIndex, LocalSpace).Rotation();
            const FVector rotatedMeshInitLocationCurrentIndex = currentIndexCoordSystem.RotateVector(MeshInitData.LocationInfo.Location);
            const FVector rotatedMeshInitLocationNextIndex    = nextIndexCoordSystem.RotateVector(MeshInitData.LocationInfo.Location);
            startLocation += (rotatedMeshInitLocationCurrentIndex + randomVectorCurrentIndex);
            endLocation   += (rotatedMeshInitLocationNextIndex    + randomVectorNextIndex);
        }
        else if (MeshInitData.LocationInfo.CoordinateSystem == EFlexCoordinateSystem::SplineSystem)
        {
            OutSplineMesh->SetRelativeLocation(MeshInitData.LocationInfo.Location + randomVectorCurrentIndex);
        }

        OutSplineMesh->SetStartAndEnd(startLocation, startTangent, endLocation, endTangent);
        OutSplineMesh->SetStartOffset(bSync ? previousPointData.EndOffset : pointData.StartOffset);
        OutSplineMesh->SetEndOffset(pointData.EndOffset);
    }
}

UStaticMeshComponent* AFlexSplineActor::CreateMeshComponent(UClass* MeshType, FSplineMeshInitData& MeshInitData, int32 Index /*= -1*/)
{
    UStaticMeshComponent* newMesh = NewObject<UStaticMeshComponent>(this, MeshType);
    newMesh->RegisterComponent();
    newMesh->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);

    if (Index < 0)
    {
        MeshInitData.MeshComponentsArray.Add(newMesh);
    }
    else
    {
        MeshInitData.MeshComponentsArray.Insert(newMesh, Index);
    }

    return newMesh;
}

UArrowComponent* AFlexSplineActor::CreateArrrowComponent(FSplineMeshInitData& MeshInitData)
{
    UArrowComponent* newArrow = NewObject<UArrowComponent>(RootComponent);
    newArrow->RegisterComponent();
    newArrow->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);
    newArrow->SetHiddenInGame(true);
    newArrow->ArrowSize = UpDirectionArrowSize;
    MeshInitData.ArrowSplineUpIndicatorArray.Add(newArrow);

    return newArrow;
}

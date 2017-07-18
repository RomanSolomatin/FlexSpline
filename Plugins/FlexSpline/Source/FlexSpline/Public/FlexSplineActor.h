/*****************************************************************************
* Copyright (C) 2017 Oliver Hawk - All Rights Reserved
*
* @Author       Oliver Hawk
* @EMail        *************************
* @Package      Flex Spline
******************************************************************************/

#pragma once

#include "GameFramework/Actor.h"
#include "FlexSplineTypes.h"
#include "FlexSplineActor.generated.h"

using WeakStaticMeshComp = TWeakObjectPtr<class UStaticMeshComponent>;
using WeakArrowComp      = TWeakObjectPtr<class UArrowComponent>;


/** Generic (XYZ - )Axis Type */
UENUM(BlueprintType)
enum class EFlexSplineAxis : uint8
{
      X
    , Y
    , Z
};

/** Defines relative coordinate system for meshes (e.g. for location, rotation...) */
UENUM(BlueprintType)
enum class EFlexCoordinateSystem : uint8
{
    /** Use coordinates local to the related spline point */
      SplinePoint
    /** Use coordinates local to the entire blueprint instance */
    , SplineSystem
};

/** Generically defines where given configurations apply */
UENUM(BlueprintType)
enum class EFlexGlobalConfigType : uint8
{
    /** Force configuration for all instances to be true */
      Everywhere
    /** Force configuration for all instances to be false */
    , Nowhere
    /** Instances decide for themselves */
    , Custom
};

/** What mesh type to use for flex spline init data */
UENUM(BlueprintType)
enum class EFlexSplineMeshType : uint8
{
    /** Deforms along with spline */
      SplineMesh
    /** Retains its form, gets placed along spline */
    , StaticMesh
};

/** At what place of the spline should a mesh be rendered */
UENUM(BlueprintType, meta = (Bitflags))
enum class EFlexSplineRenderMode : uint8
{
    /** The first spline point */
      Head
    /** The last spline point */
    , Tail
    /** Everything between the first and the last spline point */
    , Middle
    /** Every spline point specified by "Render Mode Custom Indices" */
    , Custom
};

/** Controls miscellaneous settings of mesh layers */
UENUM(BlueprintType, meta = (Bitflags))
enum class EFlexGeneralFlags : uint8
{
    /** Set Mesh Layer Visibility  */
      Active
    /** Enable Looping for this Mesh Layer */
    , Loop
};


USTRUCT(BlueprintType)
struct FFlexMeshInfo
{
    GENERATED_BODY()

    /** How should the mesh be rendered? */
    UPROPERTY(EditAnywhere, Category = FlexSpline)
    EFlexSplineMeshType MeshType;

    /** Which axis of the mesh should be defined as its front? Only relevant for spline meshes */
    UPROPERTY(EditAnywhere, Category = FlexSpline)
    EFlexSplineAxis MeshForwardAxis;

    /** Visual representation and collision */
    UPROPERTY(EditAnywhere, Category = FlexSpline)
    UStaticMesh* Mesh;

    /** Material override for mesh. If nulled, mesh resets to its default material */
    UPROPERTY(EditAnywhere, Category = FlexSpline)
    UMaterialInterface* MeshMaterial;

    FFlexMeshInfo(EFlexSplineAxis InForwardAxis = EFlexSplineAxis::X, EFlexSplineMeshType InType = EFlexSplineMeshType::SplineMesh)
        : MeshType(InType)
        , MeshForwardAxis(InForwardAxis)
        , Mesh(nullptr)
        , MeshMaterial(nullptr)
        { }
};

USTRUCT(BlueprintType)
struct FFlexRenderInfo
{
    GENERATED_BODY()

    /** Let mesh be spawned linearly or randomly according to its spawn chance? */
    UPROPERTY(EditAnywhere, Category = FlexSpline)
    uint32 bRandomizeSpawnChance : 1;

    /** How likely is mesh to spawn on spline point? */
    UPROPERTY(EditAnywhere, Category = FlexSpline, meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float SpawnChance;

    /** At what places of the spline should this mesh be rendered? */
    UPROPERTY(EditAnywhere, Category = FlexSpline, meta = (Bitmask, BitmaskEnum = "EFlexSplineRenderMode"))
    int32 RenderMode;

    /** Define indices at which to render the mesh. Only used if "Custom" Render Mode is active */
    UPROPERTY(EditAnywhere, Category = FlexSpline)
    TSet<uint32> RenderModeCustomIndices;

    FFlexRenderInfo(float InSpawnChance = 1.f, bool bInRandomizeSpawnChance = true)
        : bRandomizeSpawnChance(bInRandomizeSpawnChance)
        , SpawnChance(InSpawnChance)
        , RenderMode(0)
    {
        SET_BIT(RenderMode, EFlexSplineRenderMode::Head);
        SET_BIT(RenderMode, EFlexSplineRenderMode::Tail);
        SET_BIT(RenderMode, EFlexSplineRenderMode::Middle);
    }
};

USTRUCT(BlueprintType)
struct FFlexPhysicsInfo
{
    GENERATED_BODY()

    /** Set collision for this layer. Collision needs to be globally activated for this to take effect */
    UPROPERTY(EditAnywhere, Category = FlexSpline)
    TEnumAsByte<ECollisionEnabled::Type> Collision;

    /** What collision preset to use for this mesh type */
    UPROPERTY(EditAnywhere, Category = FlexSpline)
    FName CollisionProfileName;

    /** Overlap when collision is active? */
    UPROPERTY(EditAnywhere, Category = FlexSpline)
    uint32 bGenerateOverlapEvent : 1;

    FFlexPhysicsInfo(ECollisionEnabled::Type InCollision = ECollisionEnabled::QueryOnly,
                     FName InCollisionProfileName = "BlockAll",
                     bool bInGenerateOverlapEvent = false)
        : Collision(InCollision)
        , CollisionProfileName(InCollisionProfileName)
        , bGenerateOverlapEvent(bInGenerateOverlapEvent)
    {
    }
};

USTRUCT(BlueprintType)
struct FFlexRotationInfo
{
    GENERATED_BODY()

    /** Choose relative coordinate system */
    UPROPERTY(EditAnywhere, Category = FlexSpline)
    EFlexCoordinateSystem CoordinateSystem;

    /** Rotation relative to chosen coordinate system  */
    UPROPERTY(EditAnywhere, Category = FlexSpline)
    FRotator Rotation;

    /** Randomize rotation, seeded */
    UPROPERTY(EditAnywhere, Category = FlexSpline)
    FRotator RotationRandomOffset;

    FFlexRotationInfo(FRotator InRotation = FRotator::ZeroRotator, FRotator InRotationRandomOffset = FRotator::ZeroRotator)
        : CoordinateSystem(EFlexCoordinateSystem::SplinePoint)
        , Rotation(InRotation)
        , RotationRandomOffset(InRotationRandomOffset)
    {
    }
};

USTRUCT(BlueprintType)
struct FFlexLocationInfo
{
    GENERATED_BODY()

    /** Choose relative coordinate system */
    UPROPERTY(EditAnywhere, Category = FlexSpline)
    EFlexCoordinateSystem CoordinateSystem;

    /** Location relative to chosen coordinate system */
    UPROPERTY(EditAnywhere, Category = FlexSpline)
    FVector Location;

    /** Randomize location, seeded */
    UPROPERTY(EditAnywhere, Category = FlexSpline)
    FVector LocationRandomOffset;

    FFlexLocationInfo(FVector InLocation = FVector::ZeroVector, FVector InLocationRandomOffset = FVector::ZeroVector)
        : CoordinateSystem(EFlexCoordinateSystem::SplinePoint)
        , Location(InLocation)
        , LocationRandomOffset(InLocationRandomOffset)
    {
    }
};

USTRUCT(BlueprintType)
struct FFlexScaleInfo
{
    GENERATED_BODY()

    /** If active, "Uniform Scale" will be used to determine scale" */
    UPROPERTY(EditAnywhere, Category = FlexSpline)
    uint32 bUseUniformScale : 1;

    /** Set X, Y and Z scale value simultaneously */
    UPROPERTY(EditAnywhere, Category = FlexSpline, meta = (ClampMin = "0.0", EditCondition = "bUseUniformScale"))
    float UniformScale;

    /** Scale, relative to spline point */
    UPROPERTY(EditAnywhere, Category = FlexSpline, meta = (EditCondition = "!bUseUniformScale"))
    FVector Scale;

    /** If active, "Uniform Scale Random Offset" will be used to determine scale offset" */
    UPROPERTY(EditAnywhere, Category = FlexSpline)
    uint32 bUseUniformScaleRandomOffset : 1;

    /** Randomize scale, seeded. The X,Y and Z offsets will always have the same value */
    UPROPERTY(EditAnywhere, Category = FlexSpline, meta = (ClampMin = "0.0", EditCondition = "bUseUniformScaleRandomOffset"))
    float UniformScaleRandomOffset;

    /** Randomize scale, seeded */
    UPROPERTY(EditAnywhere, Category = FlexSpline, meta = (EditCondition = "!bUseUniformScaleRandomOffset"))
    FVector ScaleRandomOffset;


    FFlexScaleInfo(float InUniformScale = 1.f, FVector InScale = FVector(1.f), FVector InScaleRandomOffset = FVector::ZeroVector)
        : bUseUniformScale(true)
        , UniformScale(InUniformScale)
        , Scale(InScale)
        , bUseUniformScaleRandomOffset(true)
        , UniformScaleRandomOffset(0.f)
        , ScaleRandomOffset(InScaleRandomOffset)
    {
    }
};

USTRUCT(BlueprintType)
struct FFlexUpVectorInfo
{
    GENERATED_BODY()

    /** Editor feature: Should the up vector for each spline point for this mesh be displayed? */
    UPROPERTY(EditAnywhere, Category = FlexSpline)
    uint32 bShowUpDirection : 1;

    /** Choose local coordinate system */
    UPROPERTY(EditAnywhere, Category = FlexSpline)
    EFlexCoordinateSystem CoordinateSystem;

    /** Up direction for all spline meshes of this kind */
    UPROPERTY(EditAnywhere, Category = FlexSpline, meta = (DisplayName = "Up Direction"))
    FVector CustomMeshUpDirection;

    FFlexUpVectorInfo(bool bInShowUpDirection = false, FVector InCustomMeshUpDirection = FVector(0.f,0.f,1.f))
        : bShowUpDirection(bInShowUpDirection)
        , CoordinateSystem(EFlexCoordinateSystem::SplineSystem)
        , CustomMeshUpDirection(InCustomMeshUpDirection)
    {
    }
};


/**
* Stores info on what meshes and which default values on each spline point are initialized
*/
USTRUCT(BlueprintType)
struct FSplineMeshInitData
{
    GENERATED_BODY()


    /** General Mesh Layer settings. Only apply if correspondent global settings are set to Custom */
    UPROPERTY(EditAnywhere, Category = FlexSpline, meta = (Bitmask, BitmaskEnum = "EFlexGeneralFlags", DisplayName = "General"))
    int32 GeneralInfo;

    /** Mesh information */
    UPROPERTY(EditAnywhere, Category = FlexSpline, meta = (DisplayName = "Mesh"))
    FFlexMeshInfo MeshInfo;

    /** Rendering configuration */
    UPROPERTY(EditAnywhere, Category = FlexSpline, meta = (DisplayName = "Rendering"))
    FFlexRenderInfo RenderInfo;

    /** Configuration of physics and collision */
    UPROPERTY(EditAnywhere, Category = FlexSpline, meta = (DisplayName = "Physics"))
    FFlexPhysicsInfo PhysicsInfo;

    /** Rotation control relative to spline point */
    UPROPERTY(EditAnywhere, Category = FlexSpline, meta = (DisplayName = "Rotation"))
    FFlexRotationInfo RotationInfo;

    /** Location control relative to spline point */
    UPROPERTY(EditAnywhere, Category = FlexSpline, meta = (DisplayName = "Location"))
    FFlexLocationInfo LocationInfo;

    /** Scale control relative to spline point */
    UPROPERTY(EditAnywhere, Category = FlexSpline, meta = (DisplayName = "Scale"))
    FFlexScaleInfo ScaleInfo;

    /** Up vector control relative to spline point */
    UPROPERTY(EditAnywhere, Category = FlexSpline, meta = (DisplayName = "Up Vector"))
    FFlexUpVectorInfo UpVectorInfo;


    /**
    * Stores all spline mesh components, driven by data from this instance
    * Each mesh is associated to a spline point via its index
    */
    TArray<WeakStaticMeshComp> MeshComponentsArray;

    /** Shows the spline up vector at each spline point */
    TArray<WeakArrowComp> ArrowSplineUpIndicatorArray;


    FSplineMeshInitData()
        : bTemplatedInitialized(false)
    {
        SET_BIT(GeneralInfo, EFlexGeneralFlags::Active);
    }

    /** Delete all spline meshes and arrows on destruction */
    ~FSplineMeshInitData();

    bool operator==(const FSplineMeshInitData& Other) const
    {
        return this == &Other;
    }

    bool IsInitialized() const { return bTemplatedInitialized; }
    void Initialize() { bTemplatedInitialized = true; }


private:

    /** Has this init data been initialized with given template? Use Initialize() to confirm */
    UPROPERTY()
    uint32 bTemplatedInitialized : 1;
};


/**
* Stores data for each spline point, may override initial data
*/
USTRUCT(BlueprintType)
struct FSplinePointData
{
    GENERATED_BODY()

// ============================= SPLINE MESH FEATURES

    /** Only editable if not synchronized with previous point */
    UPROPERTY()
    float StartRoll;

    UPROPERTY()
    float EndRoll;

    /** Only editable if not synchronized with previous point */
    UPROPERTY()
    FVector2D StartScale;

    UPROPERTY()
    FVector2D EndScale;

    /** Only editable if not synchronized with previous point */
    UPROPERTY()
    FVector2D StartOffset;

    UPROPERTY()
    FVector2D EndOffset;

    /** Up direction for all spline meshes of this point */
    UPROPERTY()
    FVector CustomPointUpDirection;

    /**
    * If this is active, the spline at this point will deform its start values
    * to match the last point's end values. Start values will be overridden
    */
    UPROPERTY()
    bool bSynchroniseWithPrevious;


    // ============================= STATIC MESH FEATURES

    UPROPERTY()
    FVector SMLocationOffset;

    UPROPERTY()
    FVector SMScale;

    UPROPERTY()
    FRotator SMRotation;


    /** Displays the index for the associated spline point */
    UPROPERTY()
    class UTextRenderComponent* IndexTextRenderer;

    /** Unique identifier, hash-value */
    uint32 ID;

    /** CONSTRUCTOR */
    FSplinePointData()
        : StartRoll(0.f)
        , EndRoll(0.f)
        , StartScale(1.f, 1.f)
        , EndScale(1.f, 1.f)
        , StartOffset(0.f, 0.f)
        , EndOffset(0.f, 0.f)
        , CustomPointUpDirection(0.f)
        , bSynchroniseWithPrevious(true)
        , SMLocationOffset(0.f)
        , SMScale(0.f)
        , SMRotation(0.f)
        , IndexTextRenderer(nullptr)
        , ID(0)
        {
        }
};



/**
* This Actor contains a spline component that can be flexibly configured on a per mesh
* or per spline-point basis. Multiple meshes can be placed along the spline either
* as a spline mesh or retain their form as a static mesh
*/
UCLASS(HideCategories = ("Actor Tick", Rendering, Input, Actor))
class FLEXSPLINE_API AFlexSplineActor : public AActor
{
    GENERATED_BODY()

public:

    AFlexSplineActor();
    void OnConstruction(const FTransform& Transform) override;
    void PreInitializeComponents() override;

    int32 GetMeshCountForType(EFlexSplineMeshType MeshType) const;


protected:

    /** Spawns and initiates spline mesh components for each spline point */
    void ConstructSplineMesh();

    /** If mesh data has just been created initialize it with template */
    void InitializeNewMeshData();

    /** Create new point data if theres a new spline point */
    void AddPointDataEntries();

    /** Remove point data associated with deleted spline point */
    void RemovePointDataEntries(const TArray<int32>& DeletedIndices);

    /** Create new mesh components if there are fewer meshes than spline points */
    void InitDataAddMeshes();

    /** Remove mesh components if there are more meshes than spline points */
    void InitDataRemoveMeshes(const TArray<int32>& DeletedIndices);

    /** Bring point data identifiers up to date */
    void UpdatePointData();

    /** Adjust text renderer position and text according to points and meshes */
    void UpdateDebugInformation();


    /** Set mesh values according to mesh and point data */
    void UpdateMeshComponents();

    /** Called by UpdateMeshComponents, specialized for spline meshes */
    void UpdateSplineMesh(const FSplineMeshInitData& MeshInitData, class USplineMeshComponent* SplineMesh,
                          int32 CurrentIndex);

    /** Called by UpdateMeshComponents, specialized for static meshes */
    void UpdateStaticMesh(const FSplineMeshInitData& MeshInitData, class UStaticMeshComponent* StaticMesh,
                          int32 CurrentIndex);


protected:

    /** Return data's layer name, if available */
    FName GetLayerName(const FSplineMeshInitData& MeshInitData) const;

    /** Find and return all indices of spline mesh that were deleted since last update */
    void GetDeletedIndices(TArray<int32>& OutIndexArray) const;

    /** Find best position for the text renderer at this index */
    FVector GetTextPosition(int32 Index) const;

    /** Is rendering allowed, given the current index? */
    bool CanRenderFromMode(const FSplineMeshInitData& MeshInitData, int32 CurrentIndex, int32 FinalIndex) const;

    /** Find appropriate collision taking Mesh Layer and Flex Spline config into account */
    ECollisionEnabled::Type GetCollisionEnabled(const FSplineMeshInitData& MeshInitData) const;

    /** See if looping is enabled globally and for given mesh data */
    bool GetCanLoop(const FSplineMeshInitData& MeshInitData) const;

    /** Find out if current spline point should be synchronized */
    bool GetCanSynchronize(const FSplinePointData& PointData) const;

    /** Compute location for mesh according to spline, point and layer information, using the configured coordinate system */
    FVector CalculateLocation(const FSplineMeshInitData& MeshInitData, const FSplinePointData& PointData, const int32 Index) const;

    /** Compute rotation for mesh according to spline, point and layer information, using the configured coordinate system */
    FRotator CalculateRotation(const FSplineMeshInitData& MeshInitData, const FSplinePointData& PointData, const int32 index) const;

    /** Compute scale for mesh according to spline, point and layer information*/
    FVector CalculateScale(const FSplineMeshInitData& MeshInitData, const FSplinePointData& PointData, const int32 Index) const;

    /** Get up direction for spline according to chosen local space */
    FVector CalculateUpDirection(const FSplineMeshInitData& MeshInitData, const FSplinePointData& PointData, const int32 Index) const;

    /** Calculate location for spline mesh and apply to param outSplineMesh */
    void SetSplineMeshLocation(const FSplineMeshInitData& MeshInitData, class USplineMeshComponent* outSplineMesh, int32 Index);

    /**
    * Create a new mesh component of class meshType, add to mesh init data array.
    * If no valid index is specified, it is appended
    */
    class UStaticMeshComponent* CreateMeshComponent(UClass* MeshType, FSplineMeshInitData& MeshInitData, int32 Index = -1);

    /** Create arrow component, add to Actor root, cache inside @param MeshInitData */
    class UArrowComponent* CreateArrrowComponent(FSplineMeshInitData& MeshInitData);


protected:

    UPROPERTY(VisibleAnywhere, Category = "FlexSpline", meta = (AllowPrivateAccess = "true"))
    class USplineComponent* SplineComponent;


protected:

    /**
    * Sets all collisions Active, Inactive or Defined per Mesh Layer (see PhysicsInfo -> Collision)
    */
    UPROPERTY(EditAnywhere, Category = "FlexSpline|Global")
    EFlexGlobalConfigType CollisionActive;

    /**
    * Allow spline points to synchronize their start values with the previous point's
    * end values. Can be configured per spline point.
    */
    UPROPERTY(EditAnywhere, Category = "FlexSpline|Global")
    EFlexGlobalConfigType Synchronize;

    /** Should spline bite its own tail? */
    UPROPERTY(EditAnywhere, Category = "FlexSpline|Global")
    EFlexGlobalConfigType Loop;

    /** Blueprint for new "Mesh Layer" entries */
    UPROPERTY(EditAnywhere, Category = "FlexSpline|Global", meta = (DisplayName = "Mesh Layer Template"))
    FSplineMeshInitData MeshDataTemplate;


    /** Should the index for each spline point be displayed? */
    UPROPERTY(EditAnywhere, AdvancedDisplay, Category = "FlexSpline")
    bool bShowPointNumbers;

    /** Spline index text renderer size */
    UPROPERTY(EditAnywhere, AdvancedDisplay, Category = "FlexSpline", meta = (ClampMin = "0.0", UIMax = "500.0"))
    float PointNumberSize;

    /** Debug up direction arrow component size */
    UPROPERTY(EditAnywhere, AdvancedDisplay, Category = "FlexSpline", meta = (ClampMin = "0.0", UIMax = "10.0"))
    float UpDirectionArrowSize;

    /** Debug up direction arrow vertical offset for better visibility */
    UPROPERTY(EditAnywhere, AdvancedDisplay, Category = "FlexSpline", meta = (ClampMin = "0.0", UIMax = "200.0"))
    float UpDirectionArrowOffset;

    /** Color of the spline point text renderers */
    UPROPERTY(EditAnywhere, AdvancedDisplay, Category = "FlexSpline")
    FColor TextRenderColor;


    /**
    * Mesh configuration for each spline point, resizes automatically
    */
    UPROPERTY()
    TArray<FSplinePointData> PointDataArray;

    /** Stores all meshes(and related info) that should be spawned per spline point */
    UPROPERTY(EditAnywhere, Category = "FlexSpline", meta = (DisplayName = "Mesh Layers", NoElementDuplicate))
    TMap<FName, FSplineMeshInitData> MeshDataInitMap;


private:

    /** Cache lastly generated MeshDataInitMap key to circumvent strange engine behavior */
    FName LastUsedKey;

    /** Details customizer class needs access to all members */
    friend class FFlexSplineNodeBuilder;
};

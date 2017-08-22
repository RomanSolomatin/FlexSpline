/*****************************************************************************
* Copyright (C) 2017 Oliver Hawk - All Rights Reserved
*
* @Author       Oliver Hawk
* @EMail        *************************
* @Package      Flex Spline
******************************************************************************/

#include "FlexSplineDetailsPrivatePCH.h"
#include "FlexSplineDetails.h"
#include "FlexSplineActor.h"
// Engine includes
#include "Math/UnitConversion.h"
#include "IDocumentation.h"
#include "IDetailCustomNodeBuilder.h"
#include "IPropertyUtilities.h"
#include "DetailLayoutBuilder.h"
#include "IDetailGroup.h"
#include "IDetailChildrenBuilder.h"
#include "DetailWidgetRow.h"
#include "DetailCategoryBuilder.h"
#include "SplineComponentVisualizer.h"
#include "ScopedTransaction.h"
#include "Editor.h"
#include "Editor/UnrealEdEngine.h"
#include "SNumericEntryBox.h"
#include "SRotatorInputBox.h"
#include "NumericUnitTypeInterface.inl"
#include "SCheckBox.h"
#include "SBox.h"
#include "STextBlock.h"
#include "Components/SplineComponent.h"
#include "UnrealEdGlobals.h"
#include "SSCSEditor.h"
#include "InputBoxes/FlexVectorInputBox.h"


class FFlexSplineNodeBuilder;
using WeakSplineComponent = TWeakObjectPtr<USplineComponent>;
using SetSliderFuncPtr    = void (FFlexSplineNodeBuilder::*)(float NewValue, EAxis::Type Axis, AFlexSplineActor* FlexSpline);

#define LOCTEXT_NAMESPACE "FlexSplineDetails"

static constexpr float SpinboxDelta       = 0.01f;
static constexpr float SingleSpinboxWidth = 110.f;
static constexpr float DoubleSpinboxWidth = SingleSpinboxWidth * 2.f;
static constexpr float TripleSpinboxWidth = SingleSpinboxWidth * 3.f;
static const FText MultipleValuesText     = LOCTEXT("MultVal", "Multiple Values");
static const FText SyncTooltipText        = LOCTEXT("SyncTip", "Only Editable If Not Synchronized");
static const FText GlobalSyncTooltipText  = LOCTEXT("GlobalSyncTip", "Only Editable If Snychronisation Is Marked As Custom");
static const FText NoSelectionText        = LOCTEXT("NoPointsSelected", "No Flex Spline Points Are Selected");
static const FText NoSplineMeshesText     = LOCTEXT("NoSplineMeshes", "There Are No Active Spline Meshes To Edit");
static const FText NoStaticMeshesText     = LOCTEXT("NoStaticMeshes", "There Are No Active Static Meshes To Edit");


struct FSetSliderAdditionalArgs
{
    FSetSliderAdditionalArgs(SetSliderFuncPtr InImpl, FText InTransactionMessage = FText(), EAxis::Type InAxis = EAxis::None, bool bInCommited = false)
        : Impl(InImpl)
        , TransactionMessage(InTransactionMessage)
        , Axis(InAxis)
        , bCommited(bInCommited)
    {}

    SetSliderFuncPtr Impl;
    FText TransactionMessage;
    EAxis::Type Axis;
    bool bCommited;
};

enum class ESliderMode : uint8
{
    BeginSlider,
    EndSlider
};

static const TArray<FText> TransactionTexts = {
    // [0]
    LOCTEXT("SetSplinePointStartRoll", "Set Flex Spline Point Start Roll")
    // [1]
    ,LOCTEXT("SetSplinePointStartScale", "Set Flex Spline Point Start Scale")
    // [2]
    ,LOCTEXT("SetSplinePointStartOffset", "Set Flex Spline Point Start Offset")
    // [3]
    ,LOCTEXT("SetSplinePointEndRoll", "Set Flex Spline Point End Roll")
    // [4]
    ,LOCTEXT("SetSplinePointEndScale", "Set Flex Spline Point End Scale")
    // [5]
    ,LOCTEXT("SetSplinePointEndOffset", "Set Flex Spline Point End Offset")
    // [6]
    ,LOCTEXT("SetUpDir", "Set Flex Spline Point Up Direction")
    // [7]
    ,LOCTEXT("SetSync", "Set Flex Spline Point Synchronisation")
    // [8]
    ,LOCTEXT("SetSMLoc", "Set Flex Spline Point Static Mesh Location Offset")
    // [9]
    ,LOCTEXT("SetSMScale", "Set Flex Spline Point Static Mesh Scale")
    // [10]
    ,LOCTEXT("SetSMRotation", "Set Flex Spline Point Static Mesh Rotation")
};




class FFlexSplineNodeBuilder : public IDetailCustomNodeBuilder, public TSharedFromThis<FFlexSplineNodeBuilder>
{
public:

    FFlexSplineNodeBuilder();

    //~ Begin IDetailCustomNodeBuilder interface
    void SetOnRebuildChildren(FSimpleDelegate InOnRegenerateChildren) override { };
    void GenerateHeaderRowContent(FDetailWidgetRow& NodeRow) override { };
    void GenerateChildContent(IDetailChildrenBuilder& ChildrenBuilder) override;
    void Tick(float DeltaTime) override;
    bool RequiresTick() const override { return true; }
    bool InitiallyCollapsed() const override { return false; }
    FName GetName() const override;
    //~ End IDetailCustomNodeBuilder interface
    FNotifyHook* NotifyHook;
    IDetailLayoutBuilder* DetailBuilder;

private:

    template <typename T>
    struct TSharedValue
    {
        TSharedValue() : bInitialized(false) {}

        void Reset()
        {
            bInitialized = false;
        }

        void Add(T InValue)
        {
            if (!bInitialized)
            {
                Value = InValue;
                bInitialized = true;
            }
            else
            {
                if (Value.IsSet() && InValue != Value.GetValue()) { Value.Reset(); }
            }
        }

        TOptional<T> Value;
        bool bInitialized;
    };

    struct FSharedVector2DValue
    {
        FSharedVector2DValue() : bInitialized(false) {}

        void Reset() { bInitialized = false; }

        bool IsValid() const { return bInitialized; }

        void Add(const FVector2D& V)
        {
            if (!bInitialized)
            {
                X = V.X;
                Y = V.Y;
                bInitialized = true;
            }
            else
            {
                if (X.IsSet() && V.X != X.GetValue()) { X.Reset(); }
                if (Y.IsSet() && V.Y != Y.GetValue()) { Y.Reset(); }
            }
        }

        TOptional<float> X;
        TOptional<float> Y;
        bool bInitialized;
    };

    struct FSharedVectorValue
    {
        FSharedVectorValue() : bInitialized(false) {}

        void Reset()
        {
            bInitialized = false;
        }

        bool IsValid() const { return bInitialized; }

        void Add(const FVector& V)
        {
            if (!bInitialized)
            {
                X = V.X;
                Y = V.Y;
                Z = V.Z;
                bInitialized = true;
            }
            else
            {
                if (X.IsSet() && V.X != X.GetValue()) { X.Reset(); }
                if (Y.IsSet() && V.Y != Y.GetValue()) { Y.Reset(); }
                if (Z.IsSet() && V.Z != Z.GetValue()) { Z.Reset(); }
            }
        }

        TOptional<float> X;
        TOptional<float> Y;
        TOptional<float> Z;
        bool bInitialized;
    };

    struct FSharedRotatorValue
    {
        FSharedRotatorValue() : bInitialized(false) {}

        void Reset()
        {
            bInitialized = false;
        }

        bool IsValid() const { return bInitialized; }

        void Add(const FRotator& R)
        {
            if (!bInitialized)
            {
                Roll = R.Roll;
                Pitch = R.Pitch;
                Yaw = R.Yaw;
                bInitialized = true;
            }
            else
            {
                if (Roll.IsSet() && R.Roll != Roll.GetValue()) { Roll.Reset(); }
                if (Pitch.IsSet() && R.Pitch != Pitch.GetValue()) { Pitch.Reset(); }
                if (Yaw.IsSet() && R.Yaw != Yaw.GetValue()) { Yaw.Reset(); }
            }
        }

        TOptional<float> Roll;
        TOptional<float> Pitch;
        TOptional<float> Yaw;
        bool bInitialized;
    };


    TSharedRef<SWidget> BuildNotVisibleMessage(EFlexSplineMeshType MeshType) const;
    FText GetNoSelectionText(EFlexSplineMeshType MeshType) const;
    bool IsSyncDisabled() const;
    bool IsSyncGloballyEnabled() const;
    AFlexSplineActor* GetFlexSpline() const;
    bool IsFlexSplineSelected() const { return !!Cast<AFlexSplineActor>(SplineComp.IsValid() ? SplineComp->GetOwner() : nullptr); }

    EVisibility ShowVisible(EFlexSplineMeshType MeshType) const;
    EVisibility ShowNotVisible(EFlexSplineMeshType MeshType) const;
    EVisibility ShowVisibleSpline()    const { return ShowVisible(EFlexSplineMeshType::SplineMesh); }
    EVisibility ShowNotVisibleSpline() const { return ShowNotVisible(EFlexSplineMeshType::SplineMesh); }
    EVisibility ShowVisibleStatic()    const { return ShowVisible(EFlexSplineMeshType::StaticMesh); }
    EVisibility ShowNotVisibleStatic() const { return ShowNotVisible(EFlexSplineMeshType::StaticMesh); }

    TOptional<float> GetStartRoll() const { return StartRoll.Value; }
    TOptional<float> GetStartScale(EAxis::Type Axis) const;
    TOptional<float> GetStartOffset(EAxis::Type Axis) const;
    TOptional<float> GetEndRoll() const { return EndRoll.Value; }
    TOptional<float> GetEndScale(EAxis::Type Axis) const;
    TOptional<float> GetEndOffset(EAxis::Type Axis) const;
    TOptional<float> GetUpDirection(EAxis::Type Axis) const;
    ECheckBoxState GetSynchroniseWithPrevious() const;
    TOptional<float> GetSMLocationOffset(EAxis::Type Axis) const;
    TOptional<float> GetSMScale(EAxis::Type Axis) const;
    TOptional<float> GetSMRotation(EAxis::Type Axis) const;

    void OnSliderAction(float NewValue, ESliderMode SliderMode, FText TransactionMessage);
    void OnSetFloatSliderValue(float NewValue, ETextCommit::Type CommitInfo, FSetSliderAdditionalArgs Args);

    void OnSetStartRoll(float NewValue, EAxis::Type Axis, AFlexSplineActor* FlexSpline);
    void OnSetStartScale(float NewValue, EAxis::Type Axis, AFlexSplineActor* FlexSpline);
    void OnSetStartOffset(float NewValue, EAxis::Type Axis, AFlexSplineActor* FlexSpline);
    void OnSetEndRoll(float NewValue, EAxis::Type Axis, AFlexSplineActor* FlexSpline);
    void OnSetEndScale(float NewValue, EAxis::Type Axis, AFlexSplineActor* FlexSpline);
    void OnSetEndOffset(float NewValue, EAxis::Type Axis, AFlexSplineActor* FlexSpline);
    void OnSetUpDirection(float NewValue, EAxis::Type Axis, AFlexSplineActor* FlexSpline);
    void OnCheckedChangedSynchroniseWithPrevious(ECheckBoxState NewState);
    void OnSetSMLocationOffset(float NewValue, EAxis::Type Axis, AFlexSplineActor* FlexSpline);
    void OnSetSMScale(float NewValue, EAxis::Type Axis, AFlexSplineActor* FlexSpline);
    void OnSetSMRotation(float NewValue, EAxis::Type Axis, AFlexSplineActor* FlexSpline);

    void UpdateValues();
    void NotifyPreChange(AFlexSplineActor* FlexSplineActor);
    void NotifyPostChange(AFlexSplineActor* FlexSplineActor);

    WeakSplineComponent SplineComp;
    TSet<int32> SelectedKeys;
    FSplineComponentVisualizer* SplineVisualizer;

    TSharedValue<float> StartRoll;
    FSharedVector2DValue StartScale;
    FSharedVector2DValue StartOffset;
    TSharedValue<float> EndRoll;
    FSharedVector2DValue EndScale;
    FSharedVector2DValue EndOffset;
    FSharedVectorValue UpDirection;
    TSharedValue<bool> SynchroniseWithPrevious;
    FSharedVectorValue SMLocationOffset;
    FSharedVectorValue SMScale;
    FSharedRotatorValue SMRotation;
};


//////////////////////////////////////////////////////////////////////////
// FlexSplineNodeBuilder CONSTRUCTOR & BASE INTERFACE
FFlexSplineNodeBuilder::FFlexSplineNodeBuilder()
    : NotifyHook(nullptr)
    , DetailBuilder(nullptr)
{
    TSharedPtr<FComponentVisualizer> visualizer = GUnrealEd->FindComponentVisualizer(USplineComponent::StaticClass());
    SplineVisualizer = static_cast<FSplineComponentVisualizer*>(visualizer.Get());
    check(SplineVisualizer);
}

void FFlexSplineNodeBuilder::GenerateChildContent(IDetailChildrenBuilder& ChildrenBuilder)
{
    const auto FontInfo = IDetailLayoutBuilder::GetDetailFont();
    auto typeInterface = MakeShareable(new TNumericUnitTypeInterface<float>(EUnit::Degrees));

    IDetailGroup& splineGroup = ChildrenBuilder.AddGroup("SplineGroup", LOCTEXT("SplineMeshGroup", "Point Spline-Mesh Config"));
    // Message which is shown when no points are selected
    splineGroup.AddWidgetRow()
        .Visibility(TAttribute<EVisibility>(this, &FFlexSplineNodeBuilder::ShowNotVisibleSpline))
        [
            BuildNotVisibleMessage(EFlexSplineMeshType::SplineMesh)
        ];

#pragma region Start Roll
    splineGroup.AddWidgetRow()
        .Visibility(TAttribute<EVisibility>(this, &FFlexSplineNodeBuilder::ShowVisibleSpline))
        .IsEnabled(TAttribute<bool>(this, &FFlexSplineNodeBuilder::IsSyncDisabled))
        .NameContent()
        .HAlign(HAlign_Left)
        .VAlign(VAlign_Center)
        [
            SNew(STextBlock)
            .Text(LOCTEXT("StartRoll", "Start Roll"))
            .Font(FontInfo)
            .ToolTip(IDocumentation::Get()->CreateToolTip(SyncTooltipText, nullptr, TEXT("Shared/LevelEditor"), TEXT("")))
        ]
        .ValueContent()
        .MinDesiredWidth(SingleSpinboxWidth)
        .MaxDesiredWidth(SingleSpinboxWidth)
        [
            SNew(SNumericEntryBox<float>)
            .Font(FontInfo)
            .UndeterminedString(MultipleValuesText)
            .AllowSpin(true)
            .MinValue(TOptional<float>())
            .MaxValue(TOptional<float>())
            .MinSliderValue(-3.14f)
            .MaxSliderValue(3.14f)
            .Value(this, &FFlexSplineNodeBuilder::GetStartRoll)
            .OnValueCommitted(this, &FFlexSplineNodeBuilder::OnSetFloatSliderValue,
                              FSetSliderAdditionalArgs(&FFlexSplineNodeBuilder::OnSetStartRoll, TransactionTexts[0], EAxis::None, true))
            .OnValueChanged(this, &FFlexSplineNodeBuilder::OnSetFloatSliderValue, ETextCommit::Default,
                            FSetSliderAdditionalArgs(&FFlexSplineNodeBuilder::OnSetStartRoll, TransactionTexts[0]))
            .OnBeginSliderMovement(this, &FFlexSplineNodeBuilder::OnSliderAction, 0.f, ESliderMode::BeginSlider, TransactionTexts[0])
            .OnEndSliderMovement(this, &FFlexSplineNodeBuilder::OnSliderAction, ESliderMode::EndSlider, FText())
        ];
#pragma endregion

#pragma region Start Scale
    splineGroup.AddWidgetRow()
        .Visibility(TAttribute<EVisibility>(this, &FFlexSplineNodeBuilder::ShowVisibleSpline))
        .IsEnabled(TAttribute<bool>(this, &FFlexSplineNodeBuilder::IsSyncDisabled))
        .NameContent()
        .HAlign(HAlign_Left)
        .VAlign(VAlign_Center)
        [
            SNew(STextBlock)
            .Text(LOCTEXT("StartScale", "Start Scale"))
            .Font(FontInfo)
            .ToolTip(IDocumentation::Get()->CreateToolTip(SyncTooltipText, nullptr, TEXT("Shared/LevelEditor"), ""))
        ]
        .ValueContent()
        .MinDesiredWidth(DoubleSpinboxWidth)
        .MaxDesiredWidth(DoubleSpinboxWidth)
        [
            SNew(SFlexVectorInputBox)
            .Font(FontInfo)
            .AllowSpin(true)
            .bColorAxisLabels(true)
            .AllowResponsiveLayout(true)
            .MinValue(0.f)
            .MinSliderValue(0.f)
            .MaxValue(TOptional<float>())
            .MaxSliderValue(TOptional<float>())
            .Delta(SpinboxDelta)
            .OnBeginSliderMovement(this, &FFlexSplineNodeBuilder::OnSliderAction, 0.f, ESliderMode::BeginSlider, TransactionTexts[1])
            .OnEndSliderMovement(this, &FFlexSplineNodeBuilder::OnSliderAction, ESliderMode::EndSlider, FText())
            .X(this, &FFlexSplineNodeBuilder::GetStartScale, EAxis::X)
            .OnXChanged(this, &FFlexSplineNodeBuilder::OnSetFloatSliderValue, ETextCommit::Default,
                        FSetSliderAdditionalArgs(&FFlexSplineNodeBuilder::OnSetStartScale, TransactionTexts[1], EAxis::X))
            .OnXCommitted(this, &FFlexSplineNodeBuilder::OnSetFloatSliderValue,
                          FSetSliderAdditionalArgs(&FFlexSplineNodeBuilder::OnSetStartScale, TransactionTexts[1], EAxis::X, true))
            .Y(this, &FFlexSplineNodeBuilder::GetStartScale, EAxis::Y)
            .OnYChanged(this, &FFlexSplineNodeBuilder::OnSetFloatSliderValue, ETextCommit::Default,
                        FSetSliderAdditionalArgs(&FFlexSplineNodeBuilder::OnSetStartScale, TransactionTexts[1], EAxis::Y))
            .OnYCommitted(this, &FFlexSplineNodeBuilder::OnSetFloatSliderValue,
                          FSetSliderAdditionalArgs(&FFlexSplineNodeBuilder::OnSetStartScale, TransactionTexts[1], EAxis::Y, true))
        ];
#pragma endregion

#pragma region Start Offset
    splineGroup.AddWidgetRow()
        .Visibility(TAttribute<EVisibility>(this, &FFlexSplineNodeBuilder::ShowVisibleSpline))
        .IsEnabled(TAttribute<bool>(this, &FFlexSplineNodeBuilder::IsSyncDisabled))
        .NameContent()
        .HAlign(HAlign_Left)
        .VAlign(VAlign_Center)
        [
            SNew(STextBlock)
            .Text(LOCTEXT("StartOffset", "Start Offset"))
            .Font(FontInfo)
            .ToolTip(IDocumentation::Get()->CreateToolTip(SyncTooltipText, nullptr, TEXT("Shared/LevelEditor"), ""))
        ]
        .ValueContent()
        .MinDesiredWidth(DoubleSpinboxWidth)
        .MaxDesiredWidth(DoubleSpinboxWidth)
        [
            SNew(SFlexVectorInputBox)
            .Font(FontInfo)
            .AllowSpin(true)
            .bColorAxisLabels(true)
            .AllowResponsiveLayout(true)
            .MinValue(TOptional<float>())
            .MinSliderValue(TOptional<float>())
            .MaxValue(TOptional<float>())
            .MaxSliderValue(TOptional<float>())
            .Delta(SpinboxDelta)
            .OnBeginSliderMovement(this, &FFlexSplineNodeBuilder::OnSliderAction, 0.f, ESliderMode::BeginSlider, TransactionTexts[2])
            .OnEndSliderMovement(this, &FFlexSplineNodeBuilder::OnSliderAction, ESliderMode::EndSlider, FText())
            .X(this, &FFlexSplineNodeBuilder::GetStartOffset, EAxis::X)
            .OnXChanged(this, &FFlexSplineNodeBuilder::OnSetFloatSliderValue, ETextCommit::Default,
                        FSetSliderAdditionalArgs(&FFlexSplineNodeBuilder::OnSetStartOffset, TransactionTexts[2], EAxis::X))
            .OnXCommitted(this, &FFlexSplineNodeBuilder::OnSetFloatSliderValue,
                          FSetSliderAdditionalArgs(&FFlexSplineNodeBuilder::OnSetStartOffset, TransactionTexts[2], EAxis::X, true))
            .Y(this, &FFlexSplineNodeBuilder::GetStartOffset, EAxis::Y)
            .OnYChanged(this, &FFlexSplineNodeBuilder::OnSetFloatSliderValue, ETextCommit::Default,
                        FSetSliderAdditionalArgs(&FFlexSplineNodeBuilder::OnSetStartOffset, TransactionTexts[2], EAxis::Y))
            .OnYCommitted(this, &FFlexSplineNodeBuilder::OnSetFloatSliderValue,
                          FSetSliderAdditionalArgs(&FFlexSplineNodeBuilder::OnSetStartOffset, TransactionTexts[2], EAxis::Y, true))
        ];
#pragma endregion

#pragma region End Roll
    splineGroup.AddWidgetRow()
        .Visibility(TAttribute<EVisibility>(this, &FFlexSplineNodeBuilder::ShowVisibleSpline))
        .NameContent()
        .HAlign(HAlign_Left)
        .VAlign(VAlign_Center)
        [
            SNew(STextBlock)
            .Text(LOCTEXT("EndRoll", "End Roll"))
            .Font(FontInfo)
        ]
        .ValueContent()
        .MinDesiredWidth(SingleSpinboxWidth)
        .MaxDesiredWidth(SingleSpinboxWidth)
        [
            SNew(SNumericEntryBox<float>)
            .Font(FontInfo)
            .UndeterminedString(MultipleValuesText)
            .AllowSpin(true)
            .MinValue(TOptional<float>())
            .MaxValue(TOptional<float>())
            .MinSliderValue(-3.14f)
            .MaxSliderValue(3.14f)
            .Value(this, &FFlexSplineNodeBuilder::GetEndRoll)
            .OnValueCommitted(this, &FFlexSplineNodeBuilder::OnSetFloatSliderValue,
                              FSetSliderAdditionalArgs(&FFlexSplineNodeBuilder::OnSetEndRoll, TransactionTexts[3], EAxis::None, true))
            .OnValueChanged(this, &FFlexSplineNodeBuilder::OnSetFloatSliderValue, ETextCommit::Default,
                            FSetSliderAdditionalArgs(&FFlexSplineNodeBuilder::OnSetEndRoll, TransactionTexts[3]))
            .OnBeginSliderMovement(this, &FFlexSplineNodeBuilder::OnSliderAction, 0.f, ESliderMode::BeginSlider, TransactionTexts[3])
            .OnEndSliderMovement(this, &FFlexSplineNodeBuilder::OnSliderAction, ESliderMode::EndSlider, FText())
        ];
#pragma endregion

#pragma region End Scale
    splineGroup.AddWidgetRow()
        .Visibility(TAttribute<EVisibility>(this, &FFlexSplineNodeBuilder::ShowVisibleSpline))
        .NameContent()
        .HAlign(HAlign_Left)
        .VAlign(VAlign_Center)
        [
            SNew(STextBlock)
            .Text(LOCTEXT("EndScale", "End Scale"))
            .Font(FontInfo)
        ]
        .ValueContent()
        .MinDesiredWidth(DoubleSpinboxWidth)
        .MaxDesiredWidth(DoubleSpinboxWidth)
        [
            SNew(SFlexVectorInputBox)
            .Font(FontInfo)
            .AllowSpin(true)
            .bColorAxisLabels(true)
            .AllowResponsiveLayout(true)
            .MinValue(0.f)
            .MinSliderValue(0.f)
            .MaxValue(TOptional<float>())
            .MaxSliderValue(TOptional<float>())
            .Delta(SpinboxDelta)
            .OnBeginSliderMovement(this, &FFlexSplineNodeBuilder::OnSliderAction, 0.f, ESliderMode::BeginSlider, TransactionTexts[4])
            .OnEndSliderMovement(this, &FFlexSplineNodeBuilder::OnSliderAction, ESliderMode::EndSlider, FText())
            .X(this, &FFlexSplineNodeBuilder::GetEndScale, EAxis::X)
            .OnXChanged(this, &FFlexSplineNodeBuilder::OnSetFloatSliderValue, ETextCommit::Default,
                        FSetSliderAdditionalArgs(&FFlexSplineNodeBuilder::OnSetEndScale, TransactionTexts[4], EAxis::X))
            .OnXCommitted(this, &FFlexSplineNodeBuilder::OnSetFloatSliderValue,
                            FSetSliderAdditionalArgs(&FFlexSplineNodeBuilder::OnSetEndScale, TransactionTexts[4], EAxis::X, true))
            .Y(this, &FFlexSplineNodeBuilder::GetEndScale, EAxis::Y)
            .OnYChanged(this, &FFlexSplineNodeBuilder::OnSetFloatSliderValue, ETextCommit::Default,
                        FSetSliderAdditionalArgs(&FFlexSplineNodeBuilder::OnSetEndScale, TransactionTexts[4], EAxis::Y))
            .OnYCommitted(this, &FFlexSplineNodeBuilder::OnSetFloatSliderValue,
                            FSetSliderAdditionalArgs(&FFlexSplineNodeBuilder::OnSetEndScale, TransactionTexts[4], EAxis::Y, true))
        ];
#pragma endregion

#pragma region End Offset
    splineGroup.AddWidgetRow()
        .Visibility(TAttribute<EVisibility>(this, &FFlexSplineNodeBuilder::ShowVisibleSpline))
        .NameContent()
        .HAlign(HAlign_Left)
        .VAlign(VAlign_Center)
        [
            SNew(STextBlock)
            .Text(LOCTEXT("EndOffset", "End Offset"))
            .Font(FontInfo)
        ]
        .ValueContent()
        .MinDesiredWidth(DoubleSpinboxWidth)
        .MaxDesiredWidth(DoubleSpinboxWidth)
        [
            SNew(SFlexVectorInputBox)
            .Font(FontInfo)
            .AllowSpin(true)
            .bColorAxisLabels(true)
            .AllowResponsiveLayout(true)
            .MinValue(TOptional<float>())
            .MinSliderValue(TOptional<float>())
            .MaxValue(TOptional<float>())
            .MaxSliderValue(TOptional<float>())
            .Delta(SpinboxDelta)
            .OnBeginSliderMovement(this, &FFlexSplineNodeBuilder::OnSliderAction, 0.f, ESliderMode::BeginSlider, TransactionTexts[5])
            .OnEndSliderMovement(this, &FFlexSplineNodeBuilder::OnSliderAction, ESliderMode::EndSlider, FText())
            .X(this, &FFlexSplineNodeBuilder::GetEndOffset, EAxis::X)
            .OnXChanged(this, &FFlexSplineNodeBuilder::OnSetFloatSliderValue, ETextCommit::Default,
                        FSetSliderAdditionalArgs(&FFlexSplineNodeBuilder::OnSetEndOffset, TransactionTexts[5], EAxis::X))
            .OnXCommitted(this, &FFlexSplineNodeBuilder::OnSetFloatSliderValue,
                          FSetSliderAdditionalArgs(&FFlexSplineNodeBuilder::OnSetEndOffset, TransactionTexts[5], EAxis::X, true))
            .Y(this, &FFlexSplineNodeBuilder::GetEndOffset, EAxis::Y)
            .OnYChanged(this, &FFlexSplineNodeBuilder::OnSetFloatSliderValue, ETextCommit::Default,
                        FSetSliderAdditionalArgs(&FFlexSplineNodeBuilder::OnSetEndOffset, TransactionTexts[5], EAxis::Y))
            .OnYCommitted(this, &FFlexSplineNodeBuilder::OnSetFloatSliderValue,
                          FSetSliderAdditionalArgs(&FFlexSplineNodeBuilder::OnSetEndOffset, TransactionTexts[5], EAxis::Y, true))
        ];
#pragma endregion

#pragma region Up Direction
    splineGroup.AddWidgetRow()
        .Visibility(TAttribute<EVisibility>(this, &FFlexSplineNodeBuilder::ShowVisibleSpline))
        .NameContent()
        .HAlign(HAlign_Left)
        .VAlign(VAlign_Center)
        [
            SNew(STextBlock)
            .Text(LOCTEXT("UpDirection", "Up Direction"))
            .Font(FontInfo)
        ]
        .ValueContent()
        .MinDesiredWidth(TripleSpinboxWidth)
        .MaxDesiredWidth(TripleSpinboxWidth)
        [
            SNew(SFlexVectorInputBox)
            .bIsVector3D(true)
            .Font(FontInfo)
            .AllowSpin(true)
            .bColorAxisLabels(true)
            .AllowResponsiveLayout(true)
            .MinValue(TOptional<float>())
            .MinSliderValue(TOptional<float>())
            .MaxValue(TOptional<float>())
            .MaxSliderValue(TOptional<float>())
            .Delta(SpinboxDelta)
            .OnBeginSliderMovement(this, &FFlexSplineNodeBuilder::OnSliderAction, 0.f, ESliderMode::BeginSlider, TransactionTexts[6])
            .OnEndSliderMovement(this, &FFlexSplineNodeBuilder::OnSliderAction, ESliderMode::EndSlider, FText())
            .X(this, &FFlexSplineNodeBuilder::GetUpDirection, EAxis::X)
            .OnXChanged(this, &FFlexSplineNodeBuilder::OnSetFloatSliderValue, ETextCommit::Default,
                        FSetSliderAdditionalArgs(&FFlexSplineNodeBuilder::OnSetUpDirection, TransactionTexts[6], EAxis::X))
            .OnXCommitted(this, &FFlexSplineNodeBuilder::OnSetFloatSliderValue,
                          FSetSliderAdditionalArgs(&FFlexSplineNodeBuilder::OnSetUpDirection, TransactionTexts[6], EAxis::X, true))
            .Y(this, &FFlexSplineNodeBuilder::GetUpDirection, EAxis::Y)
            .OnYChanged(this, &FFlexSplineNodeBuilder::OnSetFloatSliderValue, ETextCommit::Default,
                        FSetSliderAdditionalArgs(&FFlexSplineNodeBuilder::OnSetUpDirection, TransactionTexts[6], EAxis::Y))
            .OnYCommitted(this, &FFlexSplineNodeBuilder::OnSetFloatSliderValue,
                          FSetSliderAdditionalArgs(&FFlexSplineNodeBuilder::OnSetUpDirection, TransactionTexts[6], EAxis::Y, true))
            .Z(this, &FFlexSplineNodeBuilder::GetUpDirection, EAxis::Z)
            .OnZChanged(this, &FFlexSplineNodeBuilder::OnSetFloatSliderValue, ETextCommit::Default,
                        FSetSliderAdditionalArgs(&FFlexSplineNodeBuilder::OnSetUpDirection, TransactionTexts[6], EAxis::Z))
            .OnZCommitted(this, &FFlexSplineNodeBuilder::OnSetFloatSliderValue,
                          FSetSliderAdditionalArgs(&FFlexSplineNodeBuilder::OnSetUpDirection, TransactionTexts[6], EAxis::Z, true))
        ];
#pragma endregion

#pragma region Synchronize With Previous
    splineGroup.AddWidgetRow()
        .Visibility(TAttribute<EVisibility>(this, &FFlexSplineNodeBuilder::ShowVisibleSpline))
        .IsEnabled(TAttribute<bool>(this, &FFlexSplineNodeBuilder::IsSyncGloballyEnabled))
        .NameContent()
        .HAlign(HAlign_Left)
        .VAlign(VAlign_Center)
        [
            SNew(STextBlock)
            .Text(LOCTEXT("Sync", "Synchronize With Previous"))
            .Font(FontInfo)
            .ToolTip(IDocumentation::Get()->CreateToolTip(GlobalSyncTooltipText, nullptr, TEXT("Shared/LevelEditor"), ""))
        ]
        .ValueContent()
        [
            SNew(SCheckBox)
            .IsChecked(this, &FFlexSplineNodeBuilder::GetSynchroniseWithPrevious)
            .OnCheckStateChanged(this, &FFlexSplineNodeBuilder::OnCheckedChangedSynchroniseWithPrevious)
        ];
#pragma endregion


    IDetailGroup& staticGroup = ChildrenBuilder.AddGroup("StaticGroup", LOCTEXT("StaticMeshGroup", "Point Static-Mesh Config"));
    // Message which is shown when no points are selected
    staticGroup.AddWidgetRow()
        .Visibility(TAttribute<EVisibility>(this, &FFlexSplineNodeBuilder::ShowNotVisibleStatic))
        [
            BuildNotVisibleMessage(EFlexSplineMeshType::StaticMesh)
        ];

#pragma region Static Mesh Location Offset
    staticGroup.AddWidgetRow()
        .Visibility(TAttribute<EVisibility>(this, &FFlexSplineNodeBuilder::ShowVisibleStatic))
        .NameContent()
        .HAlign(HAlign_Left)
        .VAlign(VAlign_Center)
        [
            SNew(STextBlock)
            .Text(LOCTEXT("SMLoc", "Location Offset"))
            .Font(FontInfo)
        ]
        .ValueContent()
        .MinDesiredWidth(TripleSpinboxWidth)
        .MaxDesiredWidth(TripleSpinboxWidth)
        [
            SNew(SFlexVectorInputBox)
            .bIsVector3D(true)
            .Font(FontInfo)
            .AllowSpin(true)
            .bColorAxisLabels(true)
            .AllowResponsiveLayout(true)
            .MinValue(TOptional<float>())
            .MinSliderValue(TOptional<float>())
            .MaxValue(TOptional<float>())
            .MaxSliderValue(TOptional<float>())
            .Delta(SpinboxDelta)
            .OnBeginSliderMovement(this, &FFlexSplineNodeBuilder::OnSliderAction, 0.f, ESliderMode::BeginSlider, TransactionTexts[8])
            .OnEndSliderMovement(this, &FFlexSplineNodeBuilder::OnSliderAction, ESliderMode::EndSlider, FText())
            .X(this, &FFlexSplineNodeBuilder::GetSMLocationOffset, EAxis::X)
            .OnXChanged(this, &FFlexSplineNodeBuilder::OnSetFloatSliderValue, ETextCommit::Default,
                        FSetSliderAdditionalArgs(&FFlexSplineNodeBuilder::OnSetSMLocationOffset, TransactionTexts[8], EAxis::X))
            .OnXCommitted(this, &FFlexSplineNodeBuilder::OnSetFloatSliderValue,
                          FSetSliderAdditionalArgs(&FFlexSplineNodeBuilder::OnSetSMLocationOffset, TransactionTexts[8], EAxis::X, true))
            .Y(this, &FFlexSplineNodeBuilder::GetSMLocationOffset, EAxis::Y)
            .OnYChanged(this, &FFlexSplineNodeBuilder::OnSetFloatSliderValue, ETextCommit::Default,
                        FSetSliderAdditionalArgs(&FFlexSplineNodeBuilder::OnSetSMLocationOffset, TransactionTexts[8], EAxis::Y))
            .OnYCommitted(this, &FFlexSplineNodeBuilder::OnSetFloatSliderValue,
                          FSetSliderAdditionalArgs(&FFlexSplineNodeBuilder::OnSetSMLocationOffset, TransactionTexts[8], EAxis::Y, true))
            .Z(this, &FFlexSplineNodeBuilder::GetSMLocationOffset, EAxis::Z)
            .OnZChanged(this, &FFlexSplineNodeBuilder::OnSetFloatSliderValue, ETextCommit::Default,
                        FSetSliderAdditionalArgs(&FFlexSplineNodeBuilder::OnSetSMLocationOffset, TransactionTexts[8], EAxis::Z))
            .OnZCommitted(this, &FFlexSplineNodeBuilder::OnSetFloatSliderValue,
                          FSetSliderAdditionalArgs(&FFlexSplineNodeBuilder::OnSetSMLocationOffset, TransactionTexts[8], EAxis::Z, true))
        ];
#pragma endregion

#pragma region Static Mesh Scale
    staticGroup.AddWidgetRow()
        .Visibility(TAttribute<EVisibility>(this, &FFlexSplineNodeBuilder::ShowVisibleStatic))
        .NameContent()
        .HAlign(HAlign_Left)
        .VAlign(VAlign_Center)
        [
            SNew(STextBlock)
            .Text(LOCTEXT("SMScale", "Scale"))
            .Font(FontInfo)
        ]
        .ValueContent()
        .MinDesiredWidth(TripleSpinboxWidth)
        .MaxDesiredWidth(TripleSpinboxWidth)
        [
            SNew(SFlexVectorInputBox)
            .bIsVector3D(true)
            .Font(FontInfo)
            .AllowSpin(true)
            .bColorAxisLabels(true)
            .AllowResponsiveLayout(true)
            .MinValue(TOptional<float>())
            .MinSliderValue(TOptional<float>())
            .MaxValue(TOptional<float>())
            .MaxSliderValue(TOptional<float>())
            .Delta(SpinboxDelta)
            .OnBeginSliderMovement(this, &FFlexSplineNodeBuilder::OnSliderAction, 0.f, ESliderMode::BeginSlider, TransactionTexts[9])
            .OnEndSliderMovement(this, &FFlexSplineNodeBuilder::OnSliderAction, ESliderMode::EndSlider, FText())
            .X(this, &FFlexSplineNodeBuilder::GetSMScale, EAxis::X)
            .OnXChanged(this, &FFlexSplineNodeBuilder::OnSetFloatSliderValue, ETextCommit::Default,
                        FSetSliderAdditionalArgs(&FFlexSplineNodeBuilder::OnSetSMScale, TransactionTexts[9], EAxis::X))
            .OnXCommitted(this, &FFlexSplineNodeBuilder::OnSetFloatSliderValue,
                          FSetSliderAdditionalArgs(&FFlexSplineNodeBuilder::OnSetSMScale, TransactionTexts[9], EAxis::X, true))
            .Y(this, &FFlexSplineNodeBuilder::GetSMScale, EAxis::Y)
            .OnYChanged(this, &FFlexSplineNodeBuilder::OnSetFloatSliderValue, ETextCommit::Default,
                        FSetSliderAdditionalArgs(&FFlexSplineNodeBuilder::OnSetSMScale, TransactionTexts[9], EAxis::Y))
            .OnYCommitted(this, &FFlexSplineNodeBuilder::OnSetFloatSliderValue,
                          FSetSliderAdditionalArgs(&FFlexSplineNodeBuilder::OnSetSMScale, TransactionTexts[9], EAxis::Y, true))
            .Z(this, &FFlexSplineNodeBuilder::GetSMScale, EAxis::Z)
            .OnZChanged(this, &FFlexSplineNodeBuilder::OnSetFloatSliderValue, ETextCommit::Default,
                        FSetSliderAdditionalArgs(&FFlexSplineNodeBuilder::OnSetSMScale, TransactionTexts[9], EAxis::Z))
            .OnZCommitted(this, &FFlexSplineNodeBuilder::OnSetFloatSliderValue,
                          FSetSliderAdditionalArgs(&FFlexSplineNodeBuilder::OnSetSMScale, TransactionTexts[9], EAxis::Z, true))
        ];
#pragma endregion

#pragma region Static Mesh Rotation
    staticGroup.AddWidgetRow()
        .Visibility(TAttribute<EVisibility>(this, &FFlexSplineNodeBuilder::ShowVisibleStatic))
        .NameContent()
        .HAlign(HAlign_Left)
        .VAlign(VAlign_Center)
        [
            SNew(STextBlock)
            .Text(LOCTEXT("SMRotation", "Rotation"))
            .Font(FontInfo)
        ]
        .ValueContent()
        .MinDesiredWidth(TripleSpinboxWidth)
        .MaxDesiredWidth(TripleSpinboxWidth)
        [
            SNew(SRotatorInputBox)
            .AllowSpin(true)
            .Font(FontInfo)
            .TypeInterface(typeInterface)
            .Roll(this, &FFlexSplineNodeBuilder::GetSMRotation, EAxis::X)
            .Pitch(this, &FFlexSplineNodeBuilder::GetSMRotation, EAxis::Y)
            .Yaw(this, &FFlexSplineNodeBuilder::GetSMRotation, EAxis::Z)
            .AllowResponsiveLayout(true)
            .bColorAxisLabels(true)
            .OnBeginSliderMovement(this, &FFlexSplineNodeBuilder::OnSliderAction, 0.f, ESliderMode::BeginSlider, TransactionTexts[10])
            .OnEndSliderMovement(this, &FFlexSplineNodeBuilder::OnSliderAction, ESliderMode::EndSlider, TransactionTexts[10])
            .OnRollChanged(this, &FFlexSplineNodeBuilder::OnSetFloatSliderValue, ETextCommit::Default, 
                           FSetSliderAdditionalArgs(&FFlexSplineNodeBuilder::OnSetSMRotation, TransactionTexts[10], EAxis::X))
            .OnPitchChanged(this, &FFlexSplineNodeBuilder::OnSetFloatSliderValue, ETextCommit::Default,
                            FSetSliderAdditionalArgs(&FFlexSplineNodeBuilder::OnSetSMRotation, TransactionTexts[10], EAxis::Y))
            .OnYawChanged(this, &FFlexSplineNodeBuilder::OnSetFloatSliderValue, ETextCommit::Default,
                          FSetSliderAdditionalArgs(&FFlexSplineNodeBuilder::OnSetSMRotation, TransactionTexts[10], EAxis::Z))
            .OnRollCommitted(this, &FFlexSplineNodeBuilder::OnSetFloatSliderValue,
                             FSetSliderAdditionalArgs(&FFlexSplineNodeBuilder::OnSetSMRotation, TransactionTexts[10], EAxis::X, true))
            .OnPitchCommitted(this, &FFlexSplineNodeBuilder::OnSetFloatSliderValue,
                              FSetSliderAdditionalArgs(&FFlexSplineNodeBuilder::OnSetSMRotation, TransactionTexts[10], EAxis::Y, true))
            .OnYawCommitted(this, &FFlexSplineNodeBuilder::OnSetFloatSliderValue,
                            FSetSliderAdditionalArgs(&FFlexSplineNodeBuilder::OnSetSMRotation, TransactionTexts[10], EAxis::Z, true))
        ];
#pragma endregion
}

void FFlexSplineNodeBuilder::Tick(float DeltaTime)
{
    UpdateValues();
}

FName FFlexSplineNodeBuilder::GetName() const
{
    static const FName Name("FlexSplineNodeBuilder");
    return Name;
}


//////////////////////////////////////////////////////////////////////////
// HELPERS
TSharedRef<SWidget> FFlexSplineNodeBuilder::BuildNotVisibleMessage(EFlexSplineMeshType MeshType) const
{
    return SNew(SBox)
            .HAlign(HAlign_Center)
            .VAlign(VAlign_Center)
            [
                SNew(STextBlock)
                .Text(this, &FFlexSplineNodeBuilder::GetNoSelectionText, MeshType)
                .Font(IDetailLayoutBuilder::GetDetailFont())
            ];
}

FText FFlexSplineNodeBuilder::GetNoSelectionText(EFlexSplineMeshType MeshType) const
{
    FText message = NoSelectionText;
    const auto flexSpline = GetFlexSpline();
    if (flexSpline && IsFlexSplineSelected())
    {
        if (!flexSpline->GetMeshCountForType(MeshType))
        {
            switch (MeshType)
            {
            case EFlexSplineMeshType::SplineMesh: message = NoSplineMeshesText; break;
            case EFlexSplineMeshType::StaticMesh: message = NoStaticMeshesText; break;
            }
        }
    }

    return message;
}

bool FFlexSplineNodeBuilder::IsSyncDisabled() const
{
    bool result            = true;
    AFlexSplineActor* flex = GetFlexSpline();
    if (flex)
    {
        if (flex->SynchronizeConfig == EFlexGlobalConfigType::Everywhere)
        {
            result = false;
        }
        else if (flex->SynchronizeConfig == EFlexGlobalConfigType::Custom)
        {
            for (int32 index : SelectedKeys)
            {
                if (flex->PointDataArray.IsValidIndex(index))
                {
                    const bool bSyncWithPrev = flex->PointDataArray[index].bSynchroniseWithPrevious;
                    if (bSyncWithPrev)
                    {
                        result = false;
                        break;
                    }
                }
            }
        }
    }

    return result;
}

bool FFlexSplineNodeBuilder::IsSyncGloballyEnabled() const
{
    bool result = false;
    const auto flex = GetFlexSpline();
    if (flex)
    {
        result = (flex->SynchronizeConfig == EFlexGlobalConfigType::Custom);
    }
    return result;
}

AFlexSplineActor* FFlexSplineNodeBuilder::GetFlexSpline() const
{
    AFlexSplineActor* flex = Cast<AFlexSplineActor>(SplineComp.IsValid() ? SplineComp->GetOwner() : nullptr);
    // Try to get Actor from spline point first, if it fails try getting it from details
    if (!flex && DetailBuilder)
    {
        TArray<TWeakObjectPtr<UObject>> selectedObjects;
        TArray<AFlexSplineActor*> selectedFlexActors;
        DetailBuilder->GetObjectsBeingCustomized(selectedObjects);

        for (auto& object : selectedObjects)
        {
            auto tempFlex = Cast<AFlexSplineActor>(object.Get());
            if (tempFlex)
            {
                selectedFlexActors.Add(tempFlex);
            }
        }

        if (selectedFlexActors.Num() == 1)
        {
            flex = Cast<AFlexSplineActor>(selectedFlexActors[0]);
        }
    }
    return flex;
}

EVisibility FFlexSplineNodeBuilder::ShowVisible(EFlexSplineMeshType MeshType) const
{
    return ( ShowNotVisible(MeshType) == EVisibility::Visible ) || ( !GetFlexSpline() )
           ? EVisibility::Collapsed
           : EVisibility::Visible;
}

EVisibility FFlexSplineNodeBuilder::ShowNotVisible(EFlexSplineMeshType MeshType) const
{
    EVisibility result = EVisibility::Collapsed;
    const auto flexSpline = GetFlexSpline();

    if (flexSpline)
    {
        result = (SelectedKeys.Num() == 0 || !IsFlexSplineSelected() || !flexSpline->GetMeshCountForType(MeshType))
            ? EVisibility::Visible
            : EVisibility::Collapsed;
    }

    return result;
}

TOptional<float> FFlexSplineNodeBuilder::GetStartScale(EAxis::Type Axis) const
{
    TOptional<float> result;
    switch (Axis)
    {
    case EAxis::X: result = StartScale.X; break;
    case EAxis::Y: result = StartScale.Y; break;
    default: break;
    }
    return result;
}

TOptional<float> FFlexSplineNodeBuilder::GetStartOffset(EAxis::Type Axis) const
{
    TOptional<float> result;
    switch (Axis)
    {
    case EAxis::X: result = StartOffset.X; break;
    case EAxis::Y: result = StartOffset.Y; break;
    default: break;
    }
    return result;
}

TOptional<float> FFlexSplineNodeBuilder::GetEndScale(EAxis::Type Axis) const
{
    TOptional<float> result;
    switch (Axis)
    {
    case EAxis::X: result = EndScale.X; break;
    case EAxis::Y: result = EndScale.Y; break;
    default: break;
    }
    return result;
}

TOptional<float> FFlexSplineNodeBuilder::GetEndOffset(EAxis::Type Axis) const
{
    TOptional<float> result;
    switch (Axis)
    {
    case EAxis::X: result = EndOffset.X; break;
    case EAxis::Y: result = EndOffset.Y; break;
    default: break;
    }
    return result;
}

TOptional<float> FFlexSplineNodeBuilder::GetUpDirection(EAxis::Type Axis) const
{
    TOptional<float> result;
    switch (Axis)
    {
    case EAxis::X: result = UpDirection.X; break;
    case EAxis::Y: result = UpDirection.Y; break;
    case EAxis::Z: result = UpDirection.Z; break;
    default: break;
    }
    return result;
}

ECheckBoxState FFlexSplineNodeBuilder::GetSynchroniseWithPrevious() const
{
    ECheckBoxState result = ECheckBoxState::Undetermined;

    if (SynchroniseWithPrevious.bInitialized)
    {
        result = SynchroniseWithPrevious.Value.Get(false) ?
                 ECheckBoxState::Checked                  : 
                 ECheckBoxState::Unchecked;
    }
    return result;
}

TOptional<float> FFlexSplineNodeBuilder::GetSMLocationOffset(EAxis::Type Axis) const
{
    TOptional<float> result;
    switch (Axis)
    {
    case EAxis::X: result = SMLocationOffset.X; break;
    case EAxis::Y: result = SMLocationOffset.Y; break;
    case EAxis::Z: result = SMLocationOffset.Z; break;
    default: break;
    }
    return result;
}

TOptional<float> FFlexSplineNodeBuilder::GetSMScale(EAxis::Type Axis) const
{
    TOptional<float> result;
    switch (Axis)
    {
    case EAxis::X: result = SMScale.X; break;
    case EAxis::Y: result = SMScale.Y; break;
    case EAxis::Z: result = SMScale.Z; break;
    default: break;
    }
    return result;
}

TOptional<float> FFlexSplineNodeBuilder::GetSMRotation(EAxis::Type Axis) const
{
    TOptional<float> result;
    // Axes need to be mapped here to rotation values
    switch (Axis)
    {
    case EAxis::X: result = SMRotation.Roll;  break;
    case EAxis::Y: result = SMRotation.Pitch; break;
    case EAxis::Z: result = SMRotation.Yaw;   break;
    default: break;
    }
    return result;
}


void FFlexSplineNodeBuilder::OnSliderAction(float NewValue, ESliderMode SliderMode, FText TransactionMessage)
{
    if (SliderMode == ESliderMode::BeginSlider)
    {
        GEditor->BeginTransaction(TransactionMessage);
    }
    // ====================================================================================================
    else if (SliderMode == ESliderMode::EndSlider)
    {
        GEditor->EndTransaction();
    }
}

void FFlexSplineNodeBuilder::OnSetFloatSliderValue(float NewValue, ETextCommit::Type CommitInfo, FSetSliderAdditionalArgs Args)
{
    auto flexSplineActor = GetFlexSpline();
    if (flexSplineActor)
    {
        // ===== TRANSACTION START ====
        if (Args.bCommited)
            GEditor->BeginTransaction(Args.TransactionMessage);

        NotifyPreChange(flexSplineActor);
        //------------------------- Flex Spline changes ------------------------
        if(Args.Impl)
            (this->*Args.Impl)(NewValue, Args.Axis, flexSplineActor);
        //----------------------------------------------------------------------
        NotifyPostChange(flexSplineActor);

        // ==== TRANSACTION END ====
        if (Args.bCommited)
            GEditor->EndTransaction();

        UpdateValues();
        GUnrealEd->RedrawLevelEditingViewports();
   }
}

void FFlexSplineNodeBuilder::OnSetStartRoll(float NewValue, EAxis::Type Axis, AFlexSplineActor* FlexSpline)
{
    for (int32 index : SelectedKeys)
    {
        FlexSpline->PointDataArray[index].StartRoll = NewValue;
    }
}

void FFlexSplineNodeBuilder::OnSetStartScale(float NewValue, EAxis::Type Axis, AFlexSplineActor* FlexSpline)
{
    for (int32 index : SelectedKeys)
    {
        switch (Axis)
        {
        case EAxis::X: FlexSpline->PointDataArray[index].StartScale.X = NewValue; break;
        case EAxis::Y: FlexSpline->PointDataArray[index].StartScale.Y = NewValue; break;
        default: break;
        }
    }
}

void FFlexSplineNodeBuilder::OnSetStartOffset(float NewValue, EAxis::Type Axis, AFlexSplineActor* FlexSpline)
{
    for (int32 index : SelectedKeys)
    {
        switch (Axis)
        {
        case EAxis::X: FlexSpline->PointDataArray[index].StartOffset.X = NewValue; break;
        case EAxis::Y: FlexSpline->PointDataArray[index].StartOffset.Y = NewValue; break;
        default: break;
        }
    }
}

void FFlexSplineNodeBuilder::OnSetEndRoll(float NewValue, EAxis::Type Axis, AFlexSplineActor* FlexSpline)
{
    for (int32 index : SelectedKeys)
    {
        FlexSpline->PointDataArray[index].EndRoll = NewValue;
    }
}

void FFlexSplineNodeBuilder::OnSetEndScale(float NewValue, EAxis::Type Axis, AFlexSplineActor* FlexSpline)
{
    for (int32 index : SelectedKeys)
    {
        switch (Axis)
        {
        case EAxis::X: FlexSpline->PointDataArray[index].EndScale.X = NewValue; break;
        case EAxis::Y: FlexSpline->PointDataArray[index].EndScale.Y = NewValue; break;
        default: break;
        }
    }
}

void FFlexSplineNodeBuilder::OnSetEndOffset(float NewValue, EAxis::Type Axis, AFlexSplineActor* FlexSpline)
{
    for (int32 index : SelectedKeys)
    {
        switch (Axis)
        {
        case EAxis::X: FlexSpline->PointDataArray[index].EndOffset.X = NewValue; break;
        case EAxis::Y: FlexSpline->PointDataArray[index].EndOffset.Y = NewValue; break;
        default: break;
        }
    }
}

void FFlexSplineNodeBuilder::OnSetUpDirection(float NewValue, EAxis::Type Axis, AFlexSplineActor* FlexSpline)
{
    for (int32 index : SelectedKeys)
    {
        switch (Axis)
        {
        case EAxis::X: FlexSpline->PointDataArray[index].CustomPointUpDirection.X = NewValue; break;
        case EAxis::Y: FlexSpline->PointDataArray[index].CustomPointUpDirection.Y = NewValue; break;
        case EAxis::Z: FlexSpline->PointDataArray[index].CustomPointUpDirection.Z = NewValue; break;
        default: break;
        }
    }
}

void FFlexSplineNodeBuilder::OnCheckedChangedSynchroniseWithPrevious(ECheckBoxState NewState)
{
    auto flexSplineActor = GetFlexSpline();
    if (flexSplineActor)
    {
        FScopedTransaction Transaction(TransactionTexts[7]);
        NotifyPreChange(flexSplineActor);

        for (int32 index : SelectedKeys)
        {
            bool newValue = (NewState == ECheckBoxState::Checked);
            flexSplineActor->PointDataArray[index].bSynchroniseWithPrevious = newValue;
        }

        NotifyPostChange(flexSplineActor);
        UpdateValues();
    }
}

void FFlexSplineNodeBuilder::OnSetSMLocationOffset(float NewValue, EAxis::Type Axis, AFlexSplineActor* FlexSpline)
{
    for (int32 index : SelectedKeys)
    {
        switch (Axis)
        {
        case EAxis::X: FlexSpline->PointDataArray[index].SMLocationOffset.X = NewValue; break;
        case EAxis::Y: FlexSpline->PointDataArray[index].SMLocationOffset.Y = NewValue; break;
        case EAxis::Z: FlexSpline->PointDataArray[index].SMLocationOffset.Z = NewValue; break;
        default: break;
        }
    }
}

void FFlexSplineNodeBuilder::OnSetSMScale(float NewValue, EAxis::Type Axis, AFlexSplineActor* FlexSpline)
{
    for (int32 index : SelectedKeys)
    {
        switch (Axis)
        {
        case EAxis::X: FlexSpline->PointDataArray[index].SMScale.X = NewValue; break;
        case EAxis::Y: FlexSpline->PointDataArray[index].SMScale.Y = NewValue; break;
        case EAxis::Z: FlexSpline->PointDataArray[index].SMScale.Z = NewValue; break;
        default: break;
        }
    }
}

void FFlexSplineNodeBuilder::OnSetSMRotation(float NewValue, EAxis::Type Axis, AFlexSplineActor* FlexSpline)
{
    for (int32 index : SelectedKeys)
    {
        switch (Axis)
        {
        case EAxis::X: FlexSpline->PointDataArray[index].SMRotation.Roll  = NewValue; break;
        case EAxis::Y: FlexSpline->PointDataArray[index].SMRotation.Pitch = NewValue; break;
        case EAxis::Z: FlexSpline->PointDataArray[index].SMRotation.Yaw   = NewValue; break;
        default: break;
        }
    }
}


void FFlexSplineNodeBuilder::UpdateValues()
{
    SplineComp = SplineVisualizer->GetEditedSplineComponent();
    SelectedKeys = SplineVisualizer->GetSelectedKeys();
    auto flexSplineActor = Cast<AFlexSplineActor>(SplineComp.IsValid() ? SplineComp->GetOwner() : nullptr);

    StartRoll.Reset();
    StartScale.Reset();
    StartOffset.Reset();
    EndRoll.Reset();
    EndScale.Reset();
    EndOffset.Reset();
    UpDirection.Reset();
    SynchroniseWithPrevious.Reset();
    SMLocationOffset.Reset();
    SMScale.Reset();
    SMRotation.Reset();

    if (flexSplineActor)
    {
        for (int32 index : SelectedKeys)
        {
            if (flexSplineActor->PointDataArray.IsValidIndex(index))
            {
                const FSplinePointData& pointData = flexSplineActor->PointDataArray[index];

                StartRoll.Add(pointData.StartRoll);
                StartScale.Add(pointData.StartScale);
                StartOffset.Add(pointData.StartOffset);
                EndRoll.Add(pointData.EndRoll);
                EndScale.Add(pointData.EndScale);
                EndOffset.Add(pointData.EndOffset);
                UpDirection.Add(pointData.CustomPointUpDirection);
                SynchroniseWithPrevious.Add(pointData.bSynchroniseWithPrevious);
                SMLocationOffset.Add(pointData.SMLocationOffset);
                SMScale.Add(pointData.SMScale);
                SMRotation.Add(pointData.SMRotation);
            }
        }
    }
}

void FFlexSplineNodeBuilder::NotifyPreChange(AFlexSplineActor* FlexSplineActor)
{
    UProperty* startRollProperty = FindField<UProperty>(AFlexSplineActor::StaticClass(), "PointDataArray");
    FlexSplineActor->PreEditChange(startRollProperty);
    if (NotifyHook)
    {
        NotifyHook->NotifyPreChange(startRollProperty);
    }
}

void FFlexSplineNodeBuilder::NotifyPostChange(AFlexSplineActor* FlexSplineActor)
{
    UProperty* startRollProperty = FindField<UProperty>(AFlexSplineActor::StaticClass(), "PointDataArray");
    FPropertyChangedEvent PropertyChangedEvent(startRollProperty);
    if (NotifyHook)
    {
        NotifyHook->NotifyPostChange(PropertyChangedEvent, startRollProperty);
    }
    FlexSplineActor->PostEditChangeProperty(PropertyChangedEvent);
}


//////////////////////////////////////////////////////////////////////////
// FlexSplineDetails BASE INTERFACE
TSharedRef<IDetailCustomization> FFlexSplineDetails::MakeInstance()
{
    return MakeShareable(new FFlexSplineDetails);
}

void FFlexSplineDetails::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
    // Create a category so this is displayed early in the properties
    IDetailCategoryBuilder& category = DetailBuilder.EditCategory("FlexSpline", LOCTEXT("FlexSpline", "Flex Spline"), ECategoryPriority::Important);
    TSharedRef<FFlexSplineNodeBuilder> flexSplineNodeBuilder = MakeShareable(new FFlexSplineNodeBuilder);
    flexSplineNodeBuilder->NotifyHook = DetailBuilder.GetPropertyUtilities()->GetNotifyHook();
    flexSplineNodeBuilder->DetailBuilder = &DetailBuilder;
    category.AddCustomBuilder(flexSplineNodeBuilder);

    //You can get properties using the detail builder
    //MyProperty= DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(MyClass, MyClassPropertyName));
}


#undef LOCTEXT_NAMESPACE

/*****************************************************************************
* Copyright (C) 2017 Oliver Hawk - All Rights Reserved
*
* @Author       Oliver Hawk
* @EMail        *************************
* @Package      Flex Spline
******************************************************************************/

#include "FlexSplineDetailsPrivatePCH.h"
#include "FlexVectorInputBox.h"
#include "SNumericEntryBox.h"
#include "SWidgetSwitcher.h"
#include "SBorder.h"
#include "SBoxPanel.h"
#include "Geometry.h"

#define LOCTEXT_NAMESPACE "SFlexVector2DInputBox"

TAutoConsoleVariable<float> CVarCrushThem(TEXT("Slate.AllowNumericLabelCrush"), 1.0f, TEXT("Should we crush the vector input box?."));
TAutoConsoleVariable<float> CVarStopCrushWhenAbove(TEXT("Slate.NumericLabelWidthCrushStop"), 200.0f, TEXT("Stop crushing when the width is above."));
TAutoConsoleVariable<float> CVarStartCrushWhenBelow(TEXT("Slate.NumericLabelWidthCrushStart"), 190.0f, TEXT("Start crushing when the width is below."));


void SFlexVectorInputBox::Construct(const FArguments& InArgs)
{
    bCanBeCrushed = InArgs._AllowResponsiveLayout;

    TSharedRef<SHorizontalBox> HorizontalBox = SNew(SHorizontalBox);

    ChildSlot
    [
        HorizontalBox
    ];

    ConstructX(InArgs, HorizontalBox);
    ConstructY(InArgs, HorizontalBox);
    if (InArgs._bIsVector3D)
    {
        ConstructZ(InArgs, HorizontalBox);
    }
}

void SFlexVectorInputBox::ConstructX(const FArguments& InArgs, TSharedRef<SHorizontalBox> HorizontalBox)
{
    const FLinearColor LabelColor = InArgs._bColorAxisLabels ? SNumericEntryBox<float>::RedLabelBackgroundColor : FLinearColor(0.0f, 0.0f, 0.0f, .5f);
    TSharedRef<SWidget> LabelWidget = BuildDecoratorLabel(LabelColor, FLinearColor::White, LOCTEXT("X_Label", "X"));
    TAttribute<FMargin> MarginAttribute;
    if (bCanBeCrushed)
    {
        MarginAttribute = TAttribute<FMargin>::Create(TAttribute<FMargin>::FGetter::CreateSP(this, &SFlexVectorInputBox::GetTextMargin));
    }

    HorizontalBox->AddSlot()
        .VAlign(VAlign_Center)
        .FillWidth(1.0f)
        .Padding(0.0f, 1.0f, 2.0f, 1.0f)
        [
            SNew(SNumericEntryBox<float>)
            .AllowSpin(InArgs._AllowSpin)
            .OnBeginSliderMovement(InArgs._OnBeginSliderMovement)
            .OnEndSliderMovement(InArgs._OnEndSliderMovement)
            .MinValue(InArgs._MinValue)
            .MinSliderValue(InArgs._MinSliderValue)
            .MaxValue(InArgs._MaxValue)
            .MaxSliderValue(InArgs._MaxSliderValue)
            .Delta(InArgs._Delta)
            .Font(InArgs._Font)
            .Value(InArgs._X)
            .OnValueChanged(InArgs._OnXChanged)
            .OnValueCommitted(InArgs._OnXCommitted)
            .ToolTipText(LOCTEXT("X_ToolTip", "X Value"))
            .UndeterminedString(LOCTEXT("MultipleValues", "Multiple Values"))
            .LabelPadding(0)
            .OverrideTextMargin(MarginAttribute)
            .ContextMenuExtender(InArgs._ContextMenuExtenderX)
            .TypeInterface(InArgs._TypeInterface)
            .Label()
            [
                LabelWidget
            ]
        ];

}

void SFlexVectorInputBox::ConstructY(const FArguments& InArgs, TSharedRef<SHorizontalBox> HorizontalBox)
{
    const FLinearColor LabelColor = InArgs._bColorAxisLabels ? SNumericEntryBox<float>::GreenLabelBackgroundColor : FLinearColor(0.0f, 0.0f, 0.0f, .5f);
    TSharedRef<SWidget> LabelWidget = BuildDecoratorLabel(LabelColor, FLinearColor::White, LOCTEXT("Y_Label", "Y"));
    TAttribute<FMargin> MarginAttribute;
    if (bCanBeCrushed)
    {
        MarginAttribute = TAttribute<FMargin>::Create(TAttribute<FMargin>::FGetter::CreateSP(this, &SFlexVectorInputBox::GetTextMargin));
    }

    HorizontalBox->AddSlot()
        .VAlign(VAlign_Center)
        .FillWidth(1.0f)
        .Padding(0.0f, 1.0f, 2.0f, 1.0f)
        [
            SNew(SNumericEntryBox<float>)
            .AllowSpin(InArgs._AllowSpin)
            .OnBeginSliderMovement(InArgs._OnBeginSliderMovement)
            .OnEndSliderMovement(InArgs._OnEndSliderMovement)
            .MinValue(InArgs._MinValue)
            .MinSliderValue(InArgs._MinSliderValue)
            .MaxValue(InArgs._MaxValue)
            .MaxSliderValue(InArgs._MaxSliderValue)
            .Delta(InArgs._Delta)
            .Font(InArgs._Font)
            .Value(InArgs._Y)
            .OnValueChanged(InArgs._OnYChanged)
            .OnValueCommitted(InArgs._OnYCommitted)
            .ToolTipText(LOCTEXT("Y_ToolTip", "Y Value"))
            .UndeterminedString(LOCTEXT("MultipleValues", "Multiple Values"))
            .LabelPadding(0)
            .OverrideTextMargin(MarginAttribute)
            .ContextMenuExtender(InArgs._ContextMenuExtenderY)
            .TypeInterface(InArgs._TypeInterface)
            .Label()
            [
                LabelWidget
            ]
        ];

}

void SFlexVectorInputBox::ConstructZ(const FArguments& InArgs, TSharedRef<SHorizontalBox> HorizontalBox)
{
    const FLinearColor LabelColor = InArgs._bColorAxisLabels ? SNumericEntryBox<float>::BlueLabelBackgroundColor : FLinearColor(0.0f, 0.0f, 0.0f, .5f);
    TSharedRef<SWidget> LabelWidget = BuildDecoratorLabel(LabelColor, FLinearColor::White, LOCTEXT("Z_Label", "Z"));
    TAttribute<FMargin> MarginAttribute;
    if (bCanBeCrushed)
    {
        MarginAttribute = TAttribute<FMargin>::Create(TAttribute<FMargin>::FGetter::CreateSP(this, &SFlexVectorInputBox::GetTextMargin));
    }

    HorizontalBox->AddSlot()
        .VAlign(VAlign_Center)
        .FillWidth(1.0f)
        .Padding(0.0f, 1.0f, 2.0f, 1.0f)
        [
            SNew(SNumericEntryBox<float>)
            .AllowSpin(InArgs._AllowSpin)
            .OnBeginSliderMovement(InArgs._OnBeginSliderMovement)
            .OnEndSliderMovement(InArgs._OnEndSliderMovement)
            .MinValue(InArgs._MinValue)
            .MinSliderValue(InArgs._MinSliderValue)
            .MaxValue(InArgs._MaxValue)
            .MaxSliderValue(InArgs._MaxSliderValue)
            .Delta(InArgs._Delta)
            .Font(InArgs._Font)
            .Value(InArgs._Z)
            .OnValueChanged(InArgs._OnZChanged)
            .OnValueCommitted(InArgs._OnZCommitted)
            .ToolTipText(LOCTEXT("Z_ToolTip", "Z Value"))
            .UndeterminedString(LOCTEXT("MultipleValues", "Multiple Values"))
            .LabelPadding(0)
            .OverrideTextMargin(MarginAttribute)
            .ContextMenuExtender(InArgs._ContextMenuExtenderZ)
            .TypeInterface(InArgs._TypeInterface)
            .Label()
            [
                LabelWidget
            ]
        ];

}

TSharedRef<SWidget> SFlexVectorInputBox::BuildDecoratorLabel(FLinearColor BackgroundColor, FLinearColor InForegroundColor, FText Label)
{
    TSharedRef<SWidget> LabelWidget = SNumericEntryBox<float>::BuildLabel(Label, InForegroundColor, BackgroundColor);

    TSharedRef<SWidget> ResultWidget = LabelWidget;

    if (bCanBeCrushed)
    {
        ResultWidget =
            SNew(SWidgetSwitcher)
            .WidgetIndex(this, &SFlexVectorInputBox::GetLabelActiveSlot)
            + SWidgetSwitcher::Slot()
            [
                LabelWidget
            ]
            + SWidgetSwitcher::Slot()
            [
                SNew(SBorder)
                .BorderImage(FCoreStyle::Get().GetBrush("NumericEntrySpinBox.NarrowDecorator"))
                .BorderBackgroundColor(BackgroundColor)
                .ForegroundColor(InForegroundColor)
                .VAlign(VAlign_Center)
                .HAlign(HAlign_Left)
                .Padding(FMargin(5.0f, 0.0f, 0.0f, 0.0f))
            ];
    }

    return ResultWidget;
}

int32 SFlexVectorInputBox::GetLabelActiveSlot() const
{
    return bIsBeingCrushed ? 1 : 0;
}

FMargin SFlexVectorInputBox::GetTextMargin() const
{
    return bIsBeingCrushed ? FMargin(1.0f, 2.0f) : FMargin(4.0f, 2.0f);
}

void SFlexVectorInputBox::OnArrangeChildren(const FGeometry& AllottedGeometry, FArrangedChildren& ArrangedChildren) const
{
    bool bFoop = bCanBeCrushed && (CVarCrushThem.GetValueOnAnyThread() > 0.0f);

    if (bFoop)
    {
        const float AlottedWidth = AllottedGeometry.Size.X;

        const float CrushBelow = CVarStartCrushWhenBelow.GetValueOnAnyThread();
        const float StopCrushing = CVarStopCrushWhenAbove.GetValueOnAnyThread();

        if (bIsBeingCrushed)
        {
            bIsBeingCrushed = AlottedWidth < StopCrushing;
        }
        else
        {
            bIsBeingCrushed = AlottedWidth < CrushBelow;
        }
    }
    else
    {
        bIsBeingCrushed = false;
    }

    SCompoundWidget::OnArrangeChildren(AllottedGeometry, ArrangedChildren);
}

#undef LOCTEXT_NAMESPACE
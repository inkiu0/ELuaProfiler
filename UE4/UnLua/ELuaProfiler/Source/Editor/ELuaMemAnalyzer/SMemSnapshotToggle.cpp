// The MIT License (MIT)

// Copyright 2020 HankShu inkiu0@gmail.com
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#include "SMemSnapshotToggle.h"
#include "Editor/Styles/ELuaStyle.h"
#include "EditorStyleSet.h"
#include "Runtime/ELuaMemAnalyzer/ELuaMemAnalyzer.h"
#include "Widgets/SBoxPanel.h"
#include "Styling/SlateStyle.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/Text/STextBlock.h"

SMemSnapshotToggle::SMemSnapshotToggle()
{

}

/**
 * Construct this widget
 *
 * @param	InArgs	The declaration data for this widget
 */
void SMemSnapshotToggle::Construct(const FArguments& InArgs)
{
	TSharedPtr<FELuaMemSnapshot> snapshot = FELuaMemAnalyzer::GetInstance()->GetSnapshot(InArgs._SnapshotIndex);
	FString ToggleLabel = snapshot ? FString::Printf(TEXT("%.2f %s"), snapshot->GetTotalSizeMB(), *snapshot->GetSnapTimeStr()) : "Snapshot Missing";
	SBorder::Construct(SBorder::FArguments()
		//.ContentScale(InArgs._ContentScale)
		//.DesiredSizeScale(InArgs._DesiredSizeScale)
		//.BorderBackgroundColor(InArgs._ButtonColorAndOpacity)
		//.ForegroundColor(InArgs._ForegroundColor)
		.BorderImage(this, &SMemSnapshotToggle::GetBorder)
		//.HAlign(InArgs._HAlign)
		//.VAlign(InArgs._VAlign)
		//.Padding(TAttribute<FMargin>(this, &SButton::GetCombinedPadding))
		//.ShowEffectWhenDisabled(TAttribute<bool>(this, &SButton::GetShowDisabledEffect))
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center).AutoWidth()
			[
				SNew(STextBlock)
				.Text(FText::FromString(ToggleLabel))
			]
	
			+ SHorizontalBox::Slot().HAlign(HAlign_Center).VAlign(VAlign_Top).AutoWidth()
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().HAlign(HAlign_Center).VAlign(VAlign_Top).AutoHeight()
				[
					SNew(SButton)
					.ButtonStyle(FEditorStyle::Get(), "NoBorder")
					.OnClicked_Lambda([this]()
					{
						FELuaMemAnalyzer::GetInstance()->OnDeleteSnapshot(SnapshotIndex);
						return FReply::Handled();
					})
					[
						SNew(SImage)
						.Image(FEditorStyle::GetBrush("Cross"))
					]
				]

				+ SVerticalBox::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center).AutoHeight()
				[
					SNew(SSpacer)
				]
			]
		]
	);

	NormalImage = FELuaStyle::Get().GetBrush("Black");

	SelectedImage = FELuaStyle::Get().GetBrush("Yellow");

	Invalidate(EInvalidateWidget::Layout);

	SnapshotIndex = InArgs._SnapshotIndex;
	OnClicked = InArgs._OnClicked;
	OnDoubleClicked = InArgs._OnDoubleClicked;
	OnDeleted = InArgs._OnDeleted;
	OnCancelled = InArgs._OnCancelled;
}

int32 SMemSnapshotToggle::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	const FSlateBrush* BrushResource = GetBorder();

	if (BrushResource && BrushResource->DrawAs != ESlateBrushDrawType::NoDrawType)
	{
		FSlateDrawElement::MakeBox(
			OutDrawElements,
			LayerId,
			AllottedGeometry.ToPaintGeometry(),
			BrushResource,
			ESlateDrawEffect::None,
			BrushResource->GetTint(InWidgetStyle) * InWidgetStyle.GetColorAndOpacityTint() * BorderBackgroundColor.Get().GetColor(InWidgetStyle)
		);
	}

	return SCompoundWidget::OnPaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, true);
}

/** @return An image that represents this button's border*/
const FSlateBrush* SMemSnapshotToggle::GetBorder() const
{
	if (FELuaMemAnalyzer::GetInstance()->IsSelectedSnapshot(SnapshotIndex))
	{
		return SelectedImage;
	} 
	else
	{
		return NormalImage;
	}
}

FReply SMemSnapshotToggle::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	FReply Reply = FReply::Handled();
	if (!FELuaMemAnalyzer::GetInstance()->IsSelectedSnapshot(SnapshotIndex))
	{
		FELuaMemAnalyzer::GetInstance()->OnSelectSnapshot(SnapshotIndex);
		if (OnClicked.IsBound())
		{
			Reply = OnClicked.Execute();
		}
	}
	else
	{
		FELuaMemAnalyzer::GetInstance()->OnCancelOperate(SnapshotIndex);
		OnCancelled.ExecuteIfBound();
	}

	return Reply;
}

FReply SMemSnapshotToggle::OnMouseButtonDoubleClick(const FGeometry& InMyGeometry, const FPointerEvent& InMouseEvent)
{
	FELuaMemAnalyzer::GetInstance()->ShowSnapshot(SnapshotIndex);
	FReply Reply = FReply::Handled();
	if (OnDoubleClicked.IsBound())
	{
		Reply = OnDoubleClicked.Execute();
	}
	return Reply;
}

FVector2D SMemSnapshotToggle::ComputeDesiredSize(float LayoutScaleMultiplier) const
{
	return SBorder::ComputeDesiredSize(LayoutScaleMultiplier);
}

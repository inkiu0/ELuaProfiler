// The MIT License (MIT)

// Copyright 2023 HankShu inkiu0@gmail.com
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

#pragma once
#include "ELuaScalableAxis.h"


class SELuaFramePanel : public SCompoundWidget
{
public:
	static constexpr float MOUSE_SNAP_DISTANCE = 2.0f;

	enum class ECursorType
	{
		Default,
		Arrow,
		Hand,
	};

public:
	SELuaFramePanel();

	virtual ~SELuaFramePanel();

	void Reset();

	SLATE_BEGIN_ARGS(SELuaFramePanel)
	{
		_Clipping = EWidgetClipping::ClipToBounds;
	}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual void OnMouseEnter(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual void OnMouseLeave(const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseButtonDoubleClick(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

	virtual FCursorReply OnCursorQuery(const FGeometry& MyGeometry, const FPointerEvent& CursorEvent) const override;

protected:
	void DrawHorizontalAxisGrid(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FSlateBrush* Brush, const ESlateDrawEffect DrawEffects, const FSlateFontInfo& Font) const;
	void DrawVerticalAxisGrid(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FSlateBrush* Brush, const ESlateDrawEffect DrawEffects, const FSlateFontInfo& Font) const;

	void ShowContextMenu(const FPointerEvent& MouseEvent);

	void ContextMenu_AutoZoom_Execute();
	bool ContextMenu_AutoZoom_CanExecute();
	bool ContextMenu_AutoZoom_IsChecked();
	void AutoZoom();

	void BindUICommands();
	void HorizontalScrollBar_OnUserScrolled(float ScrollOffset);
	void UpdateHorizontalScrollBar();
	void ZoomHorizontally(const float Delta, const float X);
	
	int32 GetSelectFrameIndexByPos(float X, float Y);
	void OnSelectFrameBar(float X, float Y);

private:
	bool OnSizeChanged(const float InWidth, const float InHeight);
	void DrawBackground(
		const FPaintArgs& Args,
		const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements,
		int32& LayerId,
		const FSlateBrush* Brush,
		const ESlateDrawEffect DrawEffects,
		float& OutValidAreaX,
		float& OutValidAreaW
	) const;
	
	void DrawFrameBars(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32& LayerId, const FSlateBrush* Brush, const ESlateDrawEffect DrawEffects) const;

protected:
	FELuaScalableAxis AxisX;
	FELuaScalableAxis AxisY;
	
	bool bIsDirty;
	bool bIsPanelDirty;
	
	bool bIsAutoZoomEnabled;
	float AutoZoomPanelPos;
	float AutoZoomPanelScale;
	float AutoZoomPanelSize;

	uint64 AnalysisSyncNextTimestamp;

	TSharedPtr<SScrollBar> HorizontalScrollBar;
	
	FVector2D MousePos;
	FVector2D MousePosOnBtnDown;
	FVector2D MousePosOnBtnUp;
	float XPosOnBtnDown;

	bool bIsLeftBtnClicked;
	bool bIsRightBtnClicked;

	bool bIsScrolling;

	float TooltipDesiredOpacity;
	mutable float TooltipOpacity;

	FGeometry ThisGeometry;

	ECursorType CursorType;
	
	int32 HoveredIndex;
};

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


#include "ELuaMonitor.h"
#include "SELuaFramePanel.h"
#include "Fonts/FontMeasure.h"

#define LOCTEXT_NAMESPACE "SELuaFramePanel"

SELuaFramePanel::SELuaFramePanel()
{
	Reset();
}


SELuaFramePanel::~SELuaFramePanel()
{
}


void SELuaFramePanel::Reset()
{
	AxisX.Reset();
	AxisX.SetScaleLimits(0.0001f, 16.0f);
	AxisX.SetScale(16.0f);

	AxisY.Reset();
	AxisY.SetScaleLimits(0.01, 1000000.0);
	AxisY.SetScale(1500.0);

	bIsPanelDirty = true;

	bIsDirty = true;

	bIsAutoZoomEnabled = true;
	AutoZoomPanelPos = AxisX.GetPos();
	AutoZoomPanelScale = AxisX.GetScale();
	AutoZoomPanelSize = 0.0f;

	AnalysisSyncNextTimestamp = 0;

	MousePos = FVector2D::ZeroVector;

	MousePosOnBtnDown = FVector2D::ZeroVector;
	XPosOnBtnDown = 0.0f;

	MousePosOnBtnUp = FVector2D::ZeroVector;

	bIsLeftBtnClicked = false;
	bIsRightBtnClicked = false;

	bIsScrolling = false;

	TooltipDesiredOpacity = 0.9f;
	TooltipOpacity = 0.0f;

	CursorType = ECursorType::Default;
}

void SELuaFramePanel::Construct(const FArguments& InArgs)
{
	ChildSlot
	[
		SNew(SOverlay)
		.Visibility(EVisibility::SelfHitTestInvisible)

		+ SOverlay::Slot()
		.VAlign(VAlign_Top)
		.Padding(FMargin(0, 0, 0, 0))
		[
			SAssignNew(HorizontalScrollBar, SScrollBar)
			.Orientation(Orient_Horizontal)
			.AlwaysShowScrollbar(false)
			.Visibility(EVisibility::Visible)
			.Thickness(FVector2D(5.0f, 5.0f))
			.RenderOpacity(0.75)
			.OnUserScrolled(this, &SELuaFramePanel::HorizontalScrollBar_OnUserScrolled)
		]
	];

	UpdateHorizontalScrollBar();

	BindUICommands();
}

bool SELuaFramePanel::OnSizeChanged(const float InWidth, const float InHeight)
{
	const bool bXChanged = AxisX.SetSize(InWidth);
	const bool bYChanged = AxisY.SetSize(InHeight);
	if (bXChanged || bYChanged)
	{
		return true;
	}
	return false;
}

void SELuaFramePanel::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	if (ThisGeometry != AllottedGeometry || bIsPanelDirty)
	{
		bIsPanelDirty = false;
		const float ViewWidth = AllottedGeometry.GetLocalSize().X;
		const float ViewHeight = AllottedGeometry.GetLocalSize().Y;
		OnSizeChanged(ViewWidth, ViewHeight);
		bIsDirty = true;
	}

	ThisGeometry = AllottedGeometry;

	if (!bIsScrolling)
	{
		if (AxisX.UpdatePosWithinLimits())
		{
			bIsDirty = true;
		}
	}

	if (AutoZoomPanelPos != AxisX.GetPos() ||
		AutoZoomPanelScale != AxisX.GetScale())
	{
		bIsAutoZoomEnabled = false;
	}

	bool bAutoZoom = bIsAutoZoomEnabled && AutoZoomPanelSize != AxisX.GetSize();

	const uint64 Time = FPlatformTime::Cycles64();
	if (Time > AnalysisSyncNextTimestamp)
	{
		const uint64 WaitTime = static_cast<uint64>(0.1 / FPlatformTime::GetSecondsPerCycle64()); // 100ms
		AnalysisSyncNextTimestamp = Time + WaitTime;

		const int32 NumFrames = FELuaMonitor::GetInstance()->GetTotalFrames();
		if (NumFrames > AxisX.GetMaxValue())
		{
			AxisX.SetMinMaxInterval(0, NumFrames);
			UpdateHorizontalScrollBar();
			bIsDirty = true;

			if (bIsAutoZoomEnabled)
			{
				bAutoZoom = true;
			}
		}
	}

	if (bAutoZoom)
	{
		AutoZoom();
	}

	if (bIsDirty)
	{
		bIsDirty = false;
	}
}

int32 SELuaFramePanel::GetSelectFrameIndexByPos(float X, float Y)
{
	if (!bIsDirty)
	{
		const float BarWidth = AxisX.GetBarWidth();
		const int32 StartIndex = AxisX.GetValueAtOffset(0.f);
		const int32 FrameIndex = FMath::FloorToInt(X / BarWidth) + StartIndex;
		
		if (FrameIndex >= 0 && FrameIndex < FELuaMonitor::GetInstance()->GetTotalFrames())
		{
			return FrameIndex;
		}
	}
	return -1;
}

void SELuaFramePanel::OnSelectFrameBar(float X, float Y)
{
	X = FMath::Clamp(X, 1.f, AxisX.GetSize() - 1);
	const int32 FrameIndex = GetSelectFrameIndexByPos(X, Y);
	if (FrameIndex >= 0)
	{
		FELuaMonitor::GetInstance()->SetCurFrameIndex(FrameIndex);
	}
}

void SELuaFramePanel::DrawBackground(
	const FPaintArgs& Args,
	const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements,
	int32& LayerId,
	const FSlateBrush* Brush,
	const ESlateDrawEffect DrawEffects,
	float& OutValidAreaX,
	float& OutValidAreaW) const
{
	//  <------- W ------->
	//  X0    X1    X2     X3
	//  ++++++|*****|++++++
	//  ++++++|*****|++++++
	//  ++++++|*****|++++++
	
	const FLinearColor ValidAreaColor(0.07f, 0.07f, 0.07f, 1.0f);
	const FLinearColor InvalidAreaColor(0.1f, 0.07f, 0.07f, 1.0f);
	const FLinearColor EdgeColor(0.05f, 0.05f, 0.05f, 1.0f);
	
	const float X0 = 0.0f;
	const float X1 = AxisX.GetMinPos() - AxisX.GetPos();
	const float X2 = AxisX.GetMaxPos() - AxisX.GetPos();
	const float X3 = FMath::CeilToFloat(AxisX.GetSize());

	const float Y = 0.0f;
	const float H = FMath::CeilToFloat(AxisY.GetSize());

	if (X1 >= X3 || X2 <= X0)
	{
		OutValidAreaX = X0;
		OutValidAreaW = X3 - X0;

		// Draw invalid area (entire view).
		FSlateDrawElement::MakeBox(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(FVector2D(X3 - X0, H), FSlateLayoutTransform(1.f, FVector2D(X0, Y))), Brush, DrawEffects, InvalidAreaColor);
	}
	else // X1 < X3 && X2 > X0
	{
		if (X1 > X0)
		{
			// Draw invalid area (left).
			FSlateDrawElement::MakeBox(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(FVector2D(X1 - X0, H), FSlateLayoutTransform(1.f, FVector2D(X0, Y))), Brush, DrawEffects, InvalidAreaColor);
		}

		if (X2 < X3)
		{
			// Draw invalid area (right).
			FSlateDrawElement::MakeBox(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(FVector2D(X2 + 1.0f, Y), FSlateLayoutTransform(1.f, FVector2D(X3 - X2 - 1.0f, H))), Brush, DrawEffects, InvalidAreaColor);

			// Draw the right edge (end time).
			FSlateDrawElement::MakeBox(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(FVector2D(X2, Y), FSlateLayoutTransform(1.f, FVector2D(1.0f, H))), Brush, DrawEffects, InvalidAreaColor);
		}

		float ValidAreaX = FMath::Max(X1, X0);
		float ValidAreaW = FMath::Min(X2, X3) - ValidAreaX;

		if (X1 >= X0)
		{
			// Draw the left edge (start time).
			FSlateDrawElement::MakeBox(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(FVector2D(X1, Y), FSlateLayoutTransform(1.f, FVector2D(1.0f, H))), Brush, DrawEffects, InvalidAreaColor);

			// Adjust valid area to not overlap the left edge.
			ValidAreaX += 1.0f;
			ValidAreaW -= 1.0f;
		}

		if (ValidAreaW > 0.0f)
		{
			// Draw valid area.
			FSlateDrawElement::MakeBox(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(FVector2D(ValidAreaX, Y), FSlateLayoutTransform(1.f, FVector2D(ValidAreaW, H))), Brush, DrawEffects, InvalidAreaColor);
		}

		OutValidAreaX = ValidAreaX;
		OutValidAreaW = ValidAreaW;
	}

	LayerId++;
}

void SELuaFramePanel::DrawFrameBars(
	const FPaintArgs& Args,
	const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements,
	int32& LayerId,
	const FSlateBrush* Brush,
	const ESlateDrawEffect DrawEffects) const
{
	const int32 StartIndex = AxisX.GetValueAtOffset(0.f);
	const int32 EndIndex = AxisX.GetValueAtOffset(AxisX.GetSize());

	TArray<TSharedPtr<FELuaTraceInfoTree>> FrameList = FELuaMonitor::GetInstance()->GetFrameList();

	constexpr FLinearColor NormalColor(0.f, 0.5f, 0.f, 0.9f);
	constexpr FLinearColor WarningColor(1.f, 0.65f, 0.f, 0.9f);
	constexpr FLinearColor ExceedColor(1.f, 0.f, 0.f, 0.9f);
	const float BarWidth = AxisX.GetBarWidth();
	const float ViewHeight = FMath::RoundToFloat(AxisY.GetSize());

	for (int32 Index = 0; Index <= EndIndex && Index < FrameList.Num(); Index++)
	{
		FLinearColor BarColor = NormalColor;
		const float X = BarWidth * (Index - StartIndex);
		const TSharedPtr<FELuaTraceInfoNode> Root = FrameList[Index]->GetRoot();
		const float Height = FMath::RoundToFloat(AxisY.GetOffsetForValue(Root->TotalTime / 1000));
		if (Index == FELuaMonitor::GetInstance()->GetCurFrameIndex())
		{
			BarColor = FLinearColor::White;
		}
		else
		{
			if (Root->TotalTime > 33.33333)
			{
				BarColor = ExceedColor;
			}
			else if (Root->TotalTime > 16.66667)
			{
				BarColor = WarningColor;
			}
		}
		
		const float OffsetY = FMath::RoundToFloat(AxisY.GetOffsetForValue(0.0));
		const float H = Height - OffsetY;
		const float Y = ViewHeight - H;
		
		const FLinearColor ColorBorder(BarColor.R * 0.75f, BarColor.G * 0.75f, BarColor.B * 0.75f, 1.0);
		if (BarWidth > 2.0f)
		{
			FSlateDrawElement::MakeBox(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(FVector2D(BarWidth - 2.0f, H - 2.0f), FSlateLayoutTransform(1.f, FVector2D(X + 1.0f, Y + 1.0f))), Brush, DrawEffects, BarColor);

			// Draw border.
			FSlateDrawElement::MakeBox(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(FVector2D(1.0, H), FSlateLayoutTransform(1.f, FVector2D(X, Y))), Brush, DrawEffects, ColorBorder);
			FSlateDrawElement::MakeBox(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(FVector2D(1.0, H), FSlateLayoutTransform(1.f, FVector2D(X + BarWidth - 1.0f, Y))), Brush, DrawEffects, ColorBorder);
			FSlateDrawElement::MakeBox(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(FVector2D(BarWidth - 2.0f, 1.0f), FSlateLayoutTransform(1.f, FVector2D(X + 1.0f, Y))), Brush, DrawEffects, ColorBorder);
			FSlateDrawElement::MakeBox(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(FVector2D(BarWidth - 2.0f, 1.0f), FSlateLayoutTransform(1.f, FVector2D(X + 1.0f, Y + H - 1.0f))), Brush, DrawEffects, ColorBorder);
		}
		else
		{
			FSlateDrawElement::MakeBox(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(FVector2D(BarWidth, H), FSlateLayoutTransform(1.f, FVector2D(X, Y))), Brush, DrawEffects, ColorBorder);
		}
	}

	LayerId++;
}

int32 SELuaFramePanel::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	const bool bEnabled = ShouldBeEnabled(bParentEnabled);
	const ESlateDrawEffect DrawEffects = bEnabled ? ESlateDrawEffect::None : ESlateDrawEffect::DisabledEffect;

	const TSharedRef<FSlateFontMeasure> FontMeasureService = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
	const FSlateFontInfo SummaryFont = FCoreStyle::GetDefaultFontStyle("Regular", 8);

	const FSlateBrush* WhiteBrush = FEditorStyle::Get().GetBrush("WhiteBrush");

	const float ViewWidth = AllottedGeometry.Size.X;
	const float ViewHeight = AllottedGeometry.Size.Y;

	float ValidAreaX, ValidAreaW;
	DrawBackground(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, WhiteBrush, DrawEffects, ValidAreaX, ValidAreaW);

	DrawHorizontalAxisGrid(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, WhiteBrush, DrawEffects, SummaryFont);

	DrawFrameBars(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, WhiteBrush, DrawEffects);

	DrawVerticalAxisGrid(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, WhiteBrush, DrawEffects, SummaryFont);

	if (const TSharedPtr<FELuaTraceInfoNode> CurRoot = FELuaMonitor::GetInstance()->GetRoot())
	{
		const FString Text = FString::Format(TEXT("Frame {0} {1}ms"), {FELuaMonitor::GetInstance()->GetCurFrameIndex(), FString::Printf(TEXT("%.2f"), CurRoot->TotalTime)});
		const FVector2D TextSize = FontMeasureService->Measure(Text, SummaryFont);

		constexpr float DX = 2.0f;
		const float W2 = TextSize.X / 2 + DX;

		const float X1 = AxisX.GetOffsetForValue(CurRoot->TotalTime / 1000);
		float CX = X1 + FMath::RoundToFloat(AxisX.GetBarWidth() / 2);
		if (CX + W2 > AxisX.GetSize())
		{
			CX = FMath::RoundToFloat(AxisX.GetSize() - W2);
		}
		if (CX - W2 < 0)
		{
			CX = W2;
		}

		constexpr float Y = 10.0f;
		constexpr float H = 14.0f;
		FSlateDrawElement::MakeBox(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(FVector2D(2 * W2, H), FSlateLayoutTransform(1.f, FVector2D(CX - W2, Y))), WhiteBrush, DrawEffects, FLinearColor(0.7, 0.7, 0.7, TooltipOpacity));
		LayerId++;

		FSlateDrawElement::MakeText(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(FSlateLayoutTransform(1.f, FVector2D(CX - W2 + DX, Y + 1.0f))), Text, SummaryFont, DrawEffects, FLinearColor(0.0, 0.0, 0.0, TooltipOpacity));
		LayerId++;
	}

	return SCompoundWidget::OnPaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled && IsEnabled());
}

void SELuaFramePanel::DrawVerticalAxisGrid(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FSlateBrush* Brush, const ESlateDrawEffect DrawEffects, const FSlateFontInfo& Font) const
{
	const float ViewWidth = AxisX.GetSize();

	const float RoundedViewHeight = FMath::RoundToFloat(AxisY.GetSize());

	constexpr float TextH = 14.0f;
	constexpr float MinDY = 12.0f;

	const FLinearColor GridColor(0.0f, 0.0f, 0.0f, 0.1f);
	const FLinearColor TextBgColor(0.05f, 0.05f, 0.05f, 1.0f);

	const TSharedRef<FSlateFontMeasure> FontMeasureService = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();

	const double GridValues[] =
	{
		0.0,
		1.0 / 200.0, //    5 ms (200 fps)
		1.0 / 120.0, //  8.3 ms (120 fps)
		1.0 / 90.0,  // 11.1 ms (90 fps)
		1.0 / 72.0,  // 13.9 ms (72 fps)
		1.0 / 60.0,  // 16.7 ms (60 fps)
		1.0 / 30.0,  // 33.3 ms (30 fps)
		1.0 / 20.0,  //   50 ms (20 fps)
		1.0 / 15.0,  // 66.7 ms (15 fps)
		1.0 / 10.0,  //  100 ms (10 fps)
		1.0 / 5.0,   //  200 ms (5 fps)
		1.0,   // 1s
		10.0,  // 10s
		60.0,  // 1m
		600.0, // 10m
		3600.0 // 1h
	};
	constexpr int32 NumGridValues = sizeof(GridValues) / sizeof(double);

	double PreviousY = -MinDY;

	for (int32 Index = 0; Index < NumGridValues; ++Index)
	{
		const double Value = GridValues[Index];

		constexpr double Time60fps = 1.0 / 60.0;
		constexpr double Time30fps = 1.0 / 30.0;

		FLinearColor TextColor;
		if (Value <= Time60fps)
		{
			TextColor = FLinearColor(0.5f, 1.0f, 0.5f, 1.0f);
		}
		else if (Value <= Time30fps)
		{
			TextColor = FLinearColor(1.0f, 1.0f, 0.5f, 1.0f);
			//const float U = (Value - Time60fps) * 0.5 / (Time30fps - Time60fps);
			//TextColor = FLinearColor(0.5f + U, 1.0f - U, 0.5f, 1.0f);
		}
		else
		{
			TextColor = FLinearColor(1.0f, 0.5f, 0.5f, 1.0f);
		}

		const float Y = RoundedViewHeight - FMath::RoundToFloat(AxisY.GetOffsetForValue(Value));
		if (Y < 0)
		{
			break;
		}
		if (Y > RoundedViewHeight + TextH || FMath::Abs(PreviousY - Y) < MinDY)
		{
			continue;
		}
		PreviousY = Y;
		
		FSlateDrawElement::MakeBox(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(FVector2D(ViewWidth, 1), FSlateLayoutTransform(1.f, FVector2D(0, Y))), Brush, DrawEffects, GridColor);

		const FString LabelText = (Value == 0.0) ? TEXT("0") : FString::Printf(TEXT("%.2f)"), Value);
		const FVector2D LabelTextSize = FontMeasureService->Measure(LabelText, Font);
		float LabelX = ViewWidth - LabelTextSize.X - 4.0f;
		float LabelY = FMath::Clamp(Y - TextH / 2, 0.0f, RoundedViewHeight - TextH);

		// Draw background for value text.
		FSlateDrawElement::MakeBox(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(FVector2D(LabelTextSize.X + 4.0f, TextH), FSlateLayoutTransform(1.f, FVector2D(LabelX, LabelY))), Brush, DrawEffects, TextBgColor);

		// Draw value text.
		FSlateDrawElement::MakeText(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(FSlateLayoutTransform(1.f, FVector2D(LabelX + 2.0f, LabelY + 1.0f))), LabelText, Font, DrawEffects, TextColor);
	}
	LayerId++;
}

void SELuaFramePanel::DrawHorizontalAxisGrid(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FSlateBrush* Brush, const ESlateDrawEffect DrawEffects, const FSlateFontInfo& Font) const
{
	const float RoundedViewWidth = FMath::RoundToFloat(AxisX.GetSize());

	constexpr float MinDX = 125.0f; // min horizontal distance between vertical grid lines

	const int32 LeftIndex = AxisX.GetValueAtOffset(0.0f);
	const int32 GridIndex = AxisX.GetValueAtOffset(MinDX);
	const int32 RightIndex = AxisX.GetValueAtOffset(RoundedViewWidth);
	const int32 Delta = GridIndex - LeftIndex;

	if (Delta > 0)
	{
		int32 Power10 = 1;
		int32 Delta10 = Delta;
		while (Delta10 > 0)
		{
			Delta10 /= 10;
			Power10 *= 10;
		}
		if (Power10 >= 100)
		{
			Power10 /= 100;
		}
		else
		{
			Power10 = 1;
		}

		const int32 Grid = ((Delta + Power10 - 1) / Power10) * Power10;

		double StartIndex = ((LeftIndex + Grid - 1) / Grid) * Grid;
		while (StartIndex < 0)
		{
			StartIndex += Grid;
		}

		const float ViewHeight = AxisY.GetSize();

		const FLinearColor GridColor(0.0f, 0.0f, 0.0f, 0.1f);
		const FLinearColor TopTextColor(1.0f, 1.0f, 1.0f, 0.7f);

		for (int32 Index = StartIndex; Index < RightIndex; Index += Grid)
		{
			const float X = FMath::RoundToFloat(AxisX.GetOffsetForValue(Index));
			
			FSlateDrawElement::MakeBox(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(FVector2D(1, ViewHeight), FSlateLayoutTransform(1.f, FVector2D(X, 0))), Brush, DrawEffects, GridColor);
			
			const FString LabelText = FText::AsNumber(Index).ToString();
			FSlateDrawElement::MakeText(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(FSlateLayoutTransform(1.f, FVector2D(X + 2.0f, 10.f))), LabelText, Font, DrawEffects, TopTextColor);
		}
		LayerId++;
	}
}

FReply SELuaFramePanel::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	FReply Reply = FReply::Unhandled();

	MousePosOnBtnDown = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
	XPosOnBtnDown = AxisX.GetPos();

	if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		bIsLeftBtnClicked = true;

		Reply = FReply::Handled().CaptureMouse(SharedThis(this));
	}
	else if (MouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
	{
		bIsRightBtnClicked = true;

		Reply = FReply::Handled().CaptureMouse(SharedThis(this));
	}

	return Reply;
}

FReply SELuaFramePanel::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	FReply Reply = FReply::Unhandled();

	MousePosOnBtnUp = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());

	const bool bIsValidForMouseClick = MousePosOnBtnUp.Equals(MousePosOnBtnDown, MOUSE_SNAP_DISTANCE);

	if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		if (bIsLeftBtnClicked)
		{
			if (bIsScrolling)
			{
				bIsScrolling = false;
				CursorType = ECursorType::Default;
			}
			else if (bIsValidForMouseClick)
			{
				OnSelectFrameBar(MousePosOnBtnUp.X, MousePosOnBtnUp.Y);
			}

			bIsLeftBtnClicked = false;

			Reply = FReply::Handled().ReleaseMouseCapture();
		}
	}
	else if (MouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
	{
		if (bIsRightBtnClicked)
		{
			if (bIsScrolling)
			{
				bIsScrolling = false;
				CursorType = ECursorType::Default;
			}
			else if (bIsValidForMouseClick)
			{
				ShowContextMenu(MouseEvent);
			}

			bIsRightBtnClicked = false;

			Reply = FReply::Handled().ReleaseMouseCapture();
		}
	}

	return Reply;
}

FReply SELuaFramePanel::OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	FReply Reply = FReply::Unhandled();

	MousePos = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());

	if (!MouseEvent.GetCursorDelta().IsZero())
	{
		if (MouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton) ||
			MouseEvent.IsMouseButtonDown(EKeys::RightMouseButton))
		{
			if (HasMouseCapture())
			{
				if (!bIsScrolling)
				{
					bIsScrolling = true;
					CursorType = ECursorType::Hand;

					HoveredIndex = -1;
				}

				const float PosX = XPosOnBtnDown + (MousePosOnBtnDown.X - MousePos.X);
				AxisX.ScrollAtValue(AxisX.GetValueAtPos(PosX)); // align viewport position with sample (frame index)
				UpdateHorizontalScrollBar();
				bIsDirty = true;
			}
		}
		else
		{
			if (HoveredIndex < 0)
			{
				TooltipOpacity = 0.0f;
			}

			const float ClampX = FMath::Clamp(MousePos.X, 1.f, AxisX.GetSize() - 1.f);
			HoveredIndex = GetSelectFrameIndexByPos(ClampX, MousePos.Y);
		}

		Reply = FReply::Handled();
	}

	return Reply;
}

void SELuaFramePanel::OnMouseEnter(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
}

void SELuaFramePanel::OnMouseLeave(const FPointerEvent& MouseEvent)
{
	if (!HasMouseCapture())
	{
		bIsLeftBtnClicked = false;
		bIsRightBtnClicked = false;

		HoveredIndex = -1;

		CursorType = ECursorType::Default;
	}
}

FReply SELuaFramePanel::OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	MousePos = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());

	if (MouseEvent.GetModifierKeys().IsShiftDown())
	{
		const float Delta = MouseEvent.GetWheelDelta();
		constexpr float ZoomStep = 0.25f; // as percent
		float ScaleY;

		if (Delta > 0)
		{
			ScaleY = AxisY.GetScale() * FMath::Pow(1.0f + ZoomStep, Delta);
		}
		else
		{
			ScaleY = AxisY.GetScale() * FMath::Pow(1.0f / (1.0f + ZoomStep), -Delta);
		}

		AxisY.SetScale(ScaleY);
	}
	else
	{
		const float Delta = MouseEvent.GetWheelDelta();
		ZoomHorizontally(Delta, MousePos.X);
	}

	return FReply::Handled();
}

void SELuaFramePanel::ZoomHorizontally(const float Delta, const float X)
{
	AxisX.RelativeZoomWithFixedOffset(Delta, X);
	AxisX.ScrollAtValue(AxisX.GetValueAtPos(AxisX.GetPos()));
	UpdateHorizontalScrollBar();
	bIsDirty = true;
}

FReply SELuaFramePanel::OnMouseButtonDoubleClick(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	return FReply::Unhandled();
}

FCursorReply SELuaFramePanel::OnCursorQuery(const FGeometry& MyGeometry, const FPointerEvent& CursorEvent) const
{
	FCursorReply CursorReply = FCursorReply::Unhandled();

	if (CursorType == ECursorType::Arrow)
	{
		CursorReply = FCursorReply::Cursor(EMouseCursor::ResizeLeftRight);
	}
	else if (CursorType == ECursorType::Hand)
	{
		CursorReply = FCursorReply::Cursor(EMouseCursor::GrabHand);
	}

	return CursorReply;
}

void SELuaFramePanel::ShowContextMenu(const FPointerEvent& MouseEvent)
{
	const bool bShouldCloseWindowAfterMenuSelection = true;
	FMenuBuilder MenuBuilder(bShouldCloseWindowAfterMenuSelection, NULL);

	MenuBuilder.BeginSection("Series", LOCTEXT("ContextMenu_Header_Series", "Series"));
	{
		struct FLocal
		{
			static bool ReturnFalse()
			{
				return false;
			}
		};
	}
	MenuBuilder.EndSection();

	MenuBuilder.BeginSection("Misc");
	{
		FUIAction Action_AutoZoom
		(
			FExecuteAction::CreateSP(this, &SELuaFramePanel::ContextMenu_AutoZoom_Execute),
			FCanExecuteAction::CreateSP(this, &SELuaFramePanel::ContextMenu_AutoZoom_CanExecute),
			FIsActionChecked::CreateSP(this, &SELuaFramePanel::ContextMenu_AutoZoom_IsChecked)
		);
		MenuBuilder.AddMenuEntry
		(
			LOCTEXT("ContextMenu_AutoZoom", "Auto Zoom"),
			LOCTEXT("ContextMenu_AutoZoom_Desc", "Enable auto zoom. Makes entire session time range to fit into view."),
			FSlateIcon(),
			Action_AutoZoom,
			NAME_None,
			EUserInterfaceActionType::ToggleButton
		);
	}
	MenuBuilder.EndSection();

	TSharedRef<SWidget> MenuWidget = MenuBuilder.MakeWidget();

	FWidgetPath EventPath = MouseEvent.GetEventPath() != nullptr ? *MouseEvent.GetEventPath() : FWidgetPath();
	const FVector2D ScreenSpacePosition = MouseEvent.GetScreenSpacePosition();
	FSlateApplication::Get().PushMenu(SharedThis(this), EventPath, MenuWidget, ScreenSpacePosition, FPopupTransitionEffect::ContextMenu);
}

void SELuaFramePanel::ContextMenu_AutoZoom_Execute()
{
	bIsAutoZoomEnabled = !bIsAutoZoomEnabled;

	if (bIsAutoZoomEnabled)
	{
		AutoZoom();
	}
}

bool SELuaFramePanel::ContextMenu_AutoZoom_CanExecute()
{
	return true;
}

bool SELuaFramePanel::ContextMenu_AutoZoom_IsChecked()
{
	return bIsAutoZoomEnabled;
}

void SELuaFramePanel::AutoZoom()
{
	AutoZoomPanelPos = AxisX.GetMinPos();
	AxisX.ScrollAtPos(AutoZoomPanelPos);

	AutoZoomPanelSize = AxisX.GetSize();

	if (AutoZoomPanelSize > 0.0f &&
		AxisX.GetMaxValue() - AxisX.GetMinValue() > 0)
	{
		float DX = AxisX.GetMaxPos() - AxisX.GetMinPos();

		// Auto zoom in.
		while (DX < AutoZoomPanelSize)
		{
			const float OldScale = AxisX.GetScale();
			AxisX.RelativeZoomWithFixedOffset(+0.1f, 0.0f);
			AxisX.ScrollAtPos(AutoZoomPanelPos);
			DX = AxisX.GetMaxPos() - AxisX.GetMinPos();
			if (OldScale == AxisX.GetScale())
			{
				break;
			}
		}

		// Auto zoom out (until entire session frame range fits into view).
		while (DX > AutoZoomPanelSize)
		{
			const float OldScale = AxisX.GetScale();
			AxisX.RelativeZoomWithFixedOffset(-0.1f, 0.0f);
			AxisX.ScrollAtPos(AutoZoomPanelPos);
			DX = AxisX.GetMaxPos() - AxisX.GetMinPos();
			if (OldScale == AxisX.GetScale())
			{
				break;
			}
		}
	}

	AutoZoomPanelScale = AxisX.GetScale();

	UpdateHorizontalScrollBar();
	bIsDirty = true;
}

void SELuaFramePanel::BindUICommands()
{
}

void SELuaFramePanel::HorizontalScrollBar_OnUserScrolled(float ScrollOffset)
{
	AxisX.OnUserScrolled(HorizontalScrollBar, ScrollOffset);
	bIsDirty = true;
}

void SELuaFramePanel::UpdateHorizontalScrollBar()
{
	AxisX.UpdateScrollBar(HorizontalScrollBar);
}

#undef LOCTEXT_NAMESPACE

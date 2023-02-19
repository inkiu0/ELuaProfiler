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


#include "ELuaScalableAxis.h"

#include "Widgets/Layout/SScrollBar.h"

bool FELuaScalableAxis::SetSize(const float InSize)
{
	if (!FMath::IsNearlyEqual(Size, InSize, SLATE_UNITS_TOLERANCE))
	{
		Size = InSize;
		return true;
	}
	return false;
}

bool FELuaScalableAxis::SetScale(const double NewScale)
{
	const double LocalNewScale = FMath::Clamp(NewScale, MinScale, MaxScale);
	if (LocalNewScale != Scale)
	{
		Scale = LocalNewScale;
		OnScaleChanged();
		return true;
	}
	return false;
}

bool FELuaScalableAxis::ZoomWithFixedOffset(const double NewScale, const float Offset)
{
	const double LocalNewScale = FMath::Clamp(NewScale, MinScale, MaxScale);
	if (LocalNewScale != Scale)
	{
		Position = (Position + Offset) * static_cast<float>(LocalNewScale / Scale) - Offset;
		Scale = LocalNewScale;

		OnScaleChanged();
		return true;
	}
	return false;
}

bool FELuaScalableAxis::RelativeZoomWithFixedOffset(const float Delta, const float Offset)
{
	constexpr double ZoomStep = 0.25f; // as percent

	double NewScale;

	if (Delta > 0)
	{
		NewScale = Scale * FMath::Pow(1.0f + ZoomStep, Delta);
	}
	else
	{
		NewScale = Scale * FMath::Pow(1.0f / (1.0f + ZoomStep), -Delta);
	}

	return ZoomWithFixedOffset(NewScale, Offset);
}

bool FELuaScalableAxis::UpdatePosWithinLimits()
{
	float MinPos, MaxPos;
	GetScrollLimits(MinPos, MaxPos);
	return EnforceScrollLimits(MinPos, MaxPos, 0.5);
}

void FELuaScalableAxis::GetScrollLimits(float& OutMinPos, float& OutMaxPos)
{
	if (MaxPosition - MinPosition < Size)
	{
		OutMinPos = MaxPosition - Size;
		OutMaxPos = MinPosition;
	}
	else
	{
		constexpr float ExtraSizeFactor = 0.15f; // allow extra 15% on sides
		OutMinPos = MinPosition - ExtraSizeFactor * Size;
		OutMaxPos = MaxPosition - (1.0f - ExtraSizeFactor) * Size;
	}
}

bool FELuaScalableAxis::EnforceScrollLimits(const float InMinPos, const float InMaxPos, const float InterpolationFactor)
{
	float Pos = Position;

	if (Pos < InMinPos)
	{
		Pos = InterpolationFactor * Pos + (1.0f - InterpolationFactor) * InMinPos;

		if (FMath::IsNearlyEqual(Pos, InMinPos, 0.5f))
		{
			Pos = InMinPos;
		}
	}
	else if (Pos > InMaxPos)
	{
		Pos = InterpolationFactor * Pos + (1.0f - InterpolationFactor) * InMaxPos;

		if (FMath::IsNearlyEqual(Pos, InMaxPos, 0.5f))
		{
			Pos = InMaxPos;
		}

		if (Pos < InMinPos)
		{
			Pos = InMinPos;
		}
	}

	return ScrollAtPos(Pos);
}

void FELuaScalableAxis::OnUserScrolled(TSharedPtr<SScrollBar> ScrollBar, float ScrollOffset)
{
	const float SX = 1.0f / (MaxPosition - MinPosition);
	const float ThumbSizeFraction = FMath::Clamp<float>(Size * SX, 0.0f, 1.0f);
	const float OffsetFraction = FMath::Clamp<float>(ScrollOffset, 0.0f, 1.0f - ThumbSizeFraction);

	const float Pos = MinPosition + OffsetFraction * (MaxPosition - MinPosition);
	ScrollAtPos(Pos);

	ScrollBar->SetState(OffsetFraction, ThumbSizeFraction);
}

void FELuaScalableAxis::UpdateScrollBar(TSharedPtr<SScrollBar> ScrollBar) const
{
	const float SX = 1.0f / (MaxPosition - MinPosition);
	const float ThumbSizeFraction = FMath::Clamp<float>(Size * SX, 0.0f, 1.0f);
	const float ScrollOffset = (Position - MinPosition) * SX;
	const float OffsetFraction = FMath::Clamp<float>(ScrollOffset, 0.0f, 1.0f - ThumbSizeFraction);

	ScrollBar->SetState(OffsetFraction, ThumbSizeFraction);
}

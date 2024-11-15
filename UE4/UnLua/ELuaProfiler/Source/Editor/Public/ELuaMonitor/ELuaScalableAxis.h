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

#include "CoreMinimal.h"

class SScrollBar;

class ELUAPROFILER_API FELuaScalableAxis
{
private:
	static constexpr float SLATE_UNITS_TOLERANCE = 0.1f;

public:
	FELuaScalableAxis()
	{
		Reset();
	}

	void Reset()
	{
		Size = 0.0f;

		MinValue = 0.0;
		MaxValue = 0.0;

		MinPosition = 0.0f;
		MaxPosition = 0.0f;
		Position = 0.0f;

		MinScale = 0.01;
		MaxScale = 100.0;
		Scale = 1.0;
	}

	FString ToDebugString(const TCHAR* Sufix) const
	{
		return FString::Printf(TEXT("Scale%s: %g, Pos%s: %.2f"), Sufix, Scale, Sufix, Position);
	}

	float GetSize() const { return Size; }
	bool SetSize(const float InSize);

	int32 GetMinValue() const { return MinValue; }
	int32 GetMaxValue() const { return MaxValue; }

	float GetMinPos() const { return MinPosition; }
	float GetMaxPos() const { return MaxPosition; }
	float GetPos() const { return Position; }

	double GetMinScale() const { return MinScale; }
	double GetMaxScale() const { return MaxScale; }
	double GetScale() const { return Scale; }
	float GetBarWidth() const { return Scale; }

	void SetMinMaxValueInterval(const double InMinValue, const double InMaxValue)
	{
		MinValue = InMinValue;
		MaxValue = InMaxValue;
		UpdateMinMax();
	}

	double GetValueAtPos(const float Pos) const
	{
		return static_cast<double>(Pos) / Scale;
	}

	double GetValueAtOffset(const float Offset) const
	{
		return static_cast<double>(Position + Offset) / Scale;
	}

	float GetPosForValue(const double Value) const
	{
		return static_cast<float>(Value * Scale);
	}

	float GetOffsetForValue(const double Value) const
	{
		return static_cast<float>(Value * Scale) - Position;
	}

	float GetRoundedOffsetForValue(const double Value) const
	{
		return FMath::RoundToFloat(static_cast<float>(Value * Scale) - Position);
	}

	bool ScrollAtPos(const float Pos)
	{
		if (Position != Pos)
		{
			Position = Pos;
			return true;
		}
		return false;
	}

	bool ScrollAtValue(const double Value)
	{
		return ScrollAtPos(GetPosForValue(Value));
	}

	bool CenterOnValue(const double Value)
	{
		return ScrollAtPos(static_cast<float>(Value * Scale) - Size / 2.0f);
	}

	bool CenterOnValueInterval(const double IntervalStartValue, const double IntervalEndValue)
	{
		const float IntervalSize = static_cast<float>(Scale * (IntervalEndValue- IntervalStartValue));
		if (IntervalSize > Size)
		{
			return ScrollAtValue(IntervalStartValue);
		}
		else
		{
			return ScrollAtPos(static_cast<float>(IntervalStartValue * Scale) - (Size - IntervalSize) / 2.0f);
		}
	}

	void SetScaleLimits(const double InMinScale, const double InMaxScale)
	{
		MinScale = InMinScale;
		MaxScale = InMaxScale;
	}
	
	void SetMinMaxInterval(const int32 InMinValue, const int32 InMaxValue)
	{
		MinValue = InMinValue;
		MaxValue = InMaxValue;
		UpdateMinMax();
	}

	bool SetScale(const double NewScale);
	bool ZoomWithFixedOffset(const double NewScale, const float Offset);
	bool RelativeZoomWithFixedOffset(const float Delta, const float Offset);

	void GetScrollLimits(float& OutMinPos, float& OutMaxPos);
	bool EnforceScrollLimits(const float InMinPos, const float InMaxPos, const float InterpolationFactor);
	bool UpdatePosWithinLimits();

	void OnUserScrolled(TSharedPtr<SScrollBar> ScrollBar, float ScrollOffset);
	void UpdateScrollBar(TSharedPtr<SScrollBar> ScrollBar) const;

private:
	void OnScaleChanged()
	{
		UpdateMinMax();
	}

	void UpdateMinMax()
	{
		MinPosition = GetPosForValue(MinValue);
		MaxPosition = GetPosForValue(MaxValue);
	}

private:
	float Size;

	double MinValue;
	double MaxValue;

	float MinPosition;
	float MaxPosition;
	float Position;

	double MinScale;
	double MaxScale;
	double Scale;
};
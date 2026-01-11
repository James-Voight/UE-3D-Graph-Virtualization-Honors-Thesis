#pragma once

#include "CoreMinimal.h"

struct FAxisGridConfig
{
    float AxisMin = 0.0f;
    float AxisMax = 0.0f;
    float TickStep = 1.0f;
    float GridStep = 1.0f;
    int32 NumTicks = 0;
};

namespace DataVizGrid
{
    // Round a value up to a "nice" fraction of 10 (1, 1.5, 2, 2.5, 5, 10)
    inline float NiceNumberCeil(float Value)
    {
        if (Value <= 0.0f)
        {
            return 0.0f;
        }

        const float Exponent = FMath::FloorToFloat(FMath::LogX(10.0f, Value));
        const float Fraction = Value / FMath::Pow(10.0f, Exponent);
        const float Options[] = { 1.0f, 1.5f, 2.0f, 2.5f, 5.0f, 10.0f };

        float NiceFraction = Options[UE_ARRAY_COUNT(Options) - 1];
        for (float Opt : Options)
        {
            if (Fraction <= Opt)
            {
                NiceFraction = Opt;
                break;
            }
        }

        return NiceFraction * FMath::Pow(10.0f, Exponent);
    }

    inline float NiceNumberStep(float Value)
    {
        if (Value <= 0.0f)
        {
            return 1.0f;
        }
        return NiceNumberCeil(Value);
    }

    inline FAxisGridConfig ComputeAxisGrid(float DataMin, float DataMax, int32 TargetTicks = 5)
    {
        if (DataMax < DataMin)
        {
            Swap(DataMin, DataMax);
        }

        FAxisGridConfig Result;
        const float SmallEpsilon = 1e-4f;
        if (FMath::IsNearlyEqual(DataMin, DataMax, SmallEpsilon))
        {
            DataMin -= 1.0f;
            DataMax += 1.0f;
        }

        const bool bAllPositive = DataMin >= 0.0f;
        const bool bAllNegative = DataMax <= 0.0f;

        float AxisMin = 0.0f;
        float AxisMax = 0.0f;

        if (bAllPositive)
        {
            AxisMin = 0.0f;
            AxisMax = NiceNumberCeil(DataMax);
        }
        else if (bAllNegative)
        {
            AxisMax = 0.0f;
            AxisMin = -NiceNumberCeil(FMath::Abs(DataMin));
        }
        else
        {
            const float MaxAbs = FMath::Max(FMath::Abs(DataMin), FMath::Abs(DataMax));
            AxisMax = NiceNumberCeil(MaxAbs);
            AxisMin = -AxisMax;
        }

        float Range = AxisMax - AxisMin;
        if (Range < SmallEpsilon)
        {
            Range = 1.0f;
            AxisMax = AxisMin + Range;
        }

        TargetTicks = FMath::Max(TargetTicks, 2);
        float Step = NiceNumberStep(Range / TargetTicks);
        if (Step < SmallEpsilon)
        {
            Step = 1.0f;
        }

        AxisMin = Step * FMath::FloorToInt(AxisMin / Step);
        AxisMax = Step * FMath::CeilToInt(AxisMax / Step);
        Range = AxisMax - AxisMin;

        const int32 NumTicks = FMath::Clamp(FMath::RoundToInt(Range / Step) + 1, 2, 1000);

        Result.AxisMin = AxisMin;
        Result.AxisMax = AxisMax;
        Result.TickStep = Step;
        Result.GridStep = Step;
        Result.NumTicks = NumTicks;
        return Result;
    }
}

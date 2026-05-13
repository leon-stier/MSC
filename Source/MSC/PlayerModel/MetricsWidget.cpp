#include "MetricsWidget.h"

void UCombatMetricsWidget::AddDataPoint(float FrustrationScore)
{
    FrustrationHistory.Add(FrustrationScore);
    if (FrustrationHistory.Num() > MaxHistorySize)
    {
        FrustrationHistory.RemoveAt(0);
    }
}

int32 UCombatMetricsWidget::NativePaint(
    const FPaintArgs& Args,
    const FGeometry& AllottedGeometry,
    const FSlateRect& MyCullingRect,
    FSlateWindowElementList& OutDrawElements,
    int32 LayerId,
    const FWidgetStyle& InWidgetStyle,
    bool bParentEnabled) const
{
    // Call parent first
    int32 MaxLayer = Super::NativePaint(Args, AllottedGeometry, MyCullingRect,
        OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);

    DrawGraph(AllottedGeometry, OutDrawElements, MaxLayer + 1);

    return MaxLayer + 1;
}

void UCombatMetricsWidget::DrawGraph(
    const FGeometry& AllottedGeometry,
    FSlateWindowElementList& OutDrawElements,
    int32 LayerId) const
{
    if (FrustrationHistory.Num() < 2) return;

	FVector2D WidgetSize = AllottedGeometry.GetLocalSize();
	
	FPaintGeometry PaintGeometry = AllottedGeometry.ToPaintGeometry(
		FVector2D::ZeroVector,
		WidgetSize
	);

    TArray<FVector2D> Points;
    Points.Reserve(FrustrationHistory.Num());

    for (int32 i = 0; i < FrustrationHistory.Num(); i++)
    {
        float X = (float)i / (MaxHistorySize - 1) * WidgetSize.X;

        float NormalizedValue = FMath::Clamp(
            (FrustrationHistory[i] - MinValue) / (MaxValue - MinValue),
            0.f, 1.f);
        float Y = WidgetSize.Y * (1.f - NormalizedValue);

        Points.Add(FVector2D(X, Y));
    }

    FSlateDrawElement::MakeLines(
        OutDrawElements,
        LayerId,
        PaintGeometry,
        Points,
        ESlateDrawEffect::None,
        FLinearColor::Red,
        true,   // bAntialias
        2.f     // thickness
    );

    // Draw a baseline at zero
    TArray<FVector2D> Baseline;
    float BaselineY = WidgetSize.Y * (1.f - 
        FMath::Clamp(-MinValue / (MaxValue - MinValue), 0.f, 1.f));
    Baseline.Add(FVector2D(0.f, BaselineY));
    Baseline.Add(FVector2D(WidgetSize.X, BaselineY));

    FSlateDrawElement::MakeLines(
        OutDrawElements,
        LayerId,
        AllottedGeometry.ToPaintGeometry(),
        Baseline,
        ESlateDrawEffect::None,
        FLinearColor(0.3f, 0.3f, 0.3f, 1.f),
        true,
        1.f
    );
}
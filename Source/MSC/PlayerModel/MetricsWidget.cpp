#include "MetricsWidget.h"

void UCombatMetricsWidget::AddDataPoint(FName DatasetId, float Value)
{
  FGraphDataset& Dataset = Datasets.FindOrAdd(DatasetId);
  if (Dataset.MaxValue <= Dataset.MinValue)
  {
    Dataset.MinValue = DefaultMinValue;
    Dataset.MaxValue = DefaultMaxValue;
  }

  Dataset.Values.Add(Value);
  if (Dataset.Values.Num() > MaxHistorySize)
  {
    Dataset.Values.RemoveAt(0);
  }
}

void UCombatMetricsWidget::SetDatasetStyle(FName DatasetId, FLinearColor Color, float MinValue, float MaxValue)
{
  FGraphDataset& Dataset = Datasets.FindOrAdd(DatasetId);
  Dataset.Color = Color;
  Dataset.MinValue = MinValue;
  Dataset.MaxValue = MaxValue;
}

void UCombatMetricsWidget::ClearDataset(FName DatasetId)
{
  if (FGraphDataset* Dataset = Datasets.Find(DatasetId))
  {
    Dataset->Values.Reset();
  }
}

void UCombatMetricsWidget::ClearAllDatasets()
{
  Datasets.Reset();
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
	FVector2D WidgetSize = AllottedGeometry.GetLocalSize();
  const FPaintGeometry PaintGeometry = AllottedGeometry.ToPaintGeometry();

  for (const TPair<FName, FGraphDataset>& Pair : Datasets)
  {
    const FGraphDataset& Dataset = Pair.Value;
    if (Dataset.Values.Num() < 2) continue;

    const float Range = FMath::Max(Dataset.MaxValue - Dataset.MinValue, KINDA_SMALL_NUMBER);
    TArray<FVector2D> Points;
    Points.Reserve(Dataset.Values.Num());

    for (int32 i = 0; i < Dataset.Values.Num(); i++)
    {
      const float X = (float)i / (MaxHistorySize - 1) * WidgetSize.X;
      const float NormalizedValue = FMath::Clamp(
        (Dataset.Values[i] - Dataset.MinValue) / Range,
        0.f, 1.f);
      const float Y = WidgetSize.Y * (1.f - NormalizedValue);
      Points.Add(FVector2D(X, Y));
    }

    FSlateDrawElement::MakeLines(
      OutDrawElements,
      LayerId,
      PaintGeometry,
      Points,
      ESlateDrawEffect::None,
      Dataset.Color,
      true,   // bAntialias
      2.f     // thickness
    );
  }
}
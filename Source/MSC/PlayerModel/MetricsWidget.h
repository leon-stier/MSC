#pragma once
#include "Blueprint/UserWidget.h"

#include "MetricsWidget.generated.h"

UCLASS()
class UCombatMetricsWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// Call this every frame to feed new data
	UFUNCTION(BlueprintCallable)
	void AddDataPoint(float FrustrationScore);

protected:
	virtual int32 NativePaint(
		const FPaintArgs& Args,
		const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		const FWidgetStyle& InWidgetStyle,
		bool bParentEnabled) const override;

private:
	void DrawGraph(
		const FGeometry& AllottedGeometry,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId) const;

	// Rolling history of values to plot
	TArray<float> FrustrationHistory;
	int32 MaxHistorySize = 300;

	// Graph display range
	float MinValue = 0.f;
	float MaxValue = 20.f;
};
#pragma once
#include "Blueprint/UserWidget.h"

#include "MetricsWidget.generated.h"

USTRUCT()
struct FGraphDataset
{
	GENERATED_BODY()

	TArray<float> Values;
	FLinearColor Color = FLinearColor::White;
	float MinValue = 0.f;
	float MaxValue = 1.f;
};

UCLASS()
class UCombatMetricsWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// Call this every frame to feed new data
	UFUNCTION(BlueprintCallable)
	void AddDataPoint(FName DatasetId, float Value);

	UFUNCTION(BlueprintCallable)
	void SetDatasetStyle(FName DatasetId, FLinearColor Color, float MinValue, float MaxValue);

	UFUNCTION(BlueprintCallable)
	void ClearDataset(FName DatasetId);

	UFUNCTION(BlueprintCallable)
	void ClearAllDatasets();

	UPROPERTY(BlueprintReadOnly)
	float CurrentValue;
	
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

	// Rolling history of values to plot by dataset id
	TMap<FName, FGraphDataset> Datasets;
	int32 MaxHistorySize = 300;

	// Default graph display range for new datasets
	float DefaultMinValue = 0.f;
	float DefaultMaxValue = 20.f;
};
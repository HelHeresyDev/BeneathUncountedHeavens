#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PathRenderer.generated.h"

class USplineComponent;
class UStaticMeshComponent;

UENUM(BlueprintType)
enum class EPathSegmentColor : uint8
{
	Green,
	Red
};

// One drawn run of the path. It spans the inclusive point-index range
// [StartPointIndex, EndPointIndex] and is rendered with Color. The caller
// supplies the exact boundary indices, so color transitions land on the
// provided reaction-eligibility boundary rather than an arbitrary midpoint.
USTRUCT(BlueprintType)
struct FBuhPathSegment
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "BUH|Path")
	int32 StartPointIndex = 0;

	UPROPERTY(BlueprintReadWrite, Category = "BUH|Path")
	int32 EndPointIndex = 1;

	UPROPERTY(BlueprintReadWrite, Category = "BUH|Path")
	EPathSegmentColor Color = EPathSegmentColor::Green;
};

UCLASS()
class BENEATHUNCOUNTEDHEAV_API ABUHPathRenderer : public AActor
{
	GENERATED_BODY()

public:
	ABUHPathRenderer();

	// Draws a path from world-space sample points. Each segment is rendered as
	// one mesh piece per consecutive point pair inside its index range, so the
	// geometry follows the supplied samples (straight lines, segmented runs,
	// and 3D arcs alike). Segments may overlap or tile the full point range.
	UFUNCTION(BlueprintCallable, Category = "BUH|Path")
	void SetPath(const TArray<FVector>& Points, const TArray<FBuhPathSegment>& Segments);

	// Convenience: a single straight piece from Start to End with one color.
	UFUNCTION(BlueprintCallable, Category = "BUH|Path")
	void SetLine(FVector Start, FVector End, EPathSegmentColor Color);

	UFUNCTION(BlueprintCallable, Category = "BUH|Path")
	void ClearPath();

	UFUNCTION(BlueprintCallable, Category = "BUH|Path")
	void SetPathVisible(bool bVisible);

	UFUNCTION(BlueprintPure, Category = "BUH|Path")
	bool IsPathVisible() const;

	// Number of pool pieces currently shown (part of the active path).
	UFUNCTION(BlueprintPure, Category = "BUH|Path")
	int32 GetVisibleSegmentCount() const;

	// Total pool size (high-water mark of pieces ever needed). Used to prove
	// the pool reuses components instead of leaking new ones.
	UFUNCTION(BlueprintPure, Category = "BUH|Path")
	int32 GetPoolSize() const;

	// TEMP DIAGNOSTIC: counts the currently visible pool pieces by material so
	// the live PIE can prove what the renderer actually shows (red vs green).
	UFUNCTION(BlueprintPure, Category = "BUH|Path")
	void GetVisibleColorCounts(int32& OutRed, int32& OutGreen) const;

	UPROPERTY(VisibleAnywhere, Category = "BUH|Path")
	USplineComponent* Spline;

	UPROPERTY(Transient, VisibleAnywhere, Category = "BUH|Path")
	TArray<TObjectPtr<UStaticMeshComponent>> SegmentPool;

	UPROPERTY(EditDefaultsOnly, Category = "BUH|Path")
	TObjectPtr<UMaterialInterface> GreenMaterial;

	UPROPERTY(EditDefaultsOnly, Category = "BUH|Path")
	TObjectPtr<UMaterialInterface> RedMaterial;

	UPROPERTY(EditDefaultsOnly, Category = "BUH|Path")
	TObjectPtr<UStaticMesh> PathMesh;

protected:
	virtual void BeginPlay() override;

	void EnsurePoolSize(int32 Required);

	UPROPERTY(Transient)
	bool bPathVisible = false;

	// Number of pool pieces the most recent SetPath showed. SceneComponent has
	// no public hidden-state getter, so visibility count is tracked here.
	UPROPERTY(Transient)
	int32 LastPathPieceCount = 0;
};

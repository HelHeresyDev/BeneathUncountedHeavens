#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BUHCombatUI.generated.h"

UCLASS()
class BENEATHUNCOUNTEDHEAV_API UBUHCombatUI : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "CombatSim")
	void SetTimelineInstantText(const FString& InText);

	UFUNCTION(BlueprintCallable, Category = "CombatSim")
	void ResetTimelineSlider(const FString& NewValueStr);

	UFUNCTION(BlueprintCallable, Category = "CombatSim")
	void MoveGhostsToInstant(int32 Instant);

	UFUNCTION(BlueprintCallable, Category = "CombatSim")
	void SetupTimelineAndMoveGhosts(int32 PlayerMove, int32 OpponentMove, int32 Instant);
};

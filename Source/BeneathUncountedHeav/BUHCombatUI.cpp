#include "BUHCombatUI.h"
#include "Components/TextBlock.h"
#include "Components/Slider.h"
#include "Components/Image.h"
#include "Components/Border.h"
#include "CombatSim.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "EngineUtils.h"
#include "HAL/IConsoleManager.h"
#include "Blueprint/UserWidget.h"
#include "UObject/UObjectIterator.h"

void UBUHCombatUI::SetTimelineInstantText(const FString& InText)
{
	if (UTextBlock* TB = Cast<UTextBlock>(GetWidgetFromName(FName(TEXT("TimelineInstantText")))))
	{
		TB->SetText(FText::FromString(InText));
	}
}

void UBUHCombatUI::ResetTimelineSlider(const FString& NewValueStr)
{
	if (USlider* S = Cast<USlider>(GetWidgetFromName(FName(TEXT("TimelineSlider")))))
	{
		S->SetValue(FCString::Atod(*NewValueStr));
	}
}

void UBUHCombatUI::MoveGhostsToInstant(int32 Instant)
{
	if (!UBuhCombatSim::bHasTimeline()) return;
	UWorld* World = GetWorld();
	if (!World) return;

	float PlayerX = UBuhCombatSim::GetSnapshotPlayerX(Instant);
	float OpponentX = UBuhCombatSim::GetSnapshotOpponentX(Instant);
	int32 PlayerFacing = UBuhCombatSim::GetSnapshotPlayerFacing(Instant);
	int32 OpponentFacing = UBuhCombatSim::GetSnapshotOpponentFacing(Instant);
	FString InstantText = UBuhCombatSim::GetSnapshotInstantText(Instant);

	float PlayerYaw = (PlayerFacing == 1) ? 0.0f : 180.0f;
	float OpponentYaw = (OpponentFacing == 1) ? 0.0f : 180.0f;

	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		if (!Actor) continue;
		FString Name = Actor->GetName();
		if (Name == TEXT("BP_Fighter_C_2"))
		{
			Actor->SetActorLocation(FVector(PlayerX, 0.0f, 90.0f));
			Actor->SetActorRotation(FRotator(0.0f, PlayerYaw, 0.0f));
		}
		else if (Name == TEXT("BP_Fighter_C_3"))
		{
			Actor->SetActorLocation(FVector(OpponentX, 0.0f, 90.0f));
			Actor->SetActorRotation(FRotator(0.0f, OpponentYaw, 0.0f));
		}
	}

	if (UTextBlock* TB = Cast<UTextBlock>(GetWidgetFromName(FName(TEXT("TimelineInstantText")))))
	{
		TB->SetText(FText::FromString(InstantText));
	}
}

void UBUHCombatUI::SetupTimelineAndMoveGhosts(int32 PlayerMove, int32 OpponentMove, int32 Instant)
{
	FBuhCombatState State = UBuhCombatSim::MakeState(-500.0f, 500.0f, 100.0f, 100.0f, 0);
	TArray<FBuhTimelineSnapshot> Timeline = UBuhCombatSim::GenerateTimeline(State, PlayerMove, OpponentMove, 30);
	UBuhCombatSim::SetCurrentTimeline(Timeline);
	MoveGhostsToInstant(Instant);
}

static FAutoConsoleCommand ScrubTimelineCmd(
	TEXT("BUH.ScrubTimeline"),
	TEXT("Scrub the combat timeline slider to the given instant and broadcast OnValueChanged."),
	FConsoleCommandWithArgsDelegate::CreateStatic([](const TArray<FString>& Args)
	{
		if (Args.Num() < 1) return;
		const int32 Instant = FCString::Atoi(*Args[0]);
		for (UUserWidget* W : TObjectRange<UUserWidget>())
		{
			if (!W) continue;
			if (!W->GetClass()->GetName().Contains(TEXT("WBP_CombatUI"))) continue;
			if (USlider* S = Cast<USlider>(W->GetWidgetFromName(FName(TEXT("TimelineSlider")))))
			{
				S->SetValue(static_cast<float>(Instant));
				break;
			}
		}
	}));

static FAutoConsoleCommand InspectLeftPanelCmd(
	TEXT("BUH.InspectLeftPanel"),
	TEXT("Inspect LeftPanelBG Image and LeftPanel Border visibility/opacity at runtime."),
	FConsoleCommandWithArgsDelegate::CreateStatic([](const TArray<FString>& Args)
	{
		for (UUserWidget* W : TObjectRange<UUserWidget>())
		{
			if (!W) continue;
			if (!W->GetClass()->GetName().Contains(TEXT("WBP_CombatUI"))) continue;

			UImage* LeftPanelBG = Cast<UImage>(W->GetWidgetFromName(FName(TEXT("LeftPanelBG"))));
			UBorder* LeftPanel = Cast<UBorder>(W->GetWidgetFromName(FName(TEXT("LeftPanel"))));

			UE_LOG(LogTemp, Display, TEXT("BUH.InspectLeftPanel: LeftPanelBG found=%d"), LeftPanelBG ? 1 : 0);
			if (LeftPanelBG)
			{
				UE_LOG(LogTemp, Display, TEXT("  Visibility=%d RenderOpacity=%.2f"),
					static_cast<int32>(LeftPanelBG->GetVisibility()), LeftPanelBG->GetRenderOpacity());
			}

			UE_LOG(LogTemp, Display, TEXT("BUH.InspectLeftPanel: LeftPanel found=%d"), LeftPanel ? 1 : 0);
			if (LeftPanel)
			{
				UE_LOG(LogTemp, Display, TEXT("  Visibility=%d RenderOpacity=%.2f"),
					static_cast<int32>(LeftPanel->GetVisibility()), LeftPanel->GetRenderOpacity());
			}
			break;
		}
	}));

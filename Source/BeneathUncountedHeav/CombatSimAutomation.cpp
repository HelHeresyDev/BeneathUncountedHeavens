#include "CombatSim.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBuhCombatSimTest, "BUH.CombatSimTests",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBuhCombatSimTest::RunTest(const FString& Parameters)
{
	const FString Report = UBuhCombatSim::RunAllTests();
	UE_LOG(LogTemp, Display, TEXT("=== BUH.CombatSimTests report ==="));
	UE_LOG(LogTemp, Display, TEXT("%s"), *Report);
	UE_LOG(LogTemp, Display, TEXT("=== end report ==="));

	const FString FilePath = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("CombatTestReport.txt"));
	FFileHelper::SaveStringToFile(Report, *FilePath, FFileHelper::EEncodingOptions::ForceUTF8);

	const bool bAllPassed = Report.Contains(TEXT("0 failed"));
	if (!bAllPassed)
	{
		AddError(TEXT("Combat simulation tests failed. See CombatTestReport.txt for details."));
	}
	return bAllPassed;
}

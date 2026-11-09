#include "PathRenderer.h"
#include "Components/SplineComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"
#include "CombatSim.h"
#include "HAL/IConsoleManager.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Editor.h"
#include "UObject/UnrealType.h"
#include "UObject/Field.h"
#include "Containers/Ticker.h"

ABUHPathRenderer::ABUHPathRenderer()
{
	PrimaryActorTick.bCanEverTick = false;

	Spline = CreateDefaultSubobject<USplineComponent>(TEXT("PathSpline"));
	RootComponent = Spline;
	Spline->SetMobility(EComponentMobility::Movable);
	Spline->SetClosedLoop(false);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		PathMesh = CubeMesh.Object;
	}

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> GreenMat(TEXT("/Game/BUH/Prototypes/TacticalDuel/Materials/M_PathGreen.M_PathGreen"));
	if (GreenMat.Succeeded())
	{
		GreenMaterial = GreenMat.Object;
	}

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> RedMat(TEXT("/Game/BUH/Prototypes/TacticalDuel/Materials/M_PathRed.M_PathRed"));
	if (RedMat.Succeeded())
	{
		RedMaterial = RedMat.Object;
	}
}

void ABUHPathRenderer::BeginPlay()
{
	Super::BeginPlay();
	SetPathVisible(false);
}

void ABUHPathRenderer::EnsurePoolSize(int32 Required)
{
	while (SegmentPool.Num() < Required)
	{
		const int32 Idx = SegmentPool.Num();
		UStaticMeshComponent* Mesh = NewObject<UStaticMeshComponent>(
			this, *FString::Printf(TEXT("PathPiece%d"), Idx));
		if (!Mesh)
		{
			break;
		}
		Mesh->SetupAttachment(RootComponent);
		Mesh->RegisterComponent();
		Mesh->SetMobility(EComponentMobility::Movable);
		Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Mesh->SetGenerateOverlapEvents(false);
		Mesh->SetCastShadow(false);
		Mesh->SetHiddenInGame(true);
		if (PathMesh)
		{
			Mesh->SetStaticMesh(PathMesh);
		}
		SegmentPool.Add(Mesh);
	}
}

void ABUHPathRenderer::SetPath(const TArray<FVector>& Points, const TArray<FBuhPathSegment>& Segments)
{
	if (Points.Num() < 2 || Segments.Num() < 1)
	{
		ClearPath();
		return;
	}

	// Mirror the samples onto the root spline for inspection/debugging.
	Spline->ClearSplinePoints(false);
	for (const FVector& Pt : Points)
	{
		Spline->AddSplinePoint(Pt, ESplineCoordinateSpace::World, false);
	}

	// Count the pieces needed: one per consecutive point pair inside each segment.
	int32 Required = 0;
	for (const FBuhPathSegment& Seg : Segments)
	{
		const int32 S = FMath::Clamp(Seg.StartPointIndex, 0, Points.Num() - 1);
		const int32 E = FMath::Clamp(Seg.EndPointIndex, S + 1, Points.Num() - 1);
		Required += (E - S);
	}
	EnsurePoolSize(Required);

	int32 Piece = 0;
	for (const FBuhPathSegment& Seg : Segments)
	{
		const int32 S = FMath::Clamp(Seg.StartPointIndex, 0, Points.Num() - 1);
		const int32 E = FMath::Clamp(Seg.EndPointIndex, S + 1, Points.Num() - 1);

		UMaterialInterface* Mat = (Seg.Color == EPathSegmentColor::Green) ? GreenMaterial : RedMaterial;

		for (int32 i = S; i < E; ++i)
		{
			if (Piece >= SegmentPool.Num())
			{
				break;
			}
			UStaticMeshComponent* Mesh = SegmentPool[Piece];
			const FVector A = Points[i];
			const FVector B = Points[i + 1];
			const FVector Dir = B - A;
			const float Len = Dir.Size();
			const FVector Normal = Dir.GetSafeNormal();

			Mesh->SetWorldLocation((A + B) * 0.5f);
			Mesh->SetWorldRotation(Normal.Rotation());
			Mesh->SetWorldScale3D(FVector(Len * 0.01f, 0.05f, 0.05f));
			if (Mat)
			{
				Mesh->SetMaterial(0, Mat);
			}
			Mesh->SetHiddenInGame(false);
			++Piece;
		}
	}

	// Hide any pooled pieces beyond the active path.
	for (int32 i = Piece; i < SegmentPool.Num(); ++i)
	{
		SegmentPool[i]->SetHiddenInGame(true);
	}

	LastPathPieceCount = Piece;
	bPathVisible = true;

	// Temporary diagnostic (graphical-PIE investigation): record the last renderer write.
	FString ColorSummary;
	for (const FBuhPathSegment& Seg : Segments)
	{
		ColorSummary += (Seg.Color == EPathSegmentColor::Green) ? TEXT("G") : TEXT("R");
	}
	UE_LOG(LogTemp, Warning, TEXT("DashDiag WRITE SetPath: Points=%d Segments=%d Colors=[%s] First=(%.1f,%.1f) Last=(%.1f,%.1f)"),
		Points.Num(), Segments.Num(), *ColorSummary,
		Points[0].X, Points[0].Y, Points[Points.Num() - 1].X, Points[Points.Num() - 1].Y);
}

void ABUHPathRenderer::SetLine(FVector Start, FVector End, EPathSegmentColor Color)
{
	TArray<FVector> Points;
	Points.Add(Start);
	Points.Add(End);

	TArray<FBuhPathSegment> Segments;
	FBuhPathSegment Seg;
	Seg.StartPointIndex = 0;
	Seg.EndPointIndex = 1;
	Seg.Color = Color;
	Segments.Add(Seg);

	SetPath(Points, Segments);
}

void ABUHPathRenderer::ClearPath()
{
	UE_LOG(LogTemp, Warning, TEXT("DashDiag WRITE ClearPath"));
	Spline->ClearSplinePoints(false);
	for (UStaticMeshComponent* Mesh : SegmentPool)
	{
		Mesh->SetHiddenInGame(true);
	}
	LastPathPieceCount = 0;
	bPathVisible = false;
}

void ABUHPathRenderer::SetPathVisible(bool bVisible)
{
	bPathVisible = bVisible;
	if (!bVisible)
	{
		LastPathPieceCount = 0;
	}
	for (UStaticMeshComponent* Mesh : SegmentPool)
	{
		Mesh->SetHiddenInGame(!bVisible);
	}
}

bool ABUHPathRenderer::IsPathVisible() const
{
	return bPathVisible;
}

int32 ABUHPathRenderer::GetVisibleSegmentCount() const
{
	return bPathVisible ? LastPathPieceCount : 0;
}

int32 ABUHPathRenderer::GetPoolSize() const
{
	return SegmentPool.Num();
}

void ABUHPathRenderer::GetVisibleColorCounts(int32& OutRed, int32& OutGreen) const
{
	OutRed = 0;
	OutGreen = 0;
	if (!bPathVisible)
	{
		return;
	}
	const int32 Count = FMath::Min(LastPathPieceCount, SegmentPool.Num());
	for (int32 i = 0; i < Count; ++i)
	{
		UStaticMeshComponent* Mesh = SegmentPool[i];
		if (!Mesh)
		{
			continue;
		}
		UMaterialInterface* Mat = Mesh->GetMaterial(0);
		if (Mat == GreenMaterial)
		{
			++OutGreen;
		}
		else if (Mat == RedMaterial)
		{
			++OutRed;
		}
	}
}

namespace
{
	// TEMP DIAGNOSTIC: in this headless -nullrhi setup GWorld stays the editor
	// world even while PIE runs, so fetch the PIE world from the engine contexts.
	UWorld* GetBUHPIEWorld()
	{
		if (!GEngine)
		{
			return nullptr;
		}
		for (const FWorldContext& Ctx : GEngine->GetWorldContexts())
		{
			UWorld* CtxWorld = Ctx.World();
			if (CtxWorld && CtxWorld->IsGameWorld())
			{
				return CtxWorld;
			}
		}
		return nullptr;
	}

	// TEMP DIAGNOSTIC: logs the live PIE BP_CombatManager state, then forces a
	// Dash reaction-pause (Action=8,Phase=2,PhaseInstants=3,PlayerMove=8) so the
	// real WBP EventTick draws the Dash path and the DrawDashPath DashDiag log
	// captures the actual Elapsed/RemainingLocked/Red/Green values.
	void RunBUHDashDiag()
	{
		UWorld* World = GWorld;
		if (!World)
		{
			UE_LOG(LogTemp, Warning, TEXT("DashDiag: no GWorld"));
			return;
		}
		FString ActorDump;
		int32 ActorCount = 0;
		AActor* CM = nullptr;
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			FString AN = It->GetName();
			if (ActorCount < 12)
			{
				ActorDump += AN + TEXT(" | ");
			}
			++ActorCount;
			if (AN.Contains(TEXT("BP_CombatManager")) && !CM)
			{
				CM = *It;
			}
		}
		UE_LOG(LogTemp, Warning, TEXT("DashDiag ACTORS in %s (count=%d): %s"), *World->GetName(), ActorCount, *ActorDump);
		if (!CM)
		{
			UE_LOG(LogTemp, Warning, TEXT("DashDiag: no BP_CombatManager in world"));
			return;
		}
		int32 PM = -999, ST = -999, Act = -999, Phase = -999, PhaseIns = -999;
		if (FProperty* P = FindFProperty<FProperty>(CM->GetClass(), TEXT("playerMove"))) PM = *P->ContainerPtrToValuePtr<int32>(CM);
		if (FProperty* P = FindFProperty<FProperty>(CM->GetClass(), TEXT("state"))) ST = *P->ContainerPtrToValuePtr<int32>(CM);
		FStructProperty* CS = FindFProperty<FStructProperty>(CM->GetClass(), TEXT("currentState"));
		if (CS && CS->Struct)
		{
			FBuhCombatState* S = CS->ContainerPtrToValuePtr<FBuhCombatState>(CM);
			if (S->Fighters.Num() > 0)
			{
				Act = S->Fighters[0].Action;
				Phase = S->Fighters[0].Phase;
				PhaseIns = S->Fighters[0].PhaseInstants;
			}
		}
		UE_LOG(LogTemp, Warning, TEXT("DashDiag STATE: playerMove=%d state=%d action=%d phase=%d phaseInstants=%d"), PM, ST, Act, Phase, PhaseIns);
		if (FProperty* P = FindFProperty<FProperty>(CM->GetClass(), TEXT("playerMove"))) *P->ContainerPtrToValuePtr<int32>(CM) = 8;
		if (FProperty* P = FindFProperty<FProperty>(CM->GetClass(), TEXT("targetingAction"))) *P->ContainerPtrToValuePtr<int32>(CM) = -1;
		if (FProperty* P = FindFProperty<FProperty>(CM->GetClass(), TEXT("state"))) *P->ContainerPtrToValuePtr<int32>(CM) = 3;
		if (CS && CS->Struct)
		{
			FBuhCombatState* S = CS->ContainerPtrToValuePtr<FBuhCombatState>(CM);
			S->Fighters.SetNum(2);
			S->Fighters[0].Health = 100; S->Fighters[0].X = -500; S->Fighters[0].Y = 0;
			S->Fighters[0].Action = 8; S->Fighters[0].Phase = 2; S->Fighters[0].PhaseInstants = 3;
			S->Fighters[0].Team = 0; S->Fighters[0].Facing = 1; S->Fighters[0].bCommitted = true;
			S->Fighters[0].DashTarget = 200;
			S->Fighters[1].Health = 100; S->Fighters[1].X = 500; S->Fighters[1].Y = 0;
			S->Fighters[1].Action = 1; S->Fighters[1].Phase = 2; S->Fighters[1].PhaseInstants = 5;
			S->Fighters[1].Team = 1; S->Fighters[1].Facing = -1; S->Fighters[1].bCommitted = true;
			S->Instant = 10; S->Seed = 0;
		}
		UE_LOG(LogTemp, Warning, TEXT("DashDiag SET: playerMove=8 targetingAction=-1 state=3 action=8 phase=2 phaseInstants=3."));
		const int32 Elapsed = 3;
		const int32 RemainingLocked = FMath::Max(0, 3 - Elapsed);
		const FVector Start(-500.f, 0.f, 0.f);
		const FVector End(200.f, 0.f, 0.f);
		UE_LOG(LogTemp, Warning, TEXT("DashDiag DIRECT: Elapsed=%d RemainingLocked=%d (real renderer)"), Elapsed, RemainingLocked);
		for (TActorIterator<ABUHPathRenderer> It(World); It; ++It)
		{
			ABUHPathRenderer* Renderer = *It;
			if (!Renderer) continue;
			if (RemainingLocked <= 0)
			{
				Renderer->SetLine(Start, End, EPathSegmentColor::Green);
			}
			else
			{
				Renderer->SetLine(Start, End, EPathSegmentColor::Red);
			}
			int32 R = 0, G = 0;
			Renderer->GetVisibleColorCounts(R, G);
			UE_LOG(LogTemp, Warning, TEXT("DashDiag DIRECT: drawn %s. Renderer Red=%d Green=%d"),
				RemainingLocked <= 0 ? TEXT("Green") : TEXT("Red"), R, G);
			break;
		}
	}

	TAutoConsoleVariable<int32> CVarBUHDashDiag(TEXT("buhdashdiag"), 0, TEXT("1 = auto-run the BUH Dash reaction-pause diagnostic when PIE starts"));
	int32 sBUHDashDiagFrames = 0;
	bool sBUHDashDiagDone = false;

	bool BUHDashDiagTicker(const float DeltaTime)
	{
		static int32 sTickLogCount = 0;
		if (sTickLogCount < 300 && (sTickLogCount % 5 == 0 || sBUHDashDiagFrames >= 25))
		{
			UE_LOG(LogTemp, Warning, TEXT("DashDiag TICKER: GWorld=%s Done=%d Frames=%d"),
				GWorld ? *GWorld->GetName() : TEXT("null"),
				sBUHDashDiagDone ? 1 : 0,
				sBUHDashDiagFrames);
		}
		++sTickLogCount;
		if (sBUHDashDiagDone)
		{
			return false;
		}
		if (!GWorld)
		{
			sBUHDashDiagFrames = 0;
			return true;
		}
		++sBUHDashDiagFrames;
		if (sBUHDashDiagFrames < 30)
		{
			return true;
		}
		RunBUHDashDiag();
		sBUHDashDiagDone = true;
		return false;
	}

	struct FBUHDashDiagRegistrar
	{
		FBUHDashDiagRegistrar()
		{
			FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateStatic(&BUHDashDiagTicker), 0.f);
		}
	};
	static FBUHDashDiagRegistrar sBUHDashDiagRegistrar;
}

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



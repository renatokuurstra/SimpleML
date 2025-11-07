// Copyright (c) 2025 Renato Kuurstra. Licensed under the MIT License. See LICENSE file in the project root for details.
// GA Integration Tests: end-to-end CQTests for binary (char) and float genomes, sharing setup.

#include "CoreMinimal.h"
#include "CQTest.h"
#include "Containers/StringConv.h"

#include "entt/entt.hpp"

// GA components and systems
#include "Components/GenomeComponents.h"
#include "Components/BreedingPairComponent.h"
#include "Components/EliteComponents.h"
#include "Systems/EliteSelectionCharSystem.h"
#include "Systems/EliteSelectionFloatSystem.h"
#include "Systems/TournamentSelectionSystem.h"
#include "Systems/BreedCharGenomesSystem.h"
#include "Systems/BreedFloatGenomesSystem.h"
#include "Systems/MutationCharGenomeSystem.h"
#include "Systems/MutationFloatGenomeSystem.h"
#include "Systems/GACleanupSystem.h"

// Local helpers for GA integration tests
#include "Helpers/GATestHelper.h"

// SimpleML NN components and systems
#include "Components/NetworkComponent.h"
#include "Components/NNIOComponents.h"
#include "Systems/SimpleMLNNFloatInitSystem.h"
#include "Systems/SimpleMLNNFloatFeedforwardSystem.h"

TEST_CLASS(SimpleML_GA_E2E, "SimpleML.GA.Integration")
{
	// Shared per-test state
	entt::registry Registry;

	// Global GA params
	int32 PopulationSize = 50;
	int32 MaxGenerations = 600; // binary uses 600; float test can override locally if needed
	float BottomResetFraction = 0.4f;
	int32 Seed = 1337;

	// Systems (char + float variants initialized every test; unused ones are harmless)
	UTournamentSelectionSystem* Selection = nullptr;
 UGACleanupSystem* Cleanup = nullptr;

	UEliteSelectionCharSystem* EliteChar = nullptr;
	UBreedCharGenomesSystem* BreederChar = nullptr;
	UMutationCharGenomeSystem* MutatorChar = nullptr;

	UEliteSelectionFloatSystem* EliteFloat = nullptr;
	UBreedFloatGenomesSystem* BreederFloat = nullptr;
	UMutationFloatGenomeSystem* MutatorFloat = nullptr;

	BEFORE_ALL()
	{
	}

	BEFORE_EACH()
	{
		// Initialize selection and cleanup (shared)
		Selection = NewObject<UTournamentSelectionSystem>();
		Selection->Initialize(Registry);
		Selection->TournamentSize = 8;
		Selection->SelectionPressure = 0.7f;
		Selection->bHigherIsBetter = true;
		Selection->CrossGroupParentChance = 0.0f; // single group
		Selection->RandomSeed = Seed + 1;

  Cleanup = NewObject<UGACleanupSystem>();
		Cleanup->Initialize(Registry);

		// Initialize char systems
		EliteChar = NewObject<UEliteSelectionCharSystem>();
		EliteChar->Initialize(Registry);
		EliteChar->EliteCount = 3;
		EliteChar->bHigherIsBetter = true;

		BreederChar = NewObject<UBreedCharGenomesSystem>();
		BreederChar->Initialize(Registry);
		BreederChar->RandomSeed = Seed + 2;

		MutatorChar = NewObject<UMutationCharGenomeSystem>();
		MutatorChar->Initialize(Registry);
		MutatorChar->BitFlipProbability = 0.025f; // 2.5% per-bit
		MutatorChar->RandomSeed = Seed + 3;

		// Initialize float systems
		EliteFloat = NewObject<UEliteSelectionFloatSystem>();
		EliteFloat->Initialize(Registry);
		EliteFloat->EliteCount = 3;
		EliteFloat->bHigherIsBetter = true; // using negative SSE as fitness

		BreederFloat = NewObject<UBreedFloatGenomesSystem>();
		BreederFloat->Initialize(Registry);
		BreederFloat->RandomSeed = Seed + 12;

		MutatorFloat = NewObject<UMutationFloatGenomeSystem>();
		MutatorFloat->Initialize(Registry);
		MutatorFloat->PerValueDeltaPercent = 0.025f; // ±2.5% multiplicative
		MutatorFloat->RandomMutationChance = 0.05f;  // occasional resets
		MutatorFloat->RandomResetMaxPercent = 0.05f; // up to 5% weights reset when triggered
		MutatorFloat->RandomResetMin = -1.0f;
		MutatorFloat->RandomResetMax = 1.0f;
		MutatorFloat->RandomSeed = Seed + 13;
	}

	AFTER_EACH()
	{
		// Deinit in reverse-ish order
		if (EliteChar) { EliteChar->Deinitialize(); EliteChar = nullptr; }
		if (BreederChar) { BreederChar->Deinitialize(); BreederChar = nullptr; }
		if (MutatorChar) { MutatorChar->Deinitialize(); MutatorChar = nullptr; }

		if (EliteFloat) { EliteFloat->Deinitialize(); EliteFloat = nullptr; }
		if (BreederFloat) { BreederFloat->Deinitialize(); BreederFloat = nullptr; }
		if (MutatorFloat) { MutatorFloat->Deinitialize(); MutatorFloat = nullptr; }

		if (Selection) { Selection->Deinitialize(); Selection = nullptr; }
		if (Cleanup) { Cleanup->Deinitialize(); Cleanup = nullptr; }

		Registry.clear();
	}

	AFTER_ALL()
	{
	}

	// Helper: convert bytes to space-separated uppercase hex string
	static FString ToHex(const uint8* Data, int32 Len)
	{
		FString Out; Out.Reserve(Len * 3);
		for (int32 i = 0; i < Len; ++i)
		{
			Out += FString::Printf(TEXT("%02X"), Data[i]);
			if (i + 1 < Len) { Out += TEXT(" "); }
		}
		return Out;
	}

	TEST_METHOD(Converges_To_Target_String_Binary)
	{
		// Local target and population for the binary test
		const FString TargetString = TEXT("Testing GA binaryblob");
		int32 GenomeLen = 0;
		TArray<uint8> TargetBytes;
		{
			FTCHARToUTF8 Converter(*TargetString);
			GenomeLen = Converter.Length();
			TargetBytes.SetNumUninitialized(GenomeLen);
			FMemory::Memcpy(TargetBytes.GetData(), (const void*)Converter.Get(), GenomeLen);
		}

		TArray<TArray<char>> Genomes;
		Genomes.SetNum(PopulationSize, EAllowShrinking::No);
		{
			FRandomStream Rng(Seed);
			for (int32 i = 0; i < PopulationSize; ++i)
			{
				const entt::entity E = Registry.create();
				FGenomeCharViewComponent ViewComp{};
				FFitnessComponent Fit{};
				Fit.Fitness.SetNum(1, EAllowShrinking::No);
				Fit.Fitness[0] = 0.0f;
				Fit.BuiltForFitnessIndex = 0;
				Registry.emplace<FGenomeCharViewComponent>(E, ViewComp);
				Registry.emplace<FFitnessComponent>(E, Fit);
				GATestHelper::InitializeRandomGenome(Genomes[i], Registry.get<FGenomeCharViewComponent>(E), GenomeLen, Rng);
			}
		}

		bool bMatched = false;
		TArray<char> BestSoFar; BestSoFar.SetNum(GenomeLen, EAllowShrinking::No);

		for (int32 Gen = 0; Gen < MaxGenerations; ++Gen)
		{
			// 1) Evaluate fitness
			GATestHelper::ComputeBinaryFitness(Registry, TargetBytes);

			// 1b) Mark only the bottom fraction of entities as reset candidates for next generation (exclude elites)
			{
				struct FEntFit { entt::entity E; float V; int32 Order; };
				TArray<FEntFit> Ranked; Ranked.Reserve(PopulationSize);
				int32 Order = 0;
				auto PopView = Registry.view<FFitnessComponent>(entt::exclude_t<FEliteTagComponent>{});
				for (auto E : PopView)
				{
					const FFitnessComponent& Fit = Registry.get<FFitnessComponent>(E);
					const float V = (Fit.Fitness.Num() > 0) ? Fit.Fitness[0] : -FLT_MAX;
					Ranked.Add({E, V, Order});
					++Order;
				}
				Ranked.Sort([](const FEntFit& A, const FEntFit& B)
				{
					if (A.V != B.V) return A.V < B.V; // lower fitness first
					return A.Order < B.Order;
				});
				const int32 Desired = FMath::Clamp(FMath::FloorToInt(static_cast<float>(PopulationSize) * BottomResetFraction + 1e-6f), 1, PopulationSize);
				const int32 ResetCount = FMath::Min(Desired, Ranked.Num());
				for (int32 i = 0; i < ResetCount; ++i)
				{
					const entt::entity E = Ranked[i].E;
					if (!Registry.all_of<FResetGenomeComponent>(E)) { Registry.emplace<FResetGenomeComponent>(E); }
				}
			}

			// 2) Track best genome and check for match
			float BestFitnessLocal = -FLT_MAX;
			entt::entity BestEntity = entt::null;
			auto View = Registry.view<FFitnessComponent, FGenomeCharViewComponent>();
			for (auto E : View)
			{
				const FFitnessComponent& Fit = Registry.get<FFitnessComponent>(E);
				const float F0 = Fit.Fitness.Num() > 0 ? Fit.Fitness[0] : -FLT_MAX;
				if (F0 > BestFitnessLocal) { BestFitnessLocal = F0; BestEntity = E; }
			}
			if (BestEntity != entt::null)
			{
				const TArrayView<char>& ViewBest = Registry.get<FGenomeCharViewComponent>(BestEntity).Values;
				for (int32 i = 0; i < GenomeLen; ++i) { BestSoFar[i] = (i < ViewBest.Num()) ? ViewBest[i] : 0; }
				bool bEqual = true;
				for (int32 i = 0; i < GenomeLen; ++i) { if (static_cast<uint8>(BestSoFar[i]) != TargetBytes[i]) { bEqual = false; break; } }
				const FString BestHex = ToHex(reinterpret_cast<const uint8*>(BestSoFar.GetData()), GenomeLen);
				const FString TargetHex = ToHex(TargetBytes.GetData(), GenomeLen);
				if ((Gen % 10) == 0)
				{
					UE_LOG(LogTemp, Display, TEXT("[GA Binary Test] Gen=%d BestFitness=%f BestHex=%s TargetHex=%s"), Gen, BestFitnessLocal, *BestHex, *TargetHex);
				}
				if (bEqual)
				{
					UE_LOG(LogTemp, Display, TEXT("[GA Binary Test] Before success Gen=%d BestFitness=%f BestHex=%s TargetHex=%s"), Gen, BestFitnessLocal, *BestHex, *TargetHex);
					bMatched = true; break;
				}
			}

			// 3) Elite selection → tournament selection → breeding → mutation → cleanup
			EliteChar->Update(0.0f);
			Selection->Update(0.0f);
			BreederChar->Update(0.0f);
			MutatorChar->Update(0.0f);
			Cleanup->Update(0.0f);
		}

		// Final evaluation and assertion
		GATestHelper::ComputeBinaryFitness(Registry, TargetBytes);
		float BestFitness = -FLT_MAX;
		auto View2 = Registry.view<FFitnessComponent, FGenomeCharViewComponent>();
		for (auto E : View2)
		{
			const FFitnessComponent& Fit = Registry.get<FFitnessComponent>(E);
			const float F0 = Fit.Fitness.Num() > 0 ? Fit.Fitness[0] : -FLT_MAX;
			if (F0 > BestFitness) { BestFitness = F0; }
		}
		const float MaxBits = static_cast<float>(TargetBytes.Num() * 8);
		const float MaxFitness = MaxBits * MaxBits;
		const float Threshold = 0.95f * MaxFitness; // 95% of bits correct (squared)

		if (BestFitness < Threshold)
		{
			// Log best genome in hex for diagnostics
			entt::entity BestEntityFinal = entt::null;
			float BestFitnessFinal = -FLT_MAX;
			for (auto E : View2)
			{
				const FFitnessComponent& Fit = Registry.get<FFitnessComponent>(E);
				const float F0 = Fit.Fitness.Num() > 0 ? Fit.Fitness[0] : -FLT_MAX;
				if (F0 > BestFitnessFinal) { BestFitnessFinal = F0; BestEntityFinal = E; }
			}
			if (BestEntityFinal != entt::null)
			{
				TArray<char> BestSoFarFinal; BestSoFarFinal.SetNum(TargetBytes.Num(), EAllowShrinking::No);
				const TArrayView<char>& ViewBestFinal = Registry.get<FGenomeCharViewComponent>(BestEntityFinal).Values;
				for (int32 i = 0; i < TargetBytes.Num(); ++i) { BestSoFarFinal[i] = (i < ViewBestFinal.Num()) ? ViewBestFinal[i] : 0; }
				const FString BestHexFinal = ToHex(reinterpret_cast<const uint8*>(BestSoFarFinal.GetData()), TargetBytes.Num());
				const FString TargetHexFinal = ToHex(TargetBytes.GetData(), TargetBytes.Num());
				UE_LOG(LogTemp, Warning, TEXT("GA binary E2E: Best fitness: %f (threshold: %f, max: %f). BestHex=%s TargetHex=%s"), BestFitness, Threshold, MaxFitness, *BestHexFinal, *TargetHexFinal);
			}
		}

		ASSERT_THAT(IsTrue(BestFitness >= Threshold));
	}

	static float ComputeSSE(const TArrayView<float>& Values, const TArray<float>& Tgt)
	{
		const int32 N = FMath::Min(Values.Num(), Tgt.Num());
		double s = 0.0;
		for (int32 i = 0; i < N; ++i)
		{
			const double d = static_cast<double>(Values[i]) - static_cast<double>(Tgt[i]);
			s += d * d;
		}
		return static_cast<float>(s);
	}

	TEST_METHOD(Converges_To_Target_Values_Floats)
	{
		// Slightly more room for float convergence
		PopulationSize = 30;
		BottomResetFraction = 0.3f;

		// Build local population
		const TArray<float> Target = { 0.60f, -0.80f, 0.25f, 0.00f, 0.90f };
		const int32 GenomeLen = Target.Num();
		TArray<TArray<float>> Genomes;
		Genomes.SetNum(PopulationSize, EAllowShrinking::No);
		{
			FRandomStream Rng(Seed);
			for (int32 i = 0; i < PopulationSize; ++i)
			{
				const entt::entity E = Registry.create();
				FGenomeFloatViewComponent ViewComp{};
				FFitnessComponent Fit{};
				Fit.Fitness.SetNum(1, EAllowShrinking::No);
				Fit.Fitness[0] = 0.0f; // will store -SSE
				Fit.BuiltForFitnessIndex = 0;
				Registry.emplace<FGenomeFloatViewComponent>(E, ViewComp);
				Registry.emplace<FFitnessComponent>(E, Fit);
				TArray<float>& Storage = Genomes[i];
				Storage.SetNum(GenomeLen, EAllowShrinking::No);
				for (int32 g = 0; g < GenomeLen; ++g)
				{
					const float v01 = Rng.FRand();
					Storage[g] = FMath::Lerp(-1.0f, 1.0f, v01);
				}
				Registry.get<FGenomeFloatViewComponent>(E).Values = TArrayView<float>(Storage.GetData(), Storage.Num());
			}
		}

		bool bMatched = false;
		TArray<float> BestSoFar; BestSoFar.SetNum(GenomeLen, EAllowShrinking::No);

		for (int32 Gen = 0; Gen < MaxGenerations; ++Gen)
		{
			// 1) Evaluate fitness: fitness = -SSE(Target)
			auto ViewFit = Registry.view<FFitnessComponent, FGenomeFloatViewComponent>();
			for (auto E : ViewFit)
			{
				FFitnessComponent& Fit = Registry.get<FFitnessComponent>(E);
				FGenomeFloatViewComponent& GV = Registry.get<FGenomeFloatViewComponent>(E);
				const float sse = ComputeSSE(GV.Values, Target);
				if (Fit.Fitness.Num() < 1) { Fit.Fitness.SetNum(1, EAllowShrinking::No); }
				Fit.Fitness[0] = -sse; // higher is better
				Fit.BuiltForFitnessIndex = 0;
			}

			// 1b) Mark only the bottom fraction of entities as reset candidates for next generation (exclude elites)
			{
				struct FEntFit { entt::entity E; float V; int32 Order; };
				TArray<FEntFit> Ranked; Ranked.Reserve(PopulationSize);
				int32 Order = 0;
				auto PopView = Registry.view<FFitnessComponent>(entt::exclude_t<FEliteTagComponent>{});
				for (auto E : PopView)
				{
					const FFitnessComponent& Fit = Registry.get<FFitnessComponent>(E);
					const float V = (Fit.Fitness.Num() > 0) ? Fit.Fitness[0] : -FLT_MAX;
					Ranked.Add({E, V, Order});
					++Order;
				}
				Ranked.Sort([](const FEntFit& A, const FEntFit& B)
				{
					if (A.V != B.V) return A.V < B.V; // lower fitness first
					return A.Order < B.Order;
				});
				const int32 Desired = FMath::Clamp(FMath::FloorToInt(static_cast<float>(PopulationSize) * BottomResetFraction + 1e-6f), 1, PopulationSize);
				const int32 ResetCount = FMath::Min(Desired, Ranked.Num());
				for (int32 i = 0; i < ResetCount; ++i)
				{
					const entt::entity E = Ranked[i].E;
					if (!Registry.all_of<FResetGenomeComponent>(E)) { Registry.emplace<FResetGenomeComponent>(E); }
				}
			}

			// 2) Track best genome and check for match (per-gene tolerance)
			float BestFitnessLocal = -FLT_MAX;
			entt::entity BestEntity = entt::null;
			auto View2 = Registry.view<FFitnessComponent, FGenomeFloatViewComponent>();
			for (auto E : View2)
			{
				const FFitnessComponent& Fit = Registry.get<FFitnessComponent>(E);
				const float F0 = Fit.Fitness.Num() > 0 ? Fit.Fitness[0] : -FLT_MAX;
				if (F0 > BestFitnessLocal) { BestFitnessLocal = F0; BestEntity = E; }
			}
			if (BestEntity != entt::null)
			{
				const TArrayView<float>& ViewBest = Registry.get<FGenomeFloatViewComponent>(BestEntity).Values;
				for (int32 i = 0; i < GenomeLen; ++i) { BestSoFar[i] = (i < ViewBest.Num()) ? ViewBest[i] : 0.0f; }
				const float sse = ComputeSSE(ViewBest, Target);
				const float rmse = FMath::Sqrt(sse / FMath::Max(1, GenomeLen));
				if ((Gen % 10) == 0)
				{
					UE_LOG(LogTemp, Display, TEXT("[GA Float Test] Gen=%d BestFitness=-SSE=%f SSE=%.6f RMSE=%.6f Best=(%.3f, %.3f, %.3f, %.3f, %.3f) Target=(%.3f, %.3f, %.3f, %.3f, %.3f)"),
						Gen, BestFitnessLocal, sse, rmse,
						BestSoFar[0], BestSoFar[1], BestSoFar[2], BestSoFar[3], BestSoFar[4],
						Target[0], Target[1], Target[2], Target[3], Target[4]);
				}
				bool bWithinTol = true;
				const float Tol = 1e-2f;
				for (int32 i = 0; i < GenomeLen; ++i) { if (FMath::Abs(BestSoFar[i] - Target[i]) > Tol) { bWithinTol = false; break; } }
				if (bWithinTol)
				{
					UE_LOG(LogTemp, Display, TEXT("[GA Float Test] Before success Gen=%d"), Gen);
					bMatched = true; break;
				}
			}

			// 3) Elite selection → tournament selection → breeding → mutation → cleanup (float variants)
			EliteFloat->Update(0.0f);
			Selection->Update(0.0f);
			BreederFloat->Update(0.0f);
			MutatorFloat->Update(0.0f);
			Cleanup->Update(0.0f);
		}

		// Final evaluation and assertion
		float BestSSE = FLT_MAX;
		auto ViewF = Registry.view<FFitnessComponent, FGenomeFloatViewComponent>();
		entt::entity BestEntityFinal = entt::null;
		for (auto E : ViewF)
		{
			const FFitnessComponent& Fit = Registry.get<FFitnessComponent>(E);
			const float F0 = Fit.Fitness.Num() > 0 ? Fit.Fitness[0] : -FLT_MAX;
			if (BestEntityFinal == entt::null || F0 > Registry.get<FFitnessComponent>(BestEntityFinal).Fitness[0]) { BestEntityFinal = E; }
		}
		if (BestEntityFinal != entt::null)
		{
			const TArrayView<float>& V = Registry.get<FGenomeFloatViewComponent>(BestEntityFinal).Values;
			BestSSE = ComputeSSE(V, Target);
		}
		const float RMSE = FMath::Sqrt(BestSSE / 5.0f);
		const float RMSE_Threshold = 0.02f;
		ASSERT_THAT(IsTrue(bMatched || RMSE <= RMSE_Threshold));
	}
	
	TEST_METHOD(Learns_Rectangle_Area_With_Tiny_NN)
	{
		// GA trains a tiny NN to approximate rectangle area using the SimpleML network and systems.
		// Topology: [2 inputs] -> [3 hidden ReLU] -> [1 output]. Fitness accumulates per-step: pow(1 - |yhat - x*y|, 3).
		PopulationSize = 100;
		BottomResetFraction = 0.35f;
		MaxGenerations = 700; // steps

		// Systems under test: SimpleML NN init and feedforward
		USimpleMLNNFloatInitSystem* NNInit = NewObject<USimpleMLNNFloatInitSystem>();
		USimpleMLNNFloatFeedforwardSystem* NNForward = NewObject<USimpleMLNNFloatFeedforwardSystem>();
		NNInit->Initialize(Registry);
		NNForward->Initialize(Registry);

		// Per-step randomized training samples (x, y) in [0,1]; area = x*y.
		// Deterministic across runs by using a fixed seed.
		FRandomStream RngSamples(Seed + 100);
		TArray<FNeuralNetworkLayerDescriptor> Desc = {
			FNeuralNetworkLayerDescriptor(2),
			FNeuralNetworkLayerDescriptor(3),
			FNeuralNetworkLayerDescriptor(1)
		};
		// Determine genome length from the SimpleML network memory layout for [2]->[1]->[1]
		int32 GenomeLen = 0;
		{
			TNeuralNetwork<float, FNeuron> Tmp;
			
			Tmp.Initialize(Desc);
			GenomeLen = Tmp.GetTotalWeightsCount() + Tmp.GetTotalBiasesCount();
		}

		// Build population: genome storage + NN + IO + fitness
		TArray<TArray<float>> Genomes; Genomes.SetNum(PopulationSize, EAllowShrinking::No);
		TArray<entt::entity> Entities; Entities.Reserve(PopulationSize);
		{
			FRandomStream Rng(Seed);
			for (int32 i = 0; i < PopulationSize; ++i)
			{
				const entt::entity E = Registry.create();
				Entities.Add(E);

				// GA comps
				FGenomeFloatViewComponent ViewComp{};
				FFitnessComponent Fit{};
				Fit.Fitness.SetNum(1, EAllowShrinking::No);
				Fit.Fitness[0] = 0.0f; // stores -SSE
				Fit.BuiltForFitnessIndex = 0;
				Registry.emplace<FGenomeFloatViewComponent>(E, ViewComp);
				Registry.emplace<FFitnessComponent>(E, Fit);

				// SimpleML NN + IO
				FNeuralNetworkFloat Net{};
				
				Net.Initialize(Desc);
				Registry.emplace<FNeuralNetworkFloat>(E, Net);
				FNNInFLoatComp In{}; In.Values.SetNum(2); In.Values[0] = 0; In.Values[1] = 0; Registry.emplace<FNNInFLoatComp>(E, In);
				FNNOutFloatComp Out{}; Out.Values.SetNum(1); Out.Values[0] = 0; Registry.emplace<FNNOutFloatComp>(E, Out);

				// Backing storage for genome + view binding
				TArray<float>& Storage = Genomes[i];
				Storage.SetNum(GenomeLen, EAllowShrinking::No);
				for (int32 g = 0; g < GenomeLen; ++g)
				{
					const float v01 = Rng.FRand();
					Storage[g] = FMath::Lerp(-1.0f, 1.0f, v01);
				}
				Registry.get<FGenomeFloatViewComponent>(E).Values = TArrayView<float>(Storage.GetData(), Storage.Num());
			}
		}

		// Helper: write genome -> network memory using SimpleML Eigen views (row-major order)
		auto MapGenomeToNetwork = [&](FNeuralNetworkFloat& NetComp, const TArrayView<float>& G)
		{
			int32 k = 0;
			const int32 Layers = NetComp.Network.GetNumLayers();
			for (int32 L = 0; L < Layers; ++L)
			{
				auto W = NetComp.Network.GetWeightMatrix(L);
				auto B = NetComp.Network.GetBiasVector(L);
				// Weights: row-major [Output x Input]
				for (int32 r = 0; r < W.rows(); ++r)
				{
					for (int32 c = 0; c < W.cols(); ++c)
					{
						const float v = (k < G.Num()) ? G[k] : 0.0f; ++k;
						W(r, c) = v;
					}
				}
				// Biases
				for (int32 bi = 0; bi < B.size(); ++bi)
				{
					const float v = (k < G.Num()) ? G[k] : 0.0f; ++k;
					B(bi) = v;
				}
			}
		};

		bool bReached = false;
		float BestFitnessEver = -FLT_MAX;

		for (int32 Step = 0; Step < MaxGenerations; ++Step)
		{
			// Ensure networks have randomized weights and bIsInitialized set (once)
			NNInit->Update(0.0f);

			// Map current genomes to network memory for all entities (keeps NN in sync with GA)
			for (int32 i = 0; i < PopulationSize; ++i)
			{
				const entt::entity E = Entities[i];
				FNeuralNetworkFloat& NetComp = Registry.get<FNeuralNetworkFloat>(E);
				const TArrayView<float>& G = Registry.get<FGenomeFloatViewComponent>(E).Values;
				MapGenomeToNetwork(NetComp, G);
			}

			// Per-step random sample (x,y) in [0,1]
			const float X = RngSamples.FRand();
			const float Y = RngSamples.FRand();
			const float Target = X * Y;

			// Set inputs for all entities and evaluate
			for (int32 i = 0; i < PopulationSize; ++i)
			{
				const entt::entity E = Entities[i];
				FNNInFLoatComp& In = Registry.get<FNNInFLoatComp>(E);
				In.Values[0] = X; In.Values[1] = Y;
			}
			NNForward->Update(0.0f);

			// Accumulate fitness: += pow(1 - |yhat - target|, 3)
			for (int32 i = 0; i < PopulationSize; ++i)
			{
				const entt::entity E = Entities[i];
				const FNNOutFloatComp& Out = Registry.get<FNNOutFloatComp>(E);
				const float YHat = (Out.Values.Num() > 0) ? Out.Values[0] : 0.0f;
				const float Diff = FMath::Abs(YHat - Target);
				const float Inc = FMath::Pow(1.0f - FMath::Clamp(Diff, 0.0f, 1.0f), 3.0f);
				FFitnessComponent& Fit = Registry.get<FFitnessComponent>(E);
				Fit.Fitness[0] += Inc; // accumulate
				Fit.BuiltForFitnessIndex = 0;
			}

			// Mark bottom fraction for reset (exclude elites)
			{
				struct FEntFit { entt::entity E; float V; int32 Order; };
				TArray<FEntFit> Ranked; Ranked.Reserve(PopulationSize);
				int32 Order = 0;
				auto PopView = Registry.view<FFitnessComponent>(entt::exclude_t<FEliteTagComponent>{});
				for (auto E : PopView)
				{
					const FFitnessComponent& Fit = Registry.get<FFitnessComponent>(E);
					const float V = (Fit.Fitness.Num() > 0) ? Fit.Fitness[0] : -FLT_MAX;
					Ranked.Add({E, V, Order});
					++Order;
				}
				Ranked.Sort([](const FEntFit& A, const FEntFit& B)
				{
					if (A.V != B.V) return A.V < B.V; // lower fitness first
					return A.Order < B.Order;
				});
				const int32 Desired = FMath::Clamp(FMath::FloorToInt(static_cast<float>(PopulationSize) * BottomResetFraction + 1e-6f), 1, PopulationSize);
				const int32 ResetCount = FMath::Min(Desired, Ranked.Num());
				for (int32 i = 0; i < ResetCount; ++i)
				{
					const entt::entity E = Ranked[i].E;
					if (!Registry.all_of<FResetGenomeComponent>(E)) { Registry.emplace<FResetGenomeComponent>(E); }
				}
			}

			// Track best and maybe early stop
			float BestFitnessLocal = -FLT_MAX; entt::entity BestEntity = entt::null;
			auto ViewBest = Registry.view<FFitnessComponent, FGenomeFloatViewComponent>();
			for (auto E : ViewBest)
			{
				const FFitnessComponent& Fit = Registry.get<FFitnessComponent>(E);
				const float F0 = Fit.Fitness.Num() > 0 ? Fit.Fitness[0] : -FLT_MAX;
				if (F0 > BestFitnessLocal) { BestFitnessLocal = F0; BestEntity = E; }
			}
			if (BestEntity != entt::null)
			{
				if (BestFitnessLocal > BestFitnessEver) { BestFitnessEver = BestFitnessLocal; }
				if ((Step % 25) == 0)
				{
					UE_LOG(LogTemp, Display, TEXT("[GA NN Area x SimpleML] Step=%d BestAccumulatedFitness=%.3f of Max=%d"), Step, BestFitnessLocal, MaxGenerations);
				}
				const float ThresholdAccum = 0.85f * static_cast<float>(MaxGenerations);
				if (BestFitnessLocal >= ThresholdAccum) { bReached = true; break; }
			}

			// GA step using float systems
			EliteFloat->Update(0.0f);
			Selection->Update(0.0f);
			BreederFloat->Update(0.0f);
			MutatorFloat->Update(0.0f);
			Cleanup->Update(0.0f);
		}
		
		// Final evaluation and assertion based on accumulated fitness
		float BestFitnessFinal = -FLT_MAX;
		auto ViewFinal = Registry.view<FFitnessComponent, FGenomeFloatViewComponent>();
		for (auto E : ViewFinal)
		{
			const FFitnessComponent& Fit = Registry.get<FFitnessComponent>(E);
			const float F0 = Fit.Fitness.Num() > 0 ? Fit.Fitness[0] : -FLT_MAX;
			if (F0 > BestFitnessFinal) { BestFitnessFinal = F0; }
		}
		const float AccumThreshold = 0.80f * static_cast<float>(MaxGenerations);
		ASSERT_THAT(IsTrue(bReached || BestFitnessFinal >= AccumThreshold));
		
		// Cleanup NN systems
		NNForward->Deinitialize();
		NNInit->Deinitialize();
	}
};

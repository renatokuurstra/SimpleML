# GeneticAlgorithm Module

Part of the SimpleML plugin, this module provides the core components and systems for implementing Genetic Algorithms (GA) in an ECS-driven architecture using EnTT.

This module is designed to be versatile, supporting both classic **Generational GA** (where the entire population is replaced at once) and **Steady-State GA** (where individuals are evaluated and replaced asynchronously).

- **Generational GA**: Typically used in environments where the fitness function is deterministic and fast to compute. Population-wide resets are handled by external systems or specialized cleanup systems.
- **Steady-State GA**: Often used in complex simulations (like the `SplineCircuitTrainer`) to trade raw exploration speed for stability and the ability to accumulate fitness across multiple samplings.

## Intentions
- Provide a data-driven approach to evolutionary computation.
- Decouple genome representation and fitness evaluation from specific application logic.
- Support both floating-point and byte-oriented (char) genomes.
- Support various evolutionary models including steady-state and generational.

## Basic Usage
1.  **Define Fitness**: Attach `FFitnessComponent` to entities that need evaluation.
2.  **Mark Eligibility**: Attach `FEligibleForBreedingTagComponent` to entities that have completed their evaluation and are ready to be sampled for breeding.
3.  **Selection**: Use `UTournamentSelectionSystem` or custom systems to pick parents based on fitness. Selection systems respect eligibility and elite tags.
4.  **Breeding**: Use `UBreedFloatGenomesSystem` or `UBreedCharGenomesSystem` to create new genomes from selected parents.
5.  **Mutation**: Apply `UMutationFloatGenomeSystem` or `UMutationCharGenomeSystem` to introduce variation.
6.  **Cleanup**: `UGACleanupSystem` can be used to clear breeding pairs and reset tags between evaluation cycles or generations.

## Components
- `FGenomeFloatViewComponent`: Non-owning view into a floating-point genome.
- `FUniqueSolutionComponent`: Stores a unique ID for each solution to ensure identity across entities and generations.
- `FFitnessComponent`: Stores fitness scores for an entity. Expected to be present for all solutions.
- `FEligibleForBreedingTagComponent`: Tag component that marks an entity as a candidate for selection.
- `FResetGenomeComponent`: Tag to mark an entity for genome reconstruction.
- `FEliteTagComponent`: Tag to mark entities as elites to be preserved. Elites are automatically eligible for selection. Elites are ordered and maintained via a pool-based selection that prevents a single solution from occupying multiple elite spots.
- `FBreedingPairComponent`: Links two parents together for breeding.
- `FElitePromotionDebugComponent`: Stores promotion info for visualization (Location, Expiration, Fitness, PopIndex).
- `FGeneticAlgorithmDebugComponent`: Stores summarized GA data for visualization.

## Debugging
The `UGADebugDataSystem` collects information about elites, breeding pairs, and population fitness, storing it in the `FGeneticAlgorithmDebugComponent` for visualization by UI systems.

## APIs
- `UGADebugDataSystem`: Populates `FGeneticAlgorithmDebugComponent`.
- `UTournamentSelectionSystem`: Implements tournament-based selection.
- `UEliteSelectionFloatSystem`: Handles elite preservation.

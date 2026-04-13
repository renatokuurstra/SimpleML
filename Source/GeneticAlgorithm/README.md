# GeneticAlgorithm Module

Part of the SimpleML plugin, this module provides the core components and systems for implementing Genetic Algorithms (GA) in an ECS-driven architecture using EnTT.

## Intentions
- Provide a data-driven approach to evolutionary computation.
- Decouple genome representation and fitness evaluation from specific application logic.
- Support both floating-point and byte-oriented (char) genomes.

## Basic Usage
1.  **Define Fitness**: Attach `FFitnessComponent` to entities that need evaluation. This component is now expected to be present on all entities being evaluated.
2.  **Mark Eligibility**: Attach `FEligibleForBreedingTagComponent` to entities that have completed their evaluation and are ready to be sampled for breeding.
3.  **Selection**: Use `UTournamentSelectionSystem` or custom systems to pick parents based on fitness. Selection systems now respect the eligibility and elite tags.
4.  **Breeding**: Use `UBreedFloatGenomesSystem` to create new genomes from selected parents.
5.  **Mutation**: Apply `UMutationFloatGenomeSystem` or `UMutationCharGenomeSystem` to introduce variation.

## Components
- `FGenomeFloatViewComponent`: Non-owning view into a floating-point genome.
- `FFitnessComponent`: Stores fitness scores for an entity. Expected to be present for all solutions.
- `FEligibleForBreedingTagComponent`: Tag component that marks an entity as a candidate for selection.
- `FResetGenomeComponent`: Tag to mark an entity for genome reconstruction.
- `FEliteTagComponent`: Tag to mark entities as elites to be preserved. Elites are automatically eligible for selection.
- `FBreedingPairComponent`: Links two parents together for breeding.
- `FGeneticAlgorithmDebugComponent`: Stores summarized GA data for visualization.

## Debugging
The `UGADebugDataSystem` collects information about elites, breeding pairs, and population fitness, storing it in the `FGeneticAlgorithmDebugComponent` for visualization by UI systems.

## APIs
- `UGADebugDataSystem`: Populates `FGeneticAlgorithmDebugComponent`.
- `UTournamentSelectionSystem`: Implements tournament-based selection.
- `UEliteSelectionFloatSystem`: Handles elite preservation.

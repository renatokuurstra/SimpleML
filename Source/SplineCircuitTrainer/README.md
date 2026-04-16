# Spline Circuit Trainer Plugin

Plugin for training vehicles on spline-based circuits using ECS and Neural Networks.

## Table of Contents
- [Intentions](#intentions)
- [Basic Usage](#basic-usage)
- [API](#api)
- [Genetic Algorithm Integration](#genetic-algorithm-integration)

The `SplineCircuitTrainer` uses a **Steady-State Genetic Algorithm** model. Unlike classic generational GAs, vehicles are evaluated over their lifetime and reset based on performance triggers or completion of evaluation cycles, rather than resetting the entire population at once. This ensures a more stable learning process for the neural network drivers.

While the underlying `GeneticAlgorithm` module supports both steady-state and generational models, this trainer is specifically configured for steady-state operation to handle the continuous nature of the vehicle simulation.

## Intentions
This plugin provides a high-level context (`AVehicleTrainerContext`) to manage a population of vehicles, evaluate their performance via neural networks, and evolve them using a **steady-state Genetic Algorithm**.

## Basic Usage
1. Create a `UVehicleTrainerConfig` Data Asset.
2. Set up the population size, hidden layers, and GA parameters.
3. Spawn `AVehicleTrainerContext` in your level.
4. Assign the config and a `CircuitActor` (with a Spline Component).
5. The simulation runs continuously; entities are reset and evolved asynchronously based on performance-based triggers (e.g., timeout, off-track, or low velocity).

## API
### AVehicleTrainerManager
- `TrainerConfig`: Configuration used for each context.
- `CircuitActor`: Actor containing the spline circuit to be used by all contexts.
- `NumContexts`: Number of independent contexts to spawn.
- `ContextSeeds`: Optional array to manually assign unique seeds to each context. If empty, seeds are assigned progressively (0, 1, 2, ...).

### AVehicleTrainerContext
- `TrainerConfig`: Configuration data asset.
- `CircuitActor`: Actor containing the spline for the circuit.
- `RandomSeed`: Unique seed for this context, passed down from the Manager.
- `NextGeneration()`: Triggers the GA evolution step.
- `ExecuteEvent("EvaluateNetworks")`: Manually triggers NN feedforward and output application.

### UVehicleTrainerConfig
- `Population`: Number of vehicles.
- `SpawnParametricDistance`: Distance along the spline where vehicles are spawned (default 200cm).
- `Genetic Algorithm|Selection`:
  - `EliteCount`: Number of top performers to preserve.
  - `TournamentSize`: Selection pool size for breeding.
  - `SelectionPressure`: Probability of choosing the best in a tournament.
- `Genetic Algorithm|Breeding`:
  - `MutationRate`: Probability of mutation.
  - `PerValueDeltaPercent`: Multiplicative noise for weights.
  - `BreedingEta`: Controls crossover blend.
- `Genetic Algorithm|Reset Logic`:
  - `MaxSplineDistanceThreshold`: Threshold distance from spline before reset.
  - `NoProgressTimeout`: Max time allowed without significant progress.
  - `MinimumProgressBetweenEvaluations`: Min progress required to reset the timeout timer.
  - `MinAverageVelocity`: Min speed required during the vehicle's lifespan.
  - `ResetReasonConfigs`: Configuration for each reset reason, allowing to block breeding if necessary.
- `Genetic Algorithm|Fitness Eligibility`:
  - `MinBreedAge`: Minimum age in seconds before an entity is eligible for fitness scoring and breeding.
  - `HighestFitnessFactor`: Percentage (0.0 - 1.0) of the highest fitness overall required for fitness eligibility.
- `Genetic Algorithm|Nuke`:
  - `bEnableNuke`: Global toggle for the context nuke system.
  - `StalenessThreshold`: Minimum relative improvement (e.g., 0.01 for 1%) to avoid being marked as stale.
  - `StalenessCooldown`: Time (in seconds) to wait after a nuke before evaluating again.
  - `MinHistoryForStaleness`: Minimum number of historical values (e.g., 50) required before checking for staleness.
- `Debug`:
  - `DebugLogFrequency`: Frequency of the manager's update loop (in seconds). This controls the debug log, UI updates, and the staleness/nuke checks.
  - `bDebugInfo`: Enables debug visualizations and logs.

### FTrainingDataComponent
- `DistanceTraveled`: Total distance along the spline.
- `LastDistanceDelta`: Delta distance since the last update.
- `TimeSinceLastProgress`: Time elapsed since the last positive progress > `MinimumProgressBetweenEvaluations`.
- `CreationTime`: World time when the vehicle was spawned.

## Genetic Algorithm Integration
The trainer uses the following ECS systems to manage the steady-state evolution:
- `UVehicleProgressSystem`: Evaluates vehicle distance along the circuit spline.
- `UVehicleResetFlagSystem`: Flags vehicles for reset if they go off-track or stop making progress.
- `UVehicleFitnessEligibilitySystem`: Manages adding `FEligibleForBreedingTagComponent` based on fitness and highest fitness factor. This tag determines which entities can be sampled as parents by the selection system. Elites are always eligible.
- `UVehicleFitnessSystem`: Accumulates fitness for all entities based on the distance traveled.
- `UEliteSelectionFloatSystem`: Selects top N eligible entities as elites.
- `UTournamentSelectionSystem`: Selects parents for the next iteration from eligible entities and elites.
- `UBreedFloatGenomesSystem`: Creates offspring via crossover.
- `UMutationFloatGenomeSystem`: Applies random variations to offspring.
- `UVehicleResetSystem`: Physically resets vehicles flagged for reset back to the start and resets their fitness, effectively replacing the individual in the steady-state pool.
- `UGACleanupSystem`: Removes transient GA components and the eligibility tag for the next cycle.
- `UGADebugDataSystem`: Collects GA information for visualization.
- `UVehicleTrainerDebugSystem`: Visualizes the collected debug information via the manager's widget.

### Debug Visualization
When `bDebugInfo` is enabled in the `UVehicleTrainerConfig`, a debug panel is displayed in the viewport using **SlateIM**.
- **Reset Count**: Number of entities flagged for reset in the current frame.
- **Elite Info**: Number of elites and a scrollable table of their fitness values.
- **Breeding Info**: Fitness values for the first 4 breeding pairs.
- **Population Fitness**: Top 4 and Bottom 4 fitness values of the entire population (excluding elites).

### UVehicleLibrary
- `GetVehicleSpawnTransform`: Calculates location/rotation at a spline distance with vertical offset.
- `ResetPawnPhysicalState`: Clears velocity and teleports pawn with physics reset.
- `ReasonTooFarFromSpline`: Constant FName for reset reason when too far from spline.
- `ReasonNoProgress`: Constant FName for reset reason when not making progress.
- `ReasonTooSlow`: Constant FName for reset reason when average velocity is too low.

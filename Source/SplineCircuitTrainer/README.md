# Spline Circuit Trainer Plugin

Plugin for training vehicles on spline-based circuits using ECS and Neural Networks.

## Table of Contents
- [Intentions](#intentions)
- [Basic Usage](#basic-usage)
- [API](#api)
- [Genetic Algorithm Integration](#genetic-algorithm-integration)

## Intentions
This plugin provides a high-level context (`AVehicleTrainerContext`) to manage a population of vehicles, evaluate their performance via neural networks, and evolve them using a Genetic Algorithm (GA).

## Basic Usage
1. Create a `UVehicleTrainerConfig` Data Asset.
2. Set up the population size, hidden layers, and GA parameters.
3. Spawn `AVehicleTrainerContext` in your level.
4. Assign the config and a `CircuitActor` (with a Spline Component).
5. Call `NextGeneration()` to evolve the population when they reach the end of a trial.

## API
### AVehicleTrainerContext
- `TrainerConfig`: Configuration data asset.
- `CircuitActor`: Actor containing the spline for the circuit.
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
- `Genetic Algorithm|Fitness Eligibility`:
  - `MinBreedAge`: Minimum age in seconds before an entity is eligible for fitness scoring and breeding.
  - `OldestAliveAgeFactor`: Percentage (0.0 - 1.0) of the oldest currently alive entity's age required for fitness eligibility.
- `Debug`:
  - `bDebugInfo`: Enables debug visualizations (e.g., spheres when vehicles are reset).

### FTrainingDataComponent
- `DistanceTraveled`: Total distance along the spline.
- `LastDistanceDelta`: Delta distance since the last update.
- `TimeSinceLastProgress`: Time elapsed since the last positive progress > `MinimumProgressBetweenEvaluations`.
- `CreationTime`: World time when the vehicle was spawned.

## Genetic Algorithm Integration
The trainer uses the following ECS systems in the `NewGeneration` event:
- `UVehicleProgressSystem`: Evaluates vehicle distance along the circuit spline.
- `UVehicleResetFlagSystem`: Flags vehicles for reset if they go off-track or stop making progress.
- `UVehicleFitnessEligibilitySystem`: Manages adding `FFitnessComponent` based on age and oldest alive factor.
- `UVehicleFitnessSystem`: Calculates fitness as the cube of the distance traveled for eligible entities.
- `UEliteSelectionFloatSystem`: Selects top N entities as elites.
- `UTournamentSelectionSystem`: Selects parents for the next generation.
- `UBreedFloatGenomesSystem`: Creates offspring via crossover.
- `UMutationFloatGenomeSystem`: Applies random variations to offspring.
- `UVehicleResetSystem`: Physically resets vehicles flagged for reset back to the start.
- `UGACleanupSystem`: Removes transient GA components for the next cycle.
- `UGADebugDataSystem`: Collects GA information for visualization (runs during `NewGeneration`).
- `UVehicleTrainerDebugSystem`: Visualizes the collected debug information using SlateIM. This system runs every frame in the `PostUpdate` event to ensure the standalone debug window remains active and responsive.

### Debug Visualization
When `bDebugInfo` is enabled in the `UVehicleTrainerConfig`, a debug panel is displayed in the viewport using **SlateIM**.
- **Reset Count**: Number of entities flagged for reset in the current frame.
- **Elite Info**: Number of elites and a scrollable table of their fitness values.
- **Breeding Info**: Fitness values for the first 4 breeding pairs.
- **Population Fitness**: Top 4 and Bottom 4 fitness values of the entire population (excluding elites).

### UVehicleLibrary
- `GetVehicleSpawnTransform`: Calculates location/rotation at a spline distance with vertical offset.
- `ResetPawnPhysicalState`: Clears velocity and teleports pawn with physics reset.

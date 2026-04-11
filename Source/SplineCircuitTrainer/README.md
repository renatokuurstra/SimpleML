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
- `EliteCount`: Number of top performers to preserve.
- `TournamentSize`: Selection pool size for breeding.
- `MutationRate`: Probability of mutation.
- `PerValueDeltaPercent`: Multiplicative noise for weights.

## Genetic Algorithm Integration
The trainer uses the following ECS systems in the `NewGeneration` event:
- `UVehicleProgressSystem`: Evaluates vehicle distance along the circuit spline.
- `UVehicleResetFlagSystem`: Flags vehicles for reset if they go off-track or stop making progress.
- `UVehicleFitnessSystem`: Calculates fitness as the cube of the distance traveled.
- `UEliteSelectionFloatSystem`: Selects top N entities as elites.
- `UTournamentSelectionSystem`: Selects parents for the next generation.
- `UBreedFloatGenomesSystem`: Creates offspring via crossover.
- `UMutationFloatGenomeSystem`: Applies random variations to offspring.
- `UVehicleResetSystem`: Physically resets vehicles flagged for reset back to the start.
- `UGACleanupSystem`: Removes transient GA components for the next cycle.

### UVehicleLibrary
- `GetVehicleSpawnTransform`: Calculates location/rotation at a spline distance with vertical offset.
- `ResetPawnPhysicalState`: Clears velocity and teleports pawn with physics reset.

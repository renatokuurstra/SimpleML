# SimpleML Plugin

SimpleML is an Unreal Engine plugin focused on lightweight, ECS-first machine learning utilities. It favors data-oriented design and stateless systems operating on POD-style component data.

Important: The GeneticAlgorithm module implements a steady-state GA (not classic generational). This choice trades raw exploration speed for stability and the ability to accumulate fitness across multiple samplings — which is crucial when optimizing complex models like neural networks.

## Table of Contents
- [Requirements](#requirements)
- [Installation](#installation)
- [Modules](#modules)
- [Usage Basics](#usage-basics)
- [Public API (overview)](#public-api-overview)
- [Testing](#testing)
- [Design Guidelines](#design-guidelines)
- [License](#license)

## Requirements
- Unreal Engine 5.6 (or compatible)
- REQUIRED dependency: UEcs plugin (Entity Component System)
  - Repository: https://github.com/renatokuurstra/UEcs
  - SimpleML depends on UEcs types like `UEcsSystem` and ECS components. Ensure UEcs is installed and enabled in your project.

## Installation
1. Clone or copy the `SimpleML` plugin folder into your project under `Plugins/SimpleML`.
2. Install the `UEcs` plugin:
   - Option A: Clone into your project as `Plugins/UEcs`:
     ```
     cd <YourProject>
     git submodule add https://github.com/renatokuurstra/UEcs Plugins/UEcs
     ```
   - Option B: Copy an existing `UEcs` plugin folder into `Plugins/UEcs`.
3. Ensure the `SimpleML.uplugin` lists UEcs as a dependency. In this repo it’s already declared:
   ```json
   "Plugins": [
     { "Name": "UEcs", "Enabled": true }
   ]
   ```
4. Open your project in Unreal Editor and enable both `UEcs` and `SimpleML` if not already enabled. Rebuild when prompted.

## Modules
SimpleML currently provides the following modules:
- `SimpleMLInterfaces` (Runtime): Minimal interfaces for ML-controlled entities.
- `SimpleML` (Runtime): Base module for shared ML scaffolding (Neural Networks, Activation functions).
- `SimpleMLTests` (Developer): Tests for the SimpleML module.
- `GeneticAlgorithm` (Runtime): A flexible module for genome representation, breeding, selection, and mutation, supporting both **Steady-State** and **Generational** Genetic Algorithms.
- `GeneticAlgorithmTests` (Developer): Test/experimental code that depends on `GeneticAlgorithm`.
- `SplineCircuitTrainer` (Runtime): ECS context for training vehicles on a spline circuit using a **Steady-State GA**.
- `SplineCircuitTrainerTests` (Developer): Tests for the SplineCircuitTrainer module.

## Usage Basics
- ECS-first: author systems deriving from `UEcsSystem` and operate on simple component data.
- Avoid UObject bloat: use `USTRUCT` components with `TArray`/`TArrayView` for data, reserve `UObject` only when reflection/editor features are required.
- Example components (GeneticAlgorithm):
  - `FGenomeFloatViewComponent`, `FGenomeCharViewComponent`, `FFitnessComponent`, `FResetGenomeComponent`.
- Example systems (GeneticAlgorithm):
  - Breeding: `UBreedFloatGenomesSystem`, `UBreedCharGenomesSystem`
  - Selection: `UEliteSelectionFloatSystem`
    - Maintains a persistent pool of elite entities per fitness index. Elites are only replaced if a new candidate achieves better fitness, ensuring that the best solutions are never lost during the simulation.
  - Mutation: `UMutationFloatGenomeSystem` (per-value ±X% noise, optional random resets)

## Public API (overview)
Key headers (under `Plugins/SimpleML/Source/SimpleML/Public`):
- `NeuralNetwork.h`: Templated neural network `TNeuralNetwork<T, TNeuron>` with contiguous memory layout.
  - `Initialize(const TArray<FNeuralNetworkLayerDescriptor>& InLayerDescriptors, int32 Seed = 0)`: Initializes the network structure and weights.
  - `InitializeWeights(int32 Seed = 0)`: Re-initializes weights using Xavier/He initialization with a specific seed for determinism.
  - `InitializeWeightsUniform(T Min, T Max, int32 Seed = 0)`: Re-initializes weights and biases with uniform random values.

Key headers (under `Plugins/SimpleML/Source/SimpleMLInterfaces/Public`):
- `VehicleNNInterface.h`: Defines `ISimpleMLVehicleNNInterface`.

Key headers (under `Plugins/SimpleML/Source/GeneticAlgorithm/Public`):
- Components: `Components/GenomeComponents.h`
- Systems:
  - `Systems/BreedFloatGenomesSystem.h`
  - `Systems/BreedCharGenomesSystem.h`
  - `Systems/EliteSelectionFloatSystem.h`
  - `Systems/MutationFloatGenomeSystem.h`
  - `Systems/MutationCharGenomeSystem.h`

  ### SplineCircuitTrainer
  - `AVehicleTrainerManager`: Actor that manages multiple independent `AVehicleTrainerContext` instances. It spawns and initializes them using a shared `UVehicleTrainerConfig` and `CircuitActor`. It also manages the Debug UI widget (toggled with 'H').
  - `AVehicleTrainerContext`: ECS context (AEcsContext) with references to an actor with a spline circuit and a trainer configuration. Each context is independent and maintains its own EnTT registry.
  - `UVehicleTrainerConfig`: Data asset holding trainer settings. It automatically manages the neural network structure based on inputs (spline distance, velocity, future path, recurrence) and hidden layer configuration. Includes `SpawnVerticalOffset` for positioning vehicles, `MinAverageVelocity` (cm/s) and `MinAgeForReset` (seconds) for performance-based reset logic. Includes `bDebugInfo` and `FitnessHistoryLength` for visualization.
  - `UVehicleTrainerDebugWidget`: C++ base class for the UMG debug widget. Provides `DrawLineGraph` helper and `UpdateContextData` event.
  - `UVehicleEntityFactory`: ECS system that spawns pawns and creates entities. It initializes the neural network based on `UVehicleTrainerConfig` parameters and sets the `CreationTime` for performance tracking.
  - `UVehicleNNInputSystem`: ECS system that generates inputs for the neural network based on vehicle position relative to the spline, velocity, and previous outputs (recurrence).
  - `UVehicleProgressSystem`: ECS system that evaluates the progress of vehicle pawns along a spline, handling looped splines and forward/backward movement.
  - `UVehicleResetFlagSystem`: ECS system that flags vehicles for reset if they deviate too far from the spline or fail to maintain a minimum average velocity over their lifespan.
  - `UVehicleNNOutputSystem`: ECS system that applies the neural network outputs back to the vehicle pawn via `ISimpleMLVehicleNNInterface`.
  - `FVehicleComponent`: ECS component that stores a reference to the spawned pawn.
  - `FTrainingDataComponent`: ECS component that tracks the distance traveled along the spline and the `CreationTime` of the entity.

### SimpleMLInterfaces
- `ISimpleMLVehicleNNInterface`: Interface to be implemented by vehicle pawns to receive neural network outputs.
  - `ApplyNNOutputs(TArrayView<const float> Outputs)`: Applies NN control values to the vehicle.

These systems are small, focused, and stateless; they operate purely on the provided component data.

## Testing
- A developer module `GeneticAlgorithmTests` exists to host CQTests. It depends on `GeneticAlgorithm`.
- The included example tests (e.g., byte/char target-string convergence) are intentionally simple and are typically better suited to a classic generational GA. Such problems often converge faster when the whole population is refreshed each generation.
- We use a steady-state GA because the primary target is harder domains like neural networks (NNs), where fitness must be accumulated over multiple samplings/episodes to reduce variance and correctly estimate performance. Keeping individuals alive across updates makes that accumulation feasible and prevents degenerate reseeding each step.
- To add your own tests, place them under `Plugins/SimpleML/Source/GeneticAlgorithmTests` and mirror common setup/teardown using `BEFORE_EACH`/`AFTER_EACH` where possible.

## Design Guidelines
- Plugins-first architecture; minimal inter-plugin dependencies.
- ECS-first mindset and POD-style data.
- Follow Epic coding standard (PascalCase classes, camelCase functions, b-prefixed booleans, brace style, etc.).
- Header hygiene: prefer forward declarations, avoid monolithic includes.
- Comments explain why, not what.

## License
Copyright (c) 2025 Renato Kuurstra. MIT License. See `LICENSE` in the project root.

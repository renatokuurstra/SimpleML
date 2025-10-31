# SimpleML Plugin

SimpleML is an Unreal Engine plugin focused on lightweight, ECS-first machine learning utilities. It favors data-oriented design and stateless systems operating on POD-style component data.

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
- `SimpleML` (Runtime): Base module for shared ML scaffolding.
- `GeneticAlgorithm` (Runtime): Systems and components for genome representation, breeding, selection, and mutation.
- `GeneticAlgorithmTests` (Developer): Test/experimental code that depends on `GeneticAlgorithm`.

## Usage Basics
- ECS-first: author systems deriving from `UEcsSystem` and operate on simple component data.
- Avoid UObject bloat: use `USTRUCT` components with `TArray`/`TArrayView` for data, reserve `UObject` only when reflection/editor features are required.
- Example components (GeneticAlgorithm):
  - `FGenomeFloatViewComponent`, `FGenomeCharViewComponent`, `FFitnessComponent`, `FResetGenomeComponent`.
- Example systems (GeneticAlgorithm):
  - Breeding: `UBreedFloatGenomesSystem`, `UBreedCharGenomesSystem`
  - Selection: `UEliteSelectionFloatSystem`
  - Mutation: `UMutationFloatGenomeSystem` (per-value ±X% noise, optional random resets)

## Public API (overview)
Key headers (under `Plugins/SimpleML/Source/GeneticAlgorithm/Public`):
- Components: `Components/GenomeComponents.h`
- Systems:
  - `Systems/BreedFloatGenomesSystem.h`
  - `Systems/BreedCharGenomesSystem.h`
  - `Systems/EliteSelectionFloatSystem.h`
  - `Systems/MutationFloatGenomeSystem.h`
  - `Systems/MutationCharGenomeSystem.h`

These systems are small, focused, and stateless; they operate purely on the provided component data.

## Testing
- A developer module `GeneticAlgorithmTests` exists to host CQTests. It depends on `GeneticAlgorithm`.
- To add tests, place them under `Plugins/SimpleML/Source/GeneticAlgorithmTests` and mirror common setup/teardown using BEFORE_EACH/AFTER_EACH where possible.

## Design Guidelines
- Plugins-first architecture; minimal inter-plugin dependencies.
- ECS-first mindset and POD-style data.
- Follow Epic coding standard (PascalCase classes, camelCase functions, b-prefixed booleans, brace style, etc.).
- Header hygiene: prefer forward declarations, avoid monolithic includes.
- Comments explain why, not what.

## License
Copyright (c) 2025 Renato Kuurstra. MIT License. See `LICENSE` in the project root.

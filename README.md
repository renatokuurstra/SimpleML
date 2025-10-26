# SimpleML Plugin

SimpleML provides machine learning building blocks for Unreal Engine with a focus on data-oriented design and ECS-first workflows.

## Table of Contents
- Overview
- Modules
  - SimpleML (Core)
  - Genetic Algorithm
- Public API
  - Genetic Algorithm Components
  - Genetic Algorithm Systems
- Usage
- Dependencies
- Notes on Design

## Overview
SimpleML is structured as multiple modules. The `GeneticAlgorithm` module hosts POD-style ECS components and stateless systems to compose GA workflows (mutation, selection). Arrays and data are designed for cache friendliness and integration with `UEcs`.

## Modules
- SimpleML: Core ML utilities and neural network types.
- Genetic Algorithm: ECS-first genetic algorithm components and systems.

## Public API

### Genetic Algorithm Components
- `FGenomeFloatViewComponent`
  - `TArrayView<float> Values` — non-owning view over a contiguous float genome buffer.
- `FGenomeCharViewComponent`
  - `TArrayView<char> Values` — non-owning view over a contiguous char/byte genome buffer.
- `FFitnessComponent`
  - `TArray<float> Fitness` — owning array storing fitness scores, aligned to entity order.

### Genetic Algorithm Systems
- `UMutationFloatGenomeSystem` (depends on `FGenomeFloatViewComponent`)
  - Boilerplate system for genome mutation with float views (logic to be defined later).
- `UTournamentSelectionSystem` (depends on `FFitnessComponent`)
  - Boilerplate system for tournament-based parent selection (to be defined).
- `UEliteSelectionSystem` (depends on `FFitnessComponent`)
  - Boilerplate system for elite selection (top-N preservation) (to be defined).

## Usage
1. Enable the `SimpleML` plugin. The `GeneticAlgorithm` module is part of it and loads by default.
2. Ensure the `UEcs` plugin is enabled; systems derive from `UEcsSystem`.
3. Register entities with relevant components (e.g., attach `FGenomeFloatViewComponent` with a view into your genome storage and `FFitnessComponent` for fitness values).
4. Instantiate systems as needed and call `Initialize` with your ECS `entt::registry`. Call `Update` per tick.

Note: `TArrayView` members are non-owning. You must maintain the lifetime of underlying buffers and keep them contiguous and stable while systems operate.

## Dependencies
- `UEcs` (public dependency of the `GeneticAlgorithm` module)

## Notes on Design
- ECS-first and data-oriented design: keep systems small and focused; avoid UObject bloat.
- Header hygiene: minimal includes; prefer forward declarations where possible.
- Mutation and selection logic is intentionally left as TODO to be defined later.

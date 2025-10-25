# GeneticAlgorithm Module

Table of Contents
- Overview
- Dependencies
- Folder Structure
- Usage

Overview
The GeneticAlgorithm module lives inside the SimpleML plugin and is intended to host ECS-first components and systems for genetic algorithms.

Dependencies
- UEcs (plugin): Provides ECS context, entities, and systems framework.
- Core (UE): Base engine types.

Folder Structure
- Public/
  - GeneticAlgorithm.h: Module API header
  - Components/: ECS component USTRUCTs (POD-style)
  - Systems/: Stateless ECS systems operating on components
- Private/
  - GeneticAlgorithmModule.cpp: Module implementation entry point

Usage
- Add systems and components in the respective folders. Keep systems small and stateless.
- Avoid UObject-heavy types; prefer USTRUCTs for data.
- Keep inter-module dependencies minimal and data-driven.

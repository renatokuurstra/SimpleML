#pragma once

#include "Modules/ModuleManager.h"

/**
 * GeneticAlgorithm module entry point.
 * This module is scaffolded to host ECS-oriented systems and components for genetic algorithms.
 */
class FGeneticAlgorithmModule : public IModuleInterface
{
public:
    virtual void StartupModule() override {}
    virtual void ShutdownModule() override {}
};

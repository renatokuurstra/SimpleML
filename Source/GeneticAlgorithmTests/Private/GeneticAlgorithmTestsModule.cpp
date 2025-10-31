// Copyright (c) 2025 Renato Kuurstra. Licensed under the MIT License. See LICENSE file in the project root for details.

#include "Modules/ModuleManager.h"

class FGeneticAlgorithmTestsModule : public IModuleInterface
{
public:
	virtual void StartupModule() override {}
	virtual void ShutdownModule() override {}
};

IMPLEMENT_MODULE(FGeneticAlgorithmTestsModule, GeneticAlgorithmTests)

#pragma once

#include "Modules/ModuleManager.h"

/**
 * Module entry point for the plugin's Blueprint-graph module. UncookedOnly: it is needed wherever
 * Blueprints are compiled and is absent from a cooked build. Nothing includes this header - the
 * nodes the module ships register themselves through GetMenuActions.
 */
class FStillCookingCoreEditorModule : public IModuleInterface
{
public:
	//~ Begin IModuleInterface
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	//~ End IModuleInterface
};

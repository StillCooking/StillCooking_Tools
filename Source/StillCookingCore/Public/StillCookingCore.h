#pragma once

#include "Modules/ModuleManager.h"

/** Module entry point for StillCookingCore — the plugin's runtime utility module. */
class FStillCookingCoreModule : public IModuleInterface
{
public:
	//~ Begin IModuleInterface
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	//~ End IModuleInterface
};

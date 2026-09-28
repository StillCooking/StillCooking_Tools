// Static console command path: stubs registered at module load whose bodies live in a Blueprint child
// of USCConsoleCommandRegistrySubsystem. The whole file is compiled out of Shipping, because a static
// FAutoConsoleCommand registers itself the moment the module loads - a guard inside the callback would
// be too late.
//
// WithWorldAndArgs, never plain FAutoConsoleCommand: the world argument is the whole reason this path
// needs no cached registry pointer. A consumer of the plugin writes a stub like the one below in their
// own module and never touches the registry class.
//
// Naming: SC.<Area>.<Action>, PascalCase. The dots group the commands in the console's autocomplete,
// which UFUNCTION(Exec) cannot do.

#include "Debug/SCConsoleCommandRegistrySubsystem.h"

#if !UE_BUILD_SHIPPING

#include "HAL/IConsoleManager.h"

static FAutoConsoleCommandWithWorldAndArgs GSCCmdPing(
	TEXT("SC.Debug.Ping"),
	TEXT("Raises the On Static Command event with the identifier 'Ping'. Usage: SC.Debug.Ping [Args...]"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda(
		[](const TArray<FString>& Args, UWorld* World)
		{
			USCConsoleCommandRegistrySubsystem::DispatchStaticCommand(World, TEXT("Ping"), Args);
		}),
	ECVF_Cheat);

#endif // !UE_BUILD_SHIPPING

#include "Modules/ModuleInterface.h"
#include "EcsactUnreal/RuntimeHandle.h"
#if WITH_EDITOR
#	include "Editor.h"
#endif

class ECSACT_API FEcsactGameModuleImpl : public IModuleInterface {
	FEcsactRuntimeHandle RuntimeHandle;

#if WITH_EDITOR
	auto OnPreBeginPIE(bool bIsSimulating) -> void;
	auto OnShutdownPIE(const bool bIsSimulating) -> void;
#endif

public:
	/**
	 * Loads the default configured ecsact runtime at game start (PIE or
	 * standalone.)
	 */
	auto StartupModule() -> void override;

	/**
	 * Unloads the default configured ecsact runtime at game end (PIE or
	 * standalone.)
	 */
	auto ShutdownModule() -> void override;

	[[nodiscard]] auto IsGameModule() const -> bool final {
		return true;
	}
};

#include "EcsactUnreal/EcsactExecution.h"
#include "EcsactUnreal/Ecsact.h"

float EcsactUnrealExecution::DeltaTime_ = 0.f;

auto EcsactUnrealExecution::DeltaTime() -> float {
	return DeltaTime_;
}

auto EcsactUnrealExecution::Runner( //
	class UWorld* World
) -> TWeakObjectPtr<class UEcsactRunner> {
	check(World);
	auto mod = FEcsactModule::Get();
	for(auto i = 0; mod.RunnerWorlds.Num() > i; ++i) {
		if(mod.RunnerWorlds[i].Get() == World) {
			return mod.Runners[i];
		}
	}

	return {};
}

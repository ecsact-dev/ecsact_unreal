#include "EcsactUnreal/EcsactExecution.h"
#include "EcsactUnreal/Ecsact.h"

float EcsactUnrealExecution::DeltaTime_ = 0.f;

auto EcsactUnrealExecution::DeltaTime() -> float {
	return DeltaTime_;
}

auto EcsactUnrealExecution::Runner() -> TWeakObjectPtr<class UEcsactRunner> {
	return FEcsactModule::Get().Runner;
}

#include "EcsactExecution.h"
#include "Ecsact.h"

float EcsactUnrealExecution::DeltaTime_ = 0.f;

auto EcsactUnrealExecution::DeltaTime() -> float {
	return DeltaTime_;
}

auto EcsactUnrealExecution::Runner() -> class UEcsactRunner* {
	return FEcsactModule::Get().Runner;
}

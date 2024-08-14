#include "EcsactExecution.h"

float EcsactUnrealExecution::DeltaTime_ = 0.f;

auto EcsactUnrealExecution::DeltaTime() -> float {
	return DeltaTime_;
}

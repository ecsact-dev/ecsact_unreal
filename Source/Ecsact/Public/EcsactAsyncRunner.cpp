#include "EcsactAsyncRunner.h"
#include "ecsact/runtime/async.h"

auto UEcsactAsyncRunner::Tick(float DeltaTime) -> void {
	check(ecsact_async_flush_events != nullptr);
	ecsact_async_flush_events(nullptr, nullptr);
}

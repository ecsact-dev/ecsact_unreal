#include "EcsactAsyncRunner.h"
#include "Ecsact.h"
#include "ecsact/runtime/async.h"

auto UEcsactAsyncRunner::Tick(float DeltaTime) -> void {
	if(ecsact_async_flush_events == nullptr) {
		UE_LOG(Ecsact, Error, TEXT("ecsact_async_flush_events is unavailable"));
	} else {
		ecsact_async_flush_events(nullptr, nullptr);
	}
}

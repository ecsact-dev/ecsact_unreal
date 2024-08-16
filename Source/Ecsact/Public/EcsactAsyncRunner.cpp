#include "EcsactAsyncRunner.h"
#include "Ecsact.h"
#include "EcsactUnrealExecutionOptions.h"
#include "EcsactUnrealEventsCollector.h"
#include "ecsact/runtime/async.h"
#include "ecsact/runtime/common.h"

auto UEcsactAsyncRunner::Tick(float DeltaTime) -> void {
	EnqueueExecutionOptions();

	if(ecsact_async_flush_events == nullptr) {
		UE_LOG(Ecsact, Error, TEXT("ecsact_async_flush_events is unavailable"));
	} else {
		ecsact_execution_events_collector* evc_c = nullptr;
		if(EventsCollector != nullptr) {
			evc_c = EventsCollector->GetCEVC();
		}
		ecsact_async_flush_events(evc_c, nullptr);
	}
}

auto UEcsactAsyncRunner::EnqueueExecutionOptions() -> void {
	if(!ExecutionOptions) {
		return;
	}
	if(ecsact_async_enqueue_execution_options == nullptr) {
		UE_LOG(
			Ecsact,
			Error,
			TEXT("ecsact_async_enqueue_execution_options is unavailable")
		);
		return;
	}

	if(ExecutionOptions->IsNotEmpty()) {
		ecsact_async_enqueue_execution_options(*ExecutionOptions->GetCPtr());
		ExecutionOptions->Clear();
	}
}

auto UEcsactAsyncRunner::GetStatId() const -> TStatId {
	RETURN_QUICK_DECLARE_CYCLE_STAT(UEcsactAsyncRunner, STATGROUP_Tickables);
}

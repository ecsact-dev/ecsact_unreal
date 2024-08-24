#include "EcsactUnreal/EcsactBlueprintLibrary.h"
#include "ecsact/runtime/async.h"

auto UEcsactBlueprintLibrary::AsyncDisconnect() -> void {
	ecsact_async_disconnect();
}

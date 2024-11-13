#include "EcsactUnreal/RuntimeHandle.h"

FEcsactRuntimeHandle::FEcsactRuntimeHandle() {
	DllHandle = nullptr;
}

FEcsactRuntimeHandle::FEcsactRuntimeHandle(FEcsactRuntimeHandle&& Other) {
	DllHandle = Other.DllHandle;
	Other.DllHandle = nullptr;
}

FEcsactRuntimeHandle::~FEcsactRuntimeHandle() = default;

auto FEcsactRuntimeHandle::operator=( //
	FEcsactRuntimeHandle&& Other
) -> FEcsactRuntimeHandle& {
	DllHandle = Other.DllHandle;
	Other.DllHandle = nullptr;
	return *this;
}

FEcsactRuntimeHandle::operator bool() const {
	return DllHandle != nullptr;
}

auto EcsactUnreal::Detail::SetHandle(
	FEcsactRuntimeHandle& Handle,
	void*                 DllHandle
) -> void {
	Handle.DllHandle = DllHandle;
}

// Copyright (c) 2025 Seaube Software CORP. <https://seaube.com>
//
// This file is part of the Ecsact Unreal plugin.
// Distributed under the MIT License. (See accompanying file LICENSE or view
// online at <https://github.com/ecsact-dev/ecsact_unreal/blob/main/LICENSE>)

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

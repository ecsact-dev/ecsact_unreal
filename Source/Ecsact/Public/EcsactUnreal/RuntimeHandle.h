#pragma once

class FEcsactRuntimeHandle;

namespace EcsactUnreal::Detail {
/** @internal - DO NOT USE */
auto SetHandle(FEcsactRuntimeHandle& Handle, void* DllHandle) -> void;
} // namespace EcsactUnreal::Detail

/**
 * Opaque handle for a loaded ecsact runtime.
 */
class FEcsactRuntimeHandle {
	friend class FEcsactModule;

	friend auto EcsactUnreal::Detail::SetHandle(
		FEcsactRuntimeHandle& Handle,
		void*                 DllHandle
	) -> void;

private:
	void* DllHandle;

public:
	FEcsactRuntimeHandle();
	FEcsactRuntimeHandle(FEcsactRuntimeHandle&&);
	~FEcsactRuntimeHandle();

	auto operator=(FEcsactRuntimeHandle&&) -> FEcsactRuntimeHandle&;

	operator bool() const;
};

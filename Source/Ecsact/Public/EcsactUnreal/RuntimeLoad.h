#pragma once

#include "EcsactUnreal/Ecsact.h"
#include "EcsactUnreal/RuntimeHandle.h"
#include "ecsact/runtime.h"

/** @internal - DO NOT USE */
#define ECSACT_API_FN_INIT_(fn, UNUSED_PARAM) decltype(fn) fn = nullptr

/** @internal - DO NOT USE */
#define ECSACT_API_FN_RESET_(fn, UNUSED_PARAM) fn = nullptr

/** @internal - DO NOT USE */
#define ECSACTAPI_FN_FN_LOAD_(fn, UNUSED_PARAM)                           \
	fn = reinterpret_cast<decltype(fn)>(                                    \
		FPlatformProcess::GetDllExport(ecsact_runtime_dll_handle_, TEXT(#fn)) \
	);                                                                      \
	if(fn != nullptr) {                                                     \
		UE_LOG(Ecsact, Log, TEXT("loaded %s"), TEXT(#fn));                    \
	}                                                                       \
	static_assert(true, "require ;")

/**
 * Call exactly once in your game modules .cpp file.
 */
#define ECSACT_RUNTIME_INIT() \
	FOR_EACH_ECSACT_API_FN(ECSACT_API_FN_INIT_, UNUSED_PARAM);

// Check if we're using clangd on windows to shutup the tranditional msvc error
// TODO: https://github.com/ecsact-dev/ecsact_runtime/issues/267
#if defined(__clang__) && defined(_WIN32)
#	define ECSACT_LOAD_RUNTIME()     \
		([]() -> FEcsactRuntimeHandle { \
			/* this should never be hit*/ \
			check(false);                 \
			return {};                    \
		})()
#	define ECSACT_UNLOAD_RUNTIME(Handle)
#else

/**
 * Loads the ecsact runtime functions. With a dylib runtime all available
 * ecsact_* functions will be set, but if unavailable will be NULL. Calling
 * ECSACT_LOAD_RUNTIME() multiple times without calling ECSACT_UNLOAD_RUNTIME()
 * is not allowed.
 *
 * NOTE: this is a preprocessor macro so that the function pointers in your
 * hot-reloaded module are correct. Otherwise the loaded function pointers will
 * only be on the initial process's DLLs.
 *
 * @example
 * ```cpp
 * FEcsactRuntimeHandle Handle = ECSACT_LOAD_RUNTIME();
 * ```
 */
#	define ECSACT_LOAD_RUNTIME()                                            \
		([]() -> FEcsactRuntimeHandle {                                        \
			auto module = FEcsactModule::Get();                                  \
			if(!EcsactUnreal::Detail::CheckRuntimeNotLoaded(module)) {           \
				return {};                                                         \
			}                                                                    \
			auto ecsact_runtime_dll_handle_ =                                    \
				EcsactUnreal::Detail::GetDefaultRuntimeDllHandle();                \
			if(!ecsact_runtime_dll_handle_) {                                    \
				return {};                                                         \
			}                                                                    \
			auto result = FEcsactRuntimeHandle{};                                \
			EcsactUnreal::Detail::SetHandle(result, ecsact_runtime_dll_handle_); \
			if(!EcsactUnreal::Detail::CheckRuntimeHandle(module, result)) {      \
				return {};                                                         \
			}                                                                    \
			FOR_EACH_ECSACT_API_FN(ECSACTAPI_FN_FN_LOAD_);                       \
			return result;                                                       \
		})()

/**
 * Unload a previously loaded ecsact runtime via ECSACT_LOAD_RUNTIME
 */
#	define ECSACT_UNLOAD_RUNTIME(Handle)                             \
		([](FEcsactRuntimeHandle& Handle_) -> void {                    \
			auto module = FEcsactModule::Get();                           \
			if(!EcsactUnreal::Detail::CheckUnloadable(module, Handle_)) { \
				return;                                                     \
			}                                                             \
			if(ecsact_async_disconnect != nullptr) {                      \
				ecsact_async_disconnect();                                  \
			}                                                             \
			EcsactUnreal::Detail::UnloadPostDisconnect(module, Handle_);  \
			FOR_EACH_ECSACT_API_FN(ECSACT_API_FN_RESET_);                 \
			EcsactUnreal::Detail::UnloadPostReset(module, Handle_);       \
		})(Handle)

#endif

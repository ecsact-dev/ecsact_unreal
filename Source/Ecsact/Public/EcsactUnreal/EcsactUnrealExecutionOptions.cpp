#include "EcsactUnreal/EcsactUnrealExecutionOptions.h"

UEcsactUnrealExecutionOptions::UEcsactUnrealExecutionOptions() : ExecOpts({}) {
}

auto UEcsactUnrealExecutionOptions::GetCPtr() -> ecsact_execution_options* {
	return &ExecOpts;
}

auto UEcsactUnrealExecutionOptions::IsNotEmpty() const -> bool {
	return ExecOpts.actions_length > 0 || ExecOpts.create_entities_length > 0 ||
		ExecOpts.add_components_length > 0 ||
		ExecOpts.destroy_entities_length > 0 ||
		ExecOpts.update_components_length > 0 ||
		ExecOpts.remove_components_length > 0;
}

auto UEcsactUnrealExecutionOptions::Clear() -> void {
	for(auto act : ActionList) {
		FMemory::Free(const_cast<void*>(act.action_data));
	}
	for(auto comp : AddComponentList) {
		FMemory::Free(const_cast<void*>(comp.component_data));
	}
	for(auto comp : UpdateComponentList) {
		FMemory::Free(const_cast<void*>(comp.component_data));
	}
	ActionList.Empty();
	AddComponentList.Empty();
	UpdateComponentList.Empty();
	RemoveComponentList.Empty();
	ExecOpts = {};
}

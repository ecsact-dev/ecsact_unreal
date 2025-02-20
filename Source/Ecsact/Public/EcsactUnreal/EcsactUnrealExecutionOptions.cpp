#include "EcsactUnreal/EcsactUnrealExecutionOptions.h"
#include "EcsactUnreal/Ecsact.h"
#include "ecsact/runtime/common.h"

using CreateEntityBuilder = UEcsactUnrealExecutionOptions::CreateEntityBuilder;

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
		ExecOpts.remove_components_length > 0 ||
		ExecOpts.create_entities_length > 0;
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
	DestroyEntityList.Empty();
	CreateEntityList.Empty();
	CreateEntityComponentsList.Empty();
	CreateEntityComponentsListData.Empty();
	CreateEntityComponentsListNums.Empty();
	ExecOpts = {};
}

#if WITH_EDITORONLY_DATA
auto UEcsactUnrealExecutionOptions::DebugLog() const -> void {
	if(!IsNotEmpty()) {
		UE_LOG(Ecsact, Warning, TEXT("(Empty Ecsact Execution Options)"));
		return;
	}

	UE_LOG(Ecsact, Log, TEXT(" == Ecsact Execution Options =="));
	UE_LOG(Ecsact, Log, TEXT("\tActions Length: %i"), ExecOpts.actions_length);
	for(auto i = 0; ExecOpts.actions_length > i; ++i) {
		UE_LOG(
			Ecsact,
			Log,
			TEXT("\t\tActionId: %i"),
			ExecOpts.actions[i].action_id
		);
	}
	UE_LOG(
		Ecsact,
		Log,
		TEXT("\tCreate Entities Length: %i"),
		ExecOpts.create_entities_length
	);
	for(auto i = 0; ExecOpts.create_entities_length > i; ++i) {
		UE_LOG(
			Ecsact,
			Log,
			TEXT("\t\tPlaceholderId: %i"),
			ExecOpts.create_entities[i]
		);
		for(auto ci = 0; ExecOpts.create_entities_components_length[i] > ci; ++ci) {
			auto comp_count = ExecOpts.create_entities_components_length[i];
			auto comps = ExecOpts.create_entities_components[ci];
			UE_LOG(
				Ecsact,
				Log,
				TEXT("\t\tCreate Entity Component Count: %i"),
				comp_count
			);
			for(auto cci = 0; cci > comp_count; ++cci) {
				auto comp = comps[cci];
				UE_LOG(Ecsact, Log, TEXT("\t\t\tComponentId: %i"), comp.component_id);
			}
		}
	}
}
#endif

auto UEcsactUnrealExecutionOptions::CreateEntity(
	ecsact_placeholder_entity_id PlaceholderId
) -> CreateEntityBuilder {
	return {this, PlaceholderId};
}

CreateEntityBuilder::CreateEntityBuilder(
	UEcsactUnrealExecutionOptions* Owner,
	ecsact_placeholder_entity_id   PlaceholderId
)
	: bValid(true), Owner(Owner), PlaceholderId(PlaceholderId) {
}

CreateEntityBuilder::CreateEntityBuilder(CreateEntityBuilder&& Other) {
	bValid = Other.bValid;
	Owner = Other.Owner;
	PlaceholderId = Other.PlaceholderId;
	ComponentList = std::move(Other.ComponentList);

	Other.bValid = false;
	Other.Owner = nullptr;
	Other.PlaceholderId = {};
	Other.ComponentList = {};
}

auto CreateEntityBuilder::operator=( //
	CreateEntityBuilder&& Other
) -> CreateEntityBuilder& {
	bValid = Other.bValid;
	Owner = Other.Owner;
	PlaceholderId = Other.PlaceholderId;
	ComponentList = std::move(Other.ComponentList);

	Other.bValid = false;
	Other.Owner = nullptr;
	Other.PlaceholderId = {};
	Other.ComponentList = {};
	return *this;
}

CreateEntityBuilder::~CreateEntityBuilder() {
	if(bValid) {
		Finish();
	}
}

auto CreateEntityBuilder::Finish() -> void {
	if(!bValid) {
		return;
	}

	if(!Owner) {
		UE_LOG(Ecsact, Error, TEXT("Cannot create entity - Runner not valid"));
		return;
	}

	Owner->CreateEntityList.Add(PlaceholderId);
	Owner->CreateEntityComponentsListNums.Add(ComponentList.Num());
	Owner->CreateEntityComponentsList.Push(std::move(ComponentList));
	Owner->CreateEntityComponentsListData.Empty();
	for(auto& list : Owner->CreateEntityComponentsList) {
		Owner->CreateEntityComponentsListData.Add(list.GetData());
	}

	Owner->ExecOpts.create_entities_length = Owner->CreateEntityList.Num();
	Owner->ExecOpts.create_entities = Owner->CreateEntityList.GetData();
	Owner->ExecOpts.create_entities_components_length =
		Owner->CreateEntityComponentsListNums.GetData();
	Owner->ExecOpts.create_entities_components =
		Owner->CreateEntityComponentsListData.GetData();

	bValid = false;
	Owner = nullptr;
	ComponentList = {};
}

#include "EcsactUnreal/EcsactUnrealExecutionOptions.h"
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

auto UEcsactUnrealExecutionOptions::CreateEntity(
	ecsact_placeholder_entity_id PlaceholderId
) -> CreateEntityBuilder {
	return {this, PlaceholderId};
}

CreateEntityBuilder::CreateEntityBuilder(
	UEcsactUnrealExecutionOptions* Owner,
	ecsact_placeholder_entity_id   PlacerholderId
)
	: bValid(true), Owner(Owner), PlaceholderId(PlaceholderId) {
}

CreateEntityBuilder::CreateEntityBuilder(CreateEntityBuilder&& Other) {
	bValid = Other.bValid;
	Owner = Other.Owner;
	ComponentList = std::move(Other.ComponentList);

	Other.bValid = false;
	Other.Owner = nullptr;
	Other.ComponentList = {};
}

CreateEntityBuilder::~CreateEntityBuilder() {
	if(bValid && Owner) {
		Finish();
	}
}

auto CreateEntityBuilder::Finish() -> void {
	if(!bValid || !Owner) {
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

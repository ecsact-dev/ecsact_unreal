#include "EcsactRunner.h"


auto UEcsactRunner::Tick(float DeltaTime) -> void {

}
auto UEcsactRunner::GetStatId() const -> TStatId {
	RETURN_QUICK_DECLARE_CYCLE_STAT(UTickableObject, STATGROUP_Tickables);
}

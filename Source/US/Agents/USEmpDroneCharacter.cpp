#include "Agents/USEmpDroneCharacter.h"
#include "Components/USEmpPulseComponent.h"

AUSEmpDroneCharacter::AUSEmpDroneCharacter()
{
	// 캡슐(루트)에 붙여 둥실 연출에 흔들리지 않게 한다. 바닥 높이·판 메시·고리 머티리얼은 BP에서 잡는다.
	EmpPulse = CreateDefaultSubobject<UUSEmpPulseComponent>(TEXT("EmpPulse"));
	EmpPulse->SetupAttachment(GetRootComponent());

	// EMP기는 총을 들지 않는다
	DefaultWeaponClass = nullptr;
}

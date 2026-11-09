#include "Core/USLoadingScreenSettings.h"

UUSLoadingScreenSettings::UUSLoadingScreenSettings()
{
	LoadingLabel = NSLOCTEXT("USLoadingScreen", "LoadingLabel", "로딩 중");

	Tips.Add(NSLOCTEXT("USLoadingScreen", "Tip0", "적을 좌클릭하면 그 적을 집중 공격합니다. 빈 곳을 클릭하면 해제됩니다."));
	Tips.Add(NSLOCTEXT("USLoadingScreen", "Tip1", "보급선이 오면 가까이 가서 Q를 누르세요. 골드로 시간과 화력을 살 수 있습니다."));
	Tips.Add(NSLOCTEXT("USLoadingScreen", "Tip2", "같은 계열 카드를 모으면 빌드 완성 보너스를 받습니다!"));
	Tips.Add(NSLOCTEXT("USLoadingScreen", "Tip3", "출격에서 번 스크랩으로 격납고 설계도를 강화하세요."));
}

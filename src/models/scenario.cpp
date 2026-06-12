#include "scenario.h"
#include "factory.h"

// =============================================================================
// 각 시나리오는 Factory의 config API 만 사용한다 (머신 직접 접근 X).
// =============================================================================
void FreePlay::apply(Factory& f) const {
    f.setAllBreakdownProb(0.0005f);
    f.setSpawnInterval(30);
}

void NormalFlow::apply(Factory& f) const {
    f.setAllBreakdownProb(0.0f);
    f.setSpawnInterval(30);
}

void RandomBreakdown::apply(Factory& f) const {
    f.setAllBreakdownProb(0.002f);
    f.setSpawnInterval(30);
}

void Bottleneck::apply(Factory& f) const {
    f.setAllBreakdownProb(0.0f);
    f.setSpawnInterval(12);                     // 투입 증가
    f.setProcessTicksByName("Oven", 20);        // 오븐을 느리게 → 앞단 백업(병목)
}

// =============================================================================
// 레지스트리 — 새 시나리오 추가 시 여기 한 줄만 늘리면 된다.
// =============================================================================
int scenarioCount() { return 4; }

std::string scenarioName(int idx) {
    return makeScenario(idx)->name();
}

std::unique_ptr<Scenario> makeScenario(int idx) {
    switch (idx) {
        case 1:  return std::make_unique<NormalFlow>();
        case 2:  return std::make_unique<RandomBreakdown>();
        case 3:  return std::make_unique<Bottleneck>();
        default: return std::make_unique<FreePlay>();
    }
}

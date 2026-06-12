#include "scenario.h"
#include "factory.h"

// =============================================================================
// 각 시나리오는 Factory의 config API 만 사용한다 (머신 직접 접근 X).
// =============================================================================
void NormalFlow::apply(Factory& f) const {
    f.setAllBreakdownProb(0.0f);
    f.setSpawnInterval(30);
}

void RandomBreakdown::apply(Factory& f) const {
    f.setAllBreakdownProb(0.002f); // 첫 머신 제외 
    f.setSpawnInterval(30);
}

void Bottleneck::apply(Factory& f) const {
    f.setAllBreakdownProb(0.0f);
    f.setSpawnInterval(15);                     // 투입을 빠르게(라인을 가득 채워 적체 부각)
    f.setProcessTicksByName("Oven", 40);        // 오븐만 매우 느리게 → 처리량이 오븐에 묶이고
                                                // 앞단 벨트/머신이 가득 차 백업(병목)
}

void Overflow::apply(Factory& f) const {
    f.setAllBreakdownProb(0.0f);
    f.setSpawnInterval(10);
    f.setAllConveyorLength(1);
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
        case 1:  return std::make_unique<Bottleneck>();
        case 2:  return std::make_unique<RandomBreakdown>();
        case 3:  return std::make_unique<Overflow>();
        default: return std::make_unique<NormalFlow>();
    }
}

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
    f.setProcessTicksByName("Oven", 12);  // 중간 병목 (기본 6 < 12 < Bottleneck 40)
    f.setSpawnInterval(3);                // 병목 배출량을 크게 초과하는 투입 → 초과분이 손실
    // 주: 손실은 '입력 - 병목 배출'로만 결정됨(실측). 벨트 길이는 손실에 영향 없어 조작하지 않음.
}

void FreePlay::apply(Factory& f) const {
    f.setOrdersEnabled(true);       // 게임 모드 — 주문/경제 레이어 활성
    f.setAllBreakdownProb(0.0008f); // 약한 고장 확률
    f.setSpawnInterval(30);
}

// =============================================================================
// 레지스트리 — 새 시나리오 추가 시 여기 한 줄만 늘리면 된다.
// =============================================================================
int scenarioCount() { return 5; }

std::string scenarioName(int idx) {
    return makeScenario(idx)->name();
}

std::unique_ptr<Scenario> makeScenario(int idx) {
    switch (idx) {
        case 1:  return std::make_unique<Bottleneck>();
        case 2:  return std::make_unique<RandomBreakdown>();
        case 3:  return std::make_unique<Overflow>();
        case 4:  return std::make_unique<FreePlay>();
        default: return std::make_unique<NormalFlow>();
    }
}

#include "scenario.h"
#include "factory.h"
#include <random>

// =============================================================================
// 각 시나리오는 Factory의 config API 만 사용한다 (머신 직접 접근 X).
// =============================================================================
void NormalFlow::apply(Factory& f) const {
    f.setAllBreakdownProb(0.0f);
    f.setSpawnInterval(30);
}

void RandomBreakdown::apply(Factory& f) const {
    f.setAllBreakdownProb(0.001f); // 전 머신 틱당 고장 확률(동시다발 줄이려 0.002→0.001 하향)
    f.setSpawnInterval(30);
}

void Bottleneck::apply(Factory& f) const {
    // 병목 위치를 매 로드마다 무작위로 — 항상 오븐만 느린 단조로움 제거.
    static const char* kCandidates[] = {
        "Sauce Spreader", "Cheese Spreader", "Topping Applier", "Oven", "Cutter"
    };
    static std::mt19937 rng(std::random_device{}());
    const char* slow = kCandidates[std::uniform_int_distribution<int>(0, 4)(rng)];

    f.setAllBreakdownProb(0.0f);
    f.setSpawnInterval(15);              // 투입을 빠르게(라인을 가득 채워 적체 부각)
    f.setProcessTicksByName(slow, 40);   // 무작위 한 단계만 매우 느리게 → 그 앞단이 백업(병목)
}

void Overflow::apply(Factory& f) const {
    f.setAllBreakdownProb(0.0f);
    f.setProcessTicksByName("Oven", 12);  // 중간 병목 (기본 6 < 12 < Bottleneck 40)
    f.setSpawnInterval(3);                // 병목 배출량을 크게 초과하는 투입 → 초과분이 손실
    // 주: 손실은 '입력 - 병목 배출'로만 결정됨(실측). 벨트 길이는 손실에 영향 없어 조작하지 않음.
}

void GameMode::apply(Factory& f) const {
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
        case 4:  return std::make_unique<GameMode>();
        default: return std::make_unique<NormalFlow>();
    }
}

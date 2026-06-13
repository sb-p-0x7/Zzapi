#pragma once
// =============================================================================
// scenario.h — 시나리오 (다형성)
//
//   Scenario(추상) + apply(Factory&)
//    ├ NormalFlow       : 저부하 파이프라인. 고장/주문 없음. (기본)
//    ├ Bottleneck       : 투입↑ + 오븐 가공시간↑ → 앞단 백업(병목).
//    ├ RandomBreakdown  : 전 머신 고장확률↑.
//    ├ Overflow         : 중간 병목(Oven) + 폭주 투입 → 용량 초과분이 손실(낭비↑).
//    └ GameMode         : 게임 모드. 주문/경제 활성 + 약한 고장확률.
//
//   * 공정 시연 시나리오(위 4개)는 주문 OFF — 손실 지표가 순수 생산 손실만 의미.
//   * GameMode만 setOrdersEnabled(true) → 주문/돈 게임 레이어 활성.
//   * 새 시나리오 = subclass 하나 + 레지스트리 한 줄. (과제 드롭다운 요구 충족)
//   * Factory의 config API(setAllBreakdownProb 등)만 호출 → 캡슐화 유지.
// =============================================================================
#include <string>
#include <memory>

class Factory;   // 전방 선언 (순환 의존 회피)

class Scenario {
public:
    virtual ~Scenario() = default;
    virtual std::string name() const = 0;
    virtual void        apply(Factory& f) const = 0;
};

class NormalFlow : public Scenario {
public:
    std::string name() const override { return "Normal flow"; }
    void        apply(Factory& f) const override;
};

class RandomBreakdown : public Scenario {
public:
    std::string name() const override { return "Random breakdowns"; }
    void        apply(Factory& f) const override;
};

class Bottleneck : public Scenario {
public:
    std::string name() const override { return "Bottleneck"; }
    void        apply(Factory& f) const override;
};

class Overflow : public Scenario {
public:
    std::string name() const override { return "Overflow"; }
    void        apply(Factory& f) const override;
};

class GameMode : public Scenario {
public:
    std::string name() const override { return "Game Mode"; }
    void        apply(Factory& f) const override;
};

// ── 레지스트리: 드롭다운 인덱스 ↔ 시나리오 ──
int                       scenarioCount();
std::string               scenarioName(int idx);
std::unique_ptr<Scenario> makeScenario(int idx);

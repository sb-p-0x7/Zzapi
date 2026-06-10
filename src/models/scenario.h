#pragma once
// =============================================================================
// scenario.h — 시나리오 (다형성)
//
//   Scenario(추상) + apply(Factory&)
//    ├ FreePlay         : 자유 플레이(게임 모드, 기본). 약한 고장확률.
//    ├ NormalFlow       : 균형 잡힌 파이프라인. 고장 없음.
//    └ RandomBreakdown  : 고장확률↑.
//
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

class FreePlay : public Scenario {
public:
    std::string name() const override { return "Free Play"; }
    void        apply(Factory& f) const override;
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

// ── 레지스트리: 드롭다운 인덱스 ↔ 시나리오 ──
int                       scenarioCount();
std::string               scenarioName(int idx);
std::unique_ptr<Scenario> makeScenario(int idx);

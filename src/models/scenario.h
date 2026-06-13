#pragma once
// =============================================================================
// scenario.h — scenarios (polymorphism)
//
//   Scenario (abstract) + apply(Factory&)
//    + NormalFlow       : low-load pipeline. No breakdowns/orders. (default)
//    + Bottleneck       : higher input + longer oven process time -> upstream backup (bottleneck).
//    + RandomBreakdown  : higher breakdown probability on all machines.
//    + Overflow         : mid-pipeline bottleneck (Oven) + flooded input -> excess capacity is lost (more waste).
//    + GameMode         : game mode. Orders/economy active + a mild breakdown chance.
//
//   * The process-demo scenarios (the first 4) keep orders OFF — the loss metric reflects pure production loss only.
//   * Only GameMode calls setOrdersEnabled(true) -> activates the orders/money game layer.
//   * A new scenario = one subclass + one registry line. (satisfies the assignment's dropdown requirement)
//   * Calls only Factory's config API (setAllBreakdownProb, etc.) -> preserves encapsulation.
// =============================================================================
#include <string>
#include <memory>

class Factory;   // forward declaration (avoids circular dependency)

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

// -- Registry: dropdown index <-> scenario --
int                       scenarioCount();
std::string               scenarioName(int idx);
std::unique_ptr<Scenario> makeScenario(int idx);

#include "scenario.h"
#include "factory.h"
#include <random>

// =============================================================================
// Each scenario uses only Factory's config API (no direct machine access).
// =============================================================================
void NormalFlow::apply(Factory& f) const {
    f.setAllBreakdownProb(0.0f);
    f.setSpawnInterval(30);
}

void RandomBreakdown::apply(Factory& f) const {
    f.setAllBreakdownProb(0.001f); // per-tick breakdown chance for all machines (lowered 0.002 -> 0.001 to reduce simultaneous failures)
    f.setSpawnInterval(30);
}

void Bottleneck::apply(Factory& f) const {
    // Randomize the bottleneck location on each load — avoids the monotony of always slowing the oven.
    static const char* kCandidates[] = {
        "Sauce Spreader", "Cheese Spreader", "Topping Applier", "Oven", "Cutter"
    };
    static std::mt19937 rng(std::random_device{}());
    const char* slow = kCandidates[std::uniform_int_distribution<int>(0, 4)(rng)];

    f.setAllBreakdownProb(0.0f);
    f.setSpawnInterval(15);              // fast input (fills the line to make congestion visible)
    f.setProcessTicksByName(slow, 40);   // make one random stage very slow -> its upstream backs up (bottleneck)
}

void Overflow::apply(Factory& f) const {
    f.setAllBreakdownProb(0.0f);
    f.setProcessTicksByName("Oven", 12);  // mid-pipeline bottleneck (default 6 < 12 < Bottleneck's 40)
    f.setSpawnInterval(3);                // input far exceeding the bottleneck's output -> the excess is lost
    // Note: loss is determined solely by 'input - bottleneck output' (measured). Belt length doesn't affect loss, so it isn't touched.
}

void GameMode::apply(Factory& f) const {
    f.setOrdersEnabled(true);       // game mode — activate the orders/economy layer
    f.setAllBreakdownProb(0.0008f); // mild breakdown chance
    f.setSpawnInterval(30);
}

// =============================================================================
// Registry — to add a new scenario, just add one line here.
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

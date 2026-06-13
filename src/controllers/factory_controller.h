#pragma once
// =============================================================================
// FactoryController — maps cmd -> factory control methods + decides the tick cadence
//
//   * Knows only the backend (Factory). It knows neither ImGui nor the View.
//   * Reads the single-frame FactoryCmd and calls Factory's public control API.
//   * The time cadence (real-time dt -> logical ticks) is decided here. Factory knows only logical ticks.
//   * Never touches machine objects directly. Uses only Factory's public methods + snapshot.
// =============================================================================
#include "../bridge.h"

class Factory;

class FactoryController
{
public:
    void Init(Factory* factory);

    /// Map cmd (a single-frame command) to Factory control methods
    void applyCmd(const FactoryCmd& cmd);

    /// Take the elapsed real time (dt) and, if running, step() in proportion to speed
    void advance(float dt);

private:
    Factory* m_factory = nullptr;
    float    m_acc     = 0.0f;   // tick accumulator
    int      m_speed   = 1;      // 1..5 (updated by cmd)
};

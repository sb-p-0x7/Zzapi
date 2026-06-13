#pragma once
// =============================================================================
// factory.h — the factory (aggregate model + backend entry point)
//
//   Controller usage pattern (example):
//       factory.setScenario(cmd.scenario);  // only when it changed
//       if (cmd.start) factory.start(); ...  // map cmd -> control methods
//       factory.update();                    // if running, advance speed ticks
//       view.render(factory.snapshot());     // read-only snapshot
//
//   * The simulation loop (step) has no concrete machine type branching — all base pointers.
//   * The time cadence (frame <-> tick) is decided by the controller. Factory knows only logical ticks.
//   * No public data members.
// =============================================================================
#include "../bridge.h"
#include "machine.h"
#include "order.h"
#include <vector>
#include <string>

class Factory {
public:
    Factory()  { loadScenario(0); }
    ~Factory() { clear(); }

    // -- Control API called by the controller --
    void step();                  // advance exactly one tick
    void update();                // if running, advance speed ticks (call once per frame)
    void start()  { m_running = true;  log("Started"); }
    void pause()  { m_running = false; log("Paused"); }
    void reset()  { loadScenario(m_scenario); }
    void setSpeed(int s) { if (s >= 1 && s <= 5) m_speed = s; }
    void setScenario(int idx);    // load only when it changed
    void forceBreak(int idx);
    void repair(int idx);
    void clearLog() { m_log.clear(); }
    void tuneMachine(int idx, const MachineTune& t) {
        if (idx >= 0 && idx < (int)m_pipeline.size()) m_pipeline[idx]->tune(t);
    }

    // -- Config API used by Scenario --
    void setAllBreakdownProb(float p) { for (Machine* m : m_pipeline) m->setBreakdownProb(p); }
    void setSpawnInterval(int n)      { if (n > 0) m_spawnEvery = n; }
    // Find a machine by name (displayName) and set its process time — designate a bottleneck without index coupling.
    void setProcessTicksByName(const std::string& name, int ticks) {
        if (ticks <= 0) return;
        for (Machine* m : m_pipeline)
            if (m->displayName() == name) m->setProcessTicks(ticks);
    }
    void setBreakdownProbExceptFirst(float p) {
        for(int i=1; i<(int)m_pipeline.size(); ++i) {
            m_pipeline[i]->setBreakdownProb(p);
        }
    }

    void setOrdersEnabled(bool b) { m_ordersEnabled = b; }   // orders active only in game mode

    // -- Output read by the view --
    FactorySnap snapshot() const;
    bool isRunning() const { return m_running; }
    int  scenario()  const { return m_scenario; }

private:
    std::vector<Machine*> m_pipeline;   // composition (ownership)
    OrderBook m_orders;
    bool m_ordersEnabled = false;       // true only in game mode (orders) — set by the scenario
    long m_tick       = 0;
    bool m_running    = false;
    int  m_speed      = 1;
    int  m_scenario   = 0;
    int  m_spawnEvery = 30;
    int  m_nextId     = 1;

    int  m_money      = 0;
    int  m_finished   = 0;
    int  m_lost       = 0;
    int  m_breakdowns = 0;
    std::vector<bool>        m_wasBroken;
    std::vector<std::string> m_log;

    void loadScenario(int idx);
    void build();
    void clear();
    void transfer();
    void spawn();
    void detectBreakdowns();
    void log(const std::string& msg);
};

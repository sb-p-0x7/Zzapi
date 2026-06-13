#include "factory.h"
#include "scenario.h"
#include <cstdio>

// =============================================================================
// Construction / teardown
// =============================================================================
void Factory::clear() {
    for (Machine* m : m_pipeline) delete m;
    m_pipeline.clear();
}

void Factory::build() {
    clear();
    // Pipeline: place a conveyor belt between every pair of non-conveyor machines.
    //   Dough ->[belt]-> Sauce ->[belt]-> Cheese ->[belt]-> Topping ->[belt]-> Oven ->[belt]-> Cutter ->[belt]-> Packaging
    m_pipeline.push_back(new DoughStretcher());
    m_pipeline.push_back(new ConveyorBelt());
    m_pipeline.push_back(new SauceSpreader());
    m_pipeline.push_back(new ConveyorBelt());
    m_pipeline.push_back(new CheeseSpreader());
    m_pipeline.push_back(new ConveyorBelt());
    m_pipeline.push_back(new ToppingApplier());
    m_pipeline.push_back(new ConveyorBelt());
    m_pipeline.push_back(new Oven());
    m_pipeline.push_back(new ConveyorBelt());
    m_pipeline.push_back(new Cutter());
    m_pipeline.push_back(new ConveyorBelt());
    m_pipeline.push_back(new PackagingMachine());
    m_wasBroken.assign(m_pipeline.size(), false);
}

// =============================================================================
// Scenario (polymorphism)
// =============================================================================
void Factory::loadScenario(int idx) {
    m_scenario = idx;
    build();
    m_orders.reset();
    m_ordersEnabled = false;          // OFF by default — a game-mode scenario turns it on in apply()
    m_tick = 0; m_money = 0; m_finished = 0; m_lost = 0; m_breakdowns = 0;
    m_nextId = 1; m_running = false;
    m_log.clear();

    auto sc = makeScenario(idx);
    sc->apply(*this);                 // set machine parameters (via the config API)
    log(std::string("Scenario loaded: ") + sc->name());
}

void Factory::setScenario(int idx) {
    if (idx >= 0 && idx != m_scenario) loadScenario(idx);
}

// =============================================================================
// Control
// =============================================================================
void Factory::forceBreak(int idx) {
    if (idx < 0 || idx >= (int)m_pipeline.size()) return;
    m_pipeline[idx]->forceBreak();
    log(m_pipeline[idx]->displayName() + " force-broken");
}

void Factory::repair(int idx) {
    if (idx < 0 || idx >= (int)m_pipeline.size()) return;
    m_pipeline[idx]->instantRepair();
    log(m_pipeline[idx]->displayName() + " instant-repaired");
}

// =============================================================================
// Simulation step
// =============================================================================
void Factory::update() {
    if (!m_running) return;
    for (int s = 0; s < m_speed; ++s) step();
}

void Factory::step() {
    ++m_tick;
    // 1) Advance machines (polymorphism — no type branching)
    for (Machine* m : m_pipeline) m->update((int)m_tick);
    // 2) Transfer products (back -> front)
    transfer();
    // 3) Spawn new dough
    if (m_tick % m_spawnEvery == 0) spawn();
    // 4) Update orders (game mode only)
    if (m_ordersEnabled) m_orders.update((int)m_tick);
    // 5) Log breakdown events
    detectBreakdowns();
}

void Factory::spawn() {
    if (m_pipeline.empty()) return;
    if (m_pipeline.front()->canAccept())
        m_pipeline.front()->accept(new RawDough(m_nextId++));
}

void Factory::transfer() {
    const int N = (int)m_pipeline.size();
    for (int i = N - 1; i >= 0; --i) {
        if (!m_pipeline[i]->hasOutput()) continue;
        if (i == N - 1) {
            // Output of the last machine = finished good
            Pizza* p = m_pipeline[i]->takeOutput();
            ++m_finished;
            if (m_ordersEnabled) {
                int reward = m_orders.tryFulfill(*p);
                if (reward > 0) { m_money += reward; log(p->getInfo() + " shipped (+$" + std::to_string(reward) + ")"); }
                else            { log(p->getInfo() + " shipped (no order)"); }
            } else {
                log(p->getInfo() + " shipped");
            }
            delete p;
        } else if (m_pipeline[i + 1]->canAccept()) {
            m_pipeline[i + 1]->accept(m_pipeline[i]->takeOutput());
        } else {
            // If the next machine is full -> immediate loss (same approach as the reference)
            Pizza* p = m_pipeline[i]->takeOutput();
            ++m_lost;
            log(p->getInfo() + " lost (overflow)");
            delete p;
        }
    }
}

void Factory::detectBreakdowns() {
    for (size_t i = 0; i < m_pipeline.size(); ++i) {
        bool now = m_pipeline[i]->isBroken();
        if (now && !m_wasBroken[i]) {
            ++m_breakdowns;
            log(m_pipeline[i]->displayName() + " broke down");
        }
        m_wasBroken[i] = now;
    }
}

void Factory::log(const std::string& msg) {
    char buf[16];
    snprintf(buf, sizeof(buf), "[%05ld] ", m_tick);
    m_log.push_back(std::string(buf) + msg);
    if (m_log.size() > 200) m_log.erase(m_log.begin());
}

// =============================================================================
// Snapshot
// =============================================================================
FactorySnap Factory::snapshot() const {
    FactorySnap s;
    s.tick     = m_tick;
    s.running  = m_running;
    s.speed    = m_speed;
    s.scenario = m_scenario;
    s.spawnInterval = m_spawnEvery;
    for (int i = 0; i < scenarioCount(); ++i) s.scenarioNames.push_back(scenarioName(i));

    s.money           = m_money;
    s.finishedGoods   = m_finished;
    s.lostProducts    = m_lost + (m_ordersEnabled ? m_orders.failed() : 0);
    s.totalBreakdowns = m_breakdowns;
    s.ordersEnabled   = m_ordersEnabled;

    int wip = 0;
    s.machines.reserve(m_pipeline.size());
    for (size_t i = 0; i < m_pipeline.size(); ++i) {
        MachineSnap ms = m_pipeline[i]->snapshot();
        ms.id = (int)i;
        wip += m_pipeline[i]->wipCount();
        s.machines.push_back(std::move(ms));
    }
    s.wipCount = wip;

    m_orders.fillSnap(s.orders);
    s.eventLog = m_log;
    return s;
}

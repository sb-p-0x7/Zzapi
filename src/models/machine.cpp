#include "machine.h"

// =============================================================================
// Machine (공통)
// =============================================================================
std::mt19937& Machine::rng() {
    static std::mt19937 gen(std::random_device{}());
    return gen;
}

MachineState Machine::state() const {
    if (m_broken)       return MachineState::BROKEN;
    if (!m_powered)     return MachineState::OFF;
    if (wipCount() > 0) return MachineState::WORKING;
    return MachineState::IDLE;
}

void Machine::forceBreak() {
    if (!m_broken) { m_broken = true; m_repairTimer = m_repairTicks; }
}

void Machine::instantRepair() {
    m_broken = false; m_repairTimer = 0; m_durability = m_maxDurability;
}

void Machine::resetState() {
    m_broken = false; m_powered = true; m_repairTimer = 0;
    m_durability = m_maxDurability;
}

void Machine::wear(float amount) {
    m_durability -= amount;
    if (m_durability < 0) m_durability = 0;
}

bool Machine::tickHealth() {
    if (!m_powered) return false;
    if (m_broken) {
        if (--m_repairTimer <= 0) { m_broken = false; m_durability = m_maxDurability; }
        return false;
    }
    if (m_breakdownProb > 0.f) {
        std::uniform_real_distribution<float> d(0.f, 1.f);
        if (d(rng()) < m_breakdownProb) {
            m_broken = true; m_repairTimer = m_repairTicks;
            return false;
        }
    }
    return true;
}

void Machine::fillCommonSnap(MachineSnap& s) const {
    s.name         = displayName();
    s.icon         = icon();
    s.state        = state();
    s.healthPct    = healthPct();
    s.processTicks = m_processTicks;
}

// =============================================================================
// NonConveyorMachine
// =============================================================================
void NonConveyorMachine::update(int /*tick*/) {
    if (!tickHealth()) return;          // 고장/정지면 진행 안 함
    if (!m_inside)     return;          // 가공할 게 없음
    if (++m_timer >= m_processTicks) {
        m_done   = transform(m_inside); // canAccept가 m_done 비었을 때만 받으므로 안전
        m_inside = nullptr;
        m_timer  = 0;
        wear(1.0f);
    }
}

Pizza* NonConveyorMachine::takeOutput() {
    Pizza* p = m_done; m_done = nullptr; return p;
}

int NonConveyorMachine::wipCount() const {
    return (m_inside ? 1 : 0) + (m_done ? 1 : 0);
}

MachineSnap NonConveyorMachine::snapshot() const {
    MachineSnap s;
    fillCommonSnap(s);
    s.isConveyor = false;
    Pizza* shown = m_inside ? m_inside : m_done;
    if (shown) { s.hasPizzaInside = true; s.pizzaInside = shown->toView(); }
    s.progressPct = (m_inside && m_processTicks > 0)
                  ? static_cast<float>(m_timer) / m_processTicks : 0.f;
    return s;
}

void NonConveyorMachine::resetState() {
    Machine::resetState();
    delete m_inside; delete m_done;
    m_inside = m_done = nullptr;
    m_timer = 0;
}

// =============================================================================
// ConveyorMachine
// =============================================================================
void ConveyorMachine::update(int /*tick*/) {
    if (!tickHealth()) return;
    m_moveProgress += m_moveSpeed;
    if (m_moveProgress >= 1.0f) {
        m_moveProgress -= 1.0f;
        // 뒤 → 앞 한 칸씩 전진. 맨 뒤 칸은 takeOutput으로 빠진다.
        for (int i = m_length - 1; i > 0; --i) {
            if (!m_belt[i]) { m_belt[i] = m_belt[i - 1]; m_belt[i - 1] = nullptr; }
        }
    }
}

Pizza* ConveyorMachine::takeOutput() {
    Pizza* p = m_belt.back(); m_belt.back() = nullptr; return p;
}

int ConveyorMachine::wipCount() const {
    int n = 0; for (auto* p : m_belt) if (p) ++n; return n;
}

MachineSnap ConveyorMachine::snapshot() const {
    MachineSnap s;
    fillCommonSnap(s);
    s.isConveyor = true;
    s.conveyor.moveProgress = m_moveProgress;
    s.conveyor.slots.reserve(m_length);
    for (auto* p : m_belt) {
        SlotView sv;
        if (p) { sv.occupied = true; sv.pizza = p->toView(); }
        s.conveyor.slots.push_back(sv);
    }
    return s;
}

void ConveyorMachine::resetState() {
    Machine::resetState();
    for (auto*& p : m_belt) { delete p; p = nullptr; }
    m_moveProgress = 0.f;
}

// =============================================================================
// concrete transform()
// =============================================================================
Pizza* DoughStretcher::transform(Pizza* p) {
    p->setSize(m_target);
    p->setDough(DoughStage::STRETCHED);
    return p;
}
Pizza* SauceSpreader::transform(Pizza* p)  { p->addSauce();   return p; }
Pizza* CheeseSpreader::transform(Pizza* p) { p->addCheese();  return p; }
Pizza* ToppingApplier::transform(Pizza* p) { p->addTopping(); return p; }
Pizza* Oven::transform(Pizza* p)           { p->setDough(DoughStage::BAKED); return p; }
Pizza* Cutter::transform(Pizza* p)         { p->setCut();     return p; }

Pizza* PackagingMachine::transform(Pizza* p) {
    BoxedPizza* boxed = new BoxedPizza(p->id());
    p->copyAttributesTo(*boxed);
    delete p;
    return boxed;
}

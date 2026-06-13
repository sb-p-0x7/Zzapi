#include "factory_controller.h"
#include "../models/factory.h"

// Logical ticks per second at 1x speed. Multiplied by speed (1..5) for the actual rate.
// A slow base rate is used so the time a pizza spends in a machine/belt is visible.
static constexpr float BASE_TPS = 4.0f;

void FactoryController::Init(Factory* factory)
{
    m_factory = factory;
}

void FactoryController::applyCmd(const FactoryCmd& cmd)
{
    if (!m_factory) return;

    // Scenario changes only when different (decided internally). -1 means no change.
    if (cmd.scenario >= 0) m_factory->setScenario(cmd.scenario);

    if (cmd.start) m_factory->start();
    if (cmd.pause) m_factory->pause();
    if (cmd.reset) { m_factory->reset(); m_acc = 0.0f; }

    m_factory->setSpeed(cmd.speed);
    m_speed = cmd.speed;
    if (cmd.spawnInterval > 0) m_factory->setSpawnInterval(cmd.spawnInterval);

    if (cmd.forceBreak)    m_factory->forceBreak(cmd.selectedMachine);
    if (cmd.instantRepair) m_factory->repair(cmd.selectedMachine);
    if (cmd.clearLog)      m_factory->clearLog();
    m_factory->tuneMachine(cmd.selectedMachine, cmd.tune);   // negative fields = no-op
}

void FactoryController::advance(float dt)
{
    if (!m_factory || !m_factory->isRunning()) return;

    m_acc += dt * BASE_TPS * static_cast<float>(m_speed);

    // Prevent runaway (excessive accumulation on frame hitches): cap progress per frame
    if (m_acc > 60.0f) m_acc = 60.0f;

    while (m_acc >= 1.0f) {
        m_factory->step();
        m_acc -= 1.0f;
    }
}

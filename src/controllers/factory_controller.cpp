#include "factory_controller.h"
#include "../models/factory.h"

// 1배속 기준 초당 논리 틱 수. speed(1..5)를 곱해 실제 진행 속도가 된다.
// 피자가 머신/벨트에 머무는 시간이 눈에 보이도록 느린 기본 속도를 쓴다.
static constexpr float BASE_TPS = 4.0f;

void FactoryController::Init(Factory* factory)
{
    m_factory = factory;
}

void FactoryController::applyCmd(const FactoryCmd& cmd)
{
    if (!m_factory) return;

    // 시나리오는 바뀌었을 때만(내부에서 판단). -1 이면 변경 없음.
    if (cmd.scenario >= 0) m_factory->setScenario(cmd.scenario);

    if (cmd.start) m_factory->start();
    if (cmd.pause) m_factory->pause();
    if (cmd.reset) { m_factory->reset(); m_acc = 0.0f; }

    m_factory->setSpeed(cmd.speed);
    m_speed = cmd.speed;

    if (cmd.forceBreak)    m_factory->forceBreak(cmd.selectedMachine);
    if (cmd.instantRepair) m_factory->repair(cmd.selectedMachine);
    if (cmd.clearLog)      m_factory->clearLog();
}

void FactoryController::advance(float dt)
{
    if (!m_factory || !m_factory->isRunning()) return;

    m_acc += dt * BASE_TPS * static_cast<float>(m_speed);

    // 폭주 방지(프레임 끊김 시 누적 과다): 한 프레임 최대 진행량 제한
    if (m_acc > 60.0f) m_acc = 60.0f;

    while (m_acc >= 1.0f) {
        m_factory->step();
        m_acc -= 1.0f;
    }
}

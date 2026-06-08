#include "factory.h"
#include "scenario.h"
#include <cstdio>

// =============================================================================
// 구성 / 해제
// =============================================================================
void Factory::clear() {
    for (Machine* m : m_pipeline) delete m;
    m_pipeline.clear();
}

void Factory::build() {
    clear();
    // 파이프라인: 도우 → [벨트] → 소스 → 치즈 → 토핑 → 오븐 → 커터 → [벨트] → 포장
    m_pipeline.push_back(new DoughStretcher());
    m_pipeline.push_back(new ConveyorBelt(4, 0.34f));
    m_pipeline.push_back(new SauceSpreader());
    m_pipeline.push_back(new CheeseSpreader());
    m_pipeline.push_back(new ToppingApplier());
    m_pipeline.push_back(new Oven());
    m_pipeline.push_back(new Cutter());
    m_pipeline.push_back(new ConveyorBelt(4, 0.34f));
    m_pipeline.push_back(new PackagingMachine());
    m_wasBroken.assign(m_pipeline.size(), false);
}

// =============================================================================
// 시나리오 (다형성)
// =============================================================================
void Factory::loadScenario(int idx) {
    m_scenario = idx;
    build();
    m_orders.reset();
    m_tick = 0; m_money = 0; m_finished = 0; m_lost = 0; m_breakdowns = 0;
    m_nextId = 1; m_running = false;
    m_log.clear();

    auto sc = makeScenario(idx);
    sc->apply(*this);                 // 머신 파라미터 세팅 (config API 경유)
    log(std::string("시나리오 로드: ") + sc->name());
}

void Factory::setScenario(int idx) {
    if (idx >= 0 && idx != m_scenario) loadScenario(idx);
}

// =============================================================================
// 제어
// =============================================================================
void Factory::forceBreak(int idx) {
    if (idx < 0 || idx >= (int)m_pipeline.size()) return;
    m_pipeline[idx]->forceBreak();
    log(m_pipeline[idx]->displayName() + " 강제 고장");
}

void Factory::repair(int idx) {
    if (idx < 0 || idx >= (int)m_pipeline.size()) return;
    m_pipeline[idx]->instantRepair();
    log(m_pipeline[idx]->displayName() + " 즉시 수리");
}

// =============================================================================
// 시뮬레이션 진행
// =============================================================================
void Factory::update() {
    if (!m_running) return;
    for (int s = 0; s < m_speed; ++s) step();
}

void Factory::step() {
    ++m_tick;
    // 1) 머신 진행 (다형성 — 타입 분기 없음)
    for (Machine* m : m_pipeline) m->update((int)m_tick);
    // 2) 제품 이송 (뒤 → 앞)
    transfer();
    // 3) 새 반죽 투입
    if (m_tick % m_spawnEvery == 0) spawn();
    // 4) 주문 갱신
    m_orders.update((int)m_tick);
    // 5) 고장 이벤트 로깅
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
            // 마지막 머신 배출 = 완성품
            Pizza* p = m_pipeline[i]->takeOutput();
            ++m_finished;
            int reward = m_orders.tryFulfill(*p);
            if (reward > 0) { m_money += reward; log(p->getInfo() + " 출고 (+$" + std::to_string(reward) + ")"); }
            else            { log(p->getInfo() + " 출고 (주문없음)"); }
            delete p;
        } else if (m_pipeline[i + 1]->canAccept()) {
            m_pipeline[i + 1]->accept(m_pipeline[i]->takeOutput());
        }
        // 다음 머신이 못 받으면 그대로 둠 → 자연스러운 병목/백업
    }
}

void Factory::detectBreakdowns() {
    for (size_t i = 0; i < m_pipeline.size(); ++i) {
        bool now = m_pipeline[i]->isBroken();
        if (now && !m_wasBroken[i]) {
            ++m_breakdowns;
            log(m_pipeline[i]->displayName() + " 고장 발생");
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
// 스냅샷
// =============================================================================
FactorySnap Factory::snapshot() const {
    FactorySnap s;
    s.tick     = m_tick;
    s.running  = m_running;
    s.speed    = m_speed;
    s.scenario = m_scenario;
    for (int i = 0; i < scenarioCount(); ++i) s.scenarioNames.push_back(scenarioName(i));

    s.money           = m_money;
    s.finishedGoods   = m_finished;
    s.lostProducts    = m_orders.failed();
    s.totalBreakdowns = m_breakdowns;

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

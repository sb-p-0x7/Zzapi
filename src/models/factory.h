#pragma once
// =============================================================================
// factory.h — 공장 (집합체 모델 + 백엔드 진입점)
//
//   동료 controller 사용 패턴 (예):
//       factory.setScenario(cmd.scenario);  // 바뀌었을 때만
//       if (cmd.start) factory.start(); ...  // cmd → 제어 메서드 매핑
//       factory.update();                    // running이면 speed틱 진행
//       view.render(factory.snapshot());     // 읽기 전용 스냅샷
//
//   * 시뮬 루프(step)에는 머신 concrete 타입 분기가 없다 — 전부 base 포인터.
//   * 시간 cadence(프레임↔틱)는 controller가 결정. Factory는 논리 틱만 안다.
//   * public 데이터 멤버 없음.
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

    // ── controller가 호출하는 제어 API ──
    void step();                  // 정확히 한 틱 진행
    void update();                // running이면 speed 틱 진행 (매 프레임 호출용)
    void start()  { m_running = true;  log("Started"); }
    void pause()  { m_running = false; log("Paused"); }
    void reset()  { loadScenario(m_scenario); }
    void setSpeed(int s) { if (s >= 1 && s <= 5) m_speed = s; }
    void setScenario(int idx);    // 바뀌었을 때만 로드
    void forceBreak(int idx);
    void repair(int idx);
    void clearLog() { m_log.clear(); }
    void tuneMachine(int idx, const MachineTune& t) {
        if (idx >= 0 && idx < (int)m_pipeline.size()) m_pipeline[idx]->tune(t);
    }

    // ── Scenario가 사용하는 config API ──
    void setAllBreakdownProb(float p) { for (Machine* m : m_pipeline) m->setBreakdownProb(p); }
    void setSpawnInterval(int n)      { if (n > 0) m_spawnEvery = n; }
    // 머신을 이름(displayName)으로 찾아 가공시간 설정 — 인덱스 결합 없이 병목 지정.
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

    void setAllConveyorLength(int len);

    // ── view가 읽는 출력 ──
    FactorySnap snapshot() const;
    bool isRunning() const { return m_running; }
    int  scenario()  const { return m_scenario; }

private:
    std::vector<Machine*> m_pipeline;   // composition (소유)
    OrderBook m_orders;
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

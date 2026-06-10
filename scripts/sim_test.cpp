// 모델 단독 검증용 콘솔 드라이버 (ImGui 없음).
//   g++ -std=c++17 scripts/sim_test.cpp src/models/*.cpp -Isrc -o /tmp/simtest
#include "../src/models/factory.h"
#include <cstdio>

int main() {
    Factory f;
    f.setScenario(2);    // Random breakdowns
    f.setSpeed(1);
    f.start();

    for (int frame = 0; frame < 1200; ++frame)
        f.update();      // 동료 controller가 매 프레임 부를 자리

    FactorySnap s = f.snapshot();
    printf("=== tick=%ld finished=%d wip=%d breakdowns=%d money=$%d failedOrders=%d ===\n",
           s.tick, s.finishedGoods, s.wipCount, s.totalBreakdowns, s.money, s.lostProducts);

    printf("--- machines ---\n");
    for (auto& m : s.machines) {
        const char* st = m.state == MachineState::WORKING ? "WORK"
                       : m.state == MachineState::BROKEN  ? "BREAK"
                       : m.state == MachineState::OFF     ? "OFF" : "IDLE";
        printf("  [%d] %-18s %-5s health=%.2f prog=%.2f%s\n",
               m.id, m.name.c_str(), st, m.healthPct, m.progressPct,
               m.isConveyor ? " (belt)" : (m.hasPizzaInside ? " [pizza]" : ""));
    }

    printf("--- scenarios --- ");
    for (auto& n : s.scenarioNames) printf("[%s] ", n.c_str());
    printf("\n--- active orders ---\n");
    for (auto& o : s.orders)
        printf("  %s  ticksLeft=%d  reward=$%d\n", o.desc.c_str(), o.ticksLeft, o.reward);

    printf("--- last events ---\n");
    int from = (int)s.eventLog.size() - 8; if (from < 0) from = 0;
    for (int i = from; i < (int)s.eventLog.size(); ++i)
        printf("  %s\n", s.eventLog[i].c_str());
    return 0;
}

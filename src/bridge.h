#pragma once
// =============================================================================
// bridge.h — UI ↔ 백엔드 경계 계약
//
//  * 이 파일은 UI(ui/)와 백엔드(sim/) 양쪽에서 include 한다.
//  * 메서드 없는 순수 데이터(POD)만 둔다. ImGui도 sim 타입도 여기서 모른다.
//  * 백엔드 → UI : FactorySnap (값 복사 스냅샷, UI는 항상 한 프레임 뒤)
//  * UI → 백엔드 : FactoryCmd  (한 프레임 동안만 true 인 명령)
// =============================================================================
#include <string>
#include <vector>

// ── 머신 상태 (색상 코딩용) ──
enum class MachineState { IDLE, WORKING, BROKEN, OFF };

// ── 피자 "겉모습"만 담은 값 구조체 (포인터 아님) ──
//    doughStage: 0=RAW, 1=STRETCHED, 2=BAKED
struct PizzaView {
    int  id        = -1;
    int  doughStage = 0;
    int  size      = 1;     // 0=S 1=M 2=L
    bool sauce      = false;
    bool cheese     = false;
    bool hasTopping = false;
    bool cut        = false;
    bool boxed      = false;
};

// ── 컨베이어 한 칸 ──
struct SlotView {
    bool      occupied = false;
    PizzaView pizza;
};

// ── 컨베이어 스냅샷 ──
struct ConveyorSnap {
    std::vector<SlotView> slots;
    float moveProgress = 0.0f;   // 0..1, 칸 사이 이동 진행도 → UI가 보간
};

// ── Inspector → 머신 설정 조절 (음수 = 변경 없음) ──
struct MachineTune {
    int   processTicks = -1;    // 비-벨트: 가공 시간 (틱)
    float healthPct    = -1.f;  // 0..1 내구도
    float breakProb    = -1.f;  // 0..1 틱당 고장 확률
    float beltSpeed    = -1.f;  // 벨트만: 틱당 진행량 (0..1)
};

// ── 머신 한 개 스냅샷 ──
struct MachineSnap {
    int          id          = -1;
    std::string  name;                       // UI는 dynamic_cast 안 함
    std::string  icon;
    MachineState state        = MachineState::IDLE;
    float        healthPct    = 1.0f;         // 0..1 → ProgressBar
    float        progressPct  = 0.0f;         // 0..1 → ProgressBar
    int          processTicks = 0;            // Inspector 표시용
    int          queueDepth   = 0;            // Inspector: 머신 안 대기물 수
    int          outputCount  = 0;            // Inspector: 누적 산출 개수
    float        breakProb    = 0.0f;          // Inspector 슬라이더 표시용
    float        beltSpeed    = 0.0f;          // 벨트일 때만 유효
    bool         hasPizzaInside = false;
    PizzaView    pizzaInside;
    bool         isConveyor   = false;
    ConveyorSnap conveyor;                    // isConveyor 일 때만 유효
};

// ── 주문 한 건 스냅샷 ──
struct OrderSnap {
    int         id        = -1;
    std::string desc;                 // 요구사항 요약
    int         ticksLeft = 0;
    int         reward    = 0;
};

// ── 공장 전체 스냅샷 ──
struct FactorySnap {
    long                     tick    = 0;
    bool                     running = false;
    int                      speed   = 1;
    int                      scenario = 0;
    int                      spawnInterval = 0;   // 현재 도우 투입 주기(틱)
    std::vector<std::string> scenarioNames;   // 드롭다운용
    std::vector<MachineSnap> machines;
    std::vector<OrderSnap>   orders;
    bool                     ordersEnabled = false;   // 게임모드(주문)일 때만 true
    std::vector<std::string> eventLog;        // 타임스탬프 포함 문자열
    // 통계
    int money           = 0;
    int finishedGoods   = 0;
    int wipCount        = 0;
    int totalBreakdowns = 0;
    int lostProducts    = 0;
};

// ── UI → 백엔드 명령 (한 프레임만 true) ──
struct FactoryCmd {
    bool start         = false;
    bool pause         = false;
    bool reset         = false;
    int  speed         = 1;     // 1..5
    int  scenario      = -1;    // -1 = 변경 없음
    int  selectedMachine = -1;
    bool forceBreak    = false;
    bool instantRepair = false;
    bool clearLog      = false;
    int  spawnInterval = -1;    // 도우 투입 주기(틱). -1 = 변경 없음
    MachineTune tune;           // selectedMachine 에 적용 (음수 필드 = 무시)
};

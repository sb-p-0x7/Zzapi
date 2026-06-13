#pragma once
// =============================================================================
// bridge.h — UI <-> backend boundary contract
//
//  * Included by both the UI (ui/) and the backend (sim/).
//  * Holds only plain data (POD) with no methods. It knows neither ImGui nor sim types.
//  * backend -> UI : FactorySnap (value-copied snapshot; the UI is always one frame behind)
//  * UI -> backend : FactoryCmd  (a command that is true for a single frame only)
// =============================================================================
#include <string>
#include <vector>

// -- Machine state (used for color coding) --
enum class MachineState { IDLE, WORKING, BROKEN, OFF };

// -- Value struct holding only a pizza's "appearance" (not a pointer) --
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

// -- One conveyor cell --
struct SlotView {
    bool      occupied = false;
    PizzaView pizza;
};

// -- Conveyor snapshot --
struct ConveyorSnap {
    std::vector<SlotView> slots;
    float moveProgress = 0.0f;   // 0..1, progress between cells -> UI interpolates
};

// -- Inspector -> machine setting adjustment (negative = no change) --
struct MachineTune {
    int   processTicks = -1;    // non-belt: process time (ticks)
    float healthPct    = -1.f;  // 0..1 durability
    float breakProb    = -1.f;  // 0..1 breakdown probability per tick
    float beltSpeed    = -1.f;  // belt only: progress per tick (0..1)
};

// -- Snapshot of a single machine --
struct MachineSnap {
    int          id          = -1;
    std::string  name;                       // UI does no dynamic_cast
    std::string  icon;
    MachineState state        = MachineState::IDLE;
    float        healthPct    = 1.0f;         // 0..1 -> ProgressBar
    float        progressPct  = 0.0f;         // 0..1 -> ProgressBar
    int          processTicks = 0;            // shown in Inspector
    int          queueDepth   = 0;            // Inspector: items waiting inside the machine
    int          outputCount  = 0;            // Inspector: cumulative output count
    float        breakProb    = 0.0f;          // shown on Inspector slider
    float        beltSpeed    = 0.0f;          // valid only when this is a belt
    bool         hasPizzaInside = false;
    PizzaView    pizzaInside;
    bool         isConveyor   = false;
    ConveyorSnap conveyor;                    // valid only when isConveyor
};

// -- Snapshot of a single order --
struct OrderSnap {
    int         id        = -1;
    std::string desc;                 // requirement summary
    int         ticksLeft = 0;
    int         reward    = 0;
};

// -- Snapshot of the whole factory --
struct FactorySnap {
    long                     tick    = 0;
    bool                     running = false;
    int                      speed   = 1;
    int                      scenario = 0;
    int                      spawnInterval = 0;   // current dough spawn interval (ticks)
    std::vector<std::string> scenarioNames;   // for the dropdown
    std::vector<MachineSnap> machines;
    std::vector<OrderSnap>   orders;
    bool                     ordersEnabled = false;   // true only in game mode (orders)
    std::vector<std::string> eventLog;        // strings including a timestamp
    // statistics
    int money           = 0;
    int finishedGoods   = 0;
    int wipCount        = 0;
    int totalBreakdowns = 0;
    int lostProducts    = 0;
};

// -- UI -> backend command (true for a single frame only) --
struct FactoryCmd {
    bool start         = false;
    bool pause         = false;
    bool reset         = false;
    int  speed         = 1;     // 1..5
    int  scenario      = -1;    // -1 = no change
    int  selectedMachine = -1;
    bool forceBreak    = false;
    bool instantRepair = false;
    bool clearLog      = false;
    int  spawnInterval = -1;    // dough spawn interval (ticks). -1 = no change
    MachineTune tune;           // applied to selectedMachine (negative fields = ignored)
};

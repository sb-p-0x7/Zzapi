#pragma once
// =============================================================================
// machine.h — machine hierarchy
//
//   Machine (abstract root)
//    + NonConveyorMachine (abstract)  -- processes one pizza at a time for processTicks
//    |   + DoughStretcher / SauceSpreader / CheeseSpreader / ToppingApplier
//    |       / Oven / Cutter / PackagingMachine
//    + ConveyorMachine (abstract)     -- carries pizzas over belt slots (no processing)
//        + ConveyorBelt
//
//   * The simulation loop deals only in base pointers. No if/else type branching.
//   * Each machine provides its own name/icon/snapshot (no UI dynamic_cast needed).
//   * A new machine = one subclass with transform() + a name/icon. Zero changes to loop/UI.
//   * No public data members.
// =============================================================================
#include "../bridge.h"
#include "pizza.h"
#include <string>
#include <vector>
#include <random>

class Machine {
protected:
    std::string m_name;
    int         m_processTicks;
    float       m_durability;
    float       m_maxDurability;
    bool        m_broken   = false;
    bool        m_powered  = true;
    float       m_breakdownProb = 0.0f;   // breakdown probability per tick (set by scenario)
    int         m_repairTicks   = 60;
    int         m_repairTimer   = 0;
    int         m_produced      = 0;     // cumulative output count (Inspector output count)

    static std::mt19937& rng();

    // Transform applied to the product when processing finishes. Default returns it unchanged.
    // Only the packaging machine swaps in a new BoxedPizza and returns it (deleting the original).
    virtual Pizza* transform(Pizza* p) { return p; }

    void fillCommonSnap(MachineSnap& s) const;

public:
    Machine(std::string name, int processTicks, float durability)
        : m_name(std::move(name)), m_processTicks(processTicks),
          m_durability(durability), m_maxDurability(durability) {}
    virtual ~Machine() = default;

    // -- Self-identification (removes UI branching) --
    virtual std::string displayName() const = 0;
    virtual std::string icon()        const = 0;
    virtual std::string getInfo()     const = 0;   // assignment-recommended interface

    // -- Simulation-loop interface (polymorphic) --
    virtual void   update(int tick) = 0;   // advance one tick
    virtual bool   canAccept() const = 0;
    virtual void   accept(Pizza* p)  = 0;
    virtual bool   hasOutput() const = 0;
    virtual Pizza* takeOutput()      = 0;
    virtual int    wipCount()  const = 0;
    virtual MachineSnap snapshot() const = 0;

    // -- Common state/control --
    MachineState state() const;
    bool  isBroken()  const { return m_broken; }
    bool  isPowered() const { return m_powered; }
    float healthPct() const { return m_maxDurability > 0 ? m_durability / m_maxDurability : 0.f; }
    void  setBreakdownProb(float p) { m_breakdownProb = p; }
    void  setProcessTicks(int t)    { m_processTicks = t; }
    void  forceBreak();
    void  instantRepair();
    virtual void resetState();
    // Inspector setting adjustment. Negative fields are ignored. Belts extend this to handle beltSpeed (polymorphism).
    virtual void tune(const MachineTune& t);

protected:
    bool tickHealth();              // breakdown roll + repair timer. Returns true if it can proceed
    void wear(float amount = 1.0f);
};

// =============================================================================
// NonConveyorMachine — stationary, one at a time
// =============================================================================
class NonConveyorMachine : public Machine {
protected:
    Pizza* m_inside = nullptr;   // pizza being processed
    Pizza* m_done   = nullptr;   // finished, waiting to be taken
    int    m_timer  = 0;

public:
    NonConveyorMachine(std::string name, int processTicks, float durability)
        : Machine(std::move(name), processTicks, durability) {}

    void   update(int tick) override;
    // Accepts only when the output slot is also empty -> backpressure forms naturally when blocked
    bool   canAccept() const override { return !m_broken && m_powered && !m_inside && !m_done; }
    void   accept(Pizza* p) override  { m_inside = p; m_timer = 0; }
    bool   hasOutput() const override { return m_done != nullptr; }
    Pizza* takeOutput() override;
    int    wipCount()  const override;
    MachineSnap snapshot() const override;
    void   resetState() override;
};

// =============================================================================
// ConveyorMachine — belt transport (no processing)
// =============================================================================
class ConveyorMachine : public Machine {
protected:
    int                 m_length;
    float               m_moveSpeed;       // progress per tick (accumulated 0..1)
    float               m_moveProgress = 0.f;
    std::vector<Pizza*> m_belt;            // nullptr = empty slot

public:
    ConveyorMachine(std::string name, float durability, int length, float moveSpeed)
        : Machine(std::move(name), 0, durability),
          m_length(length), m_moveSpeed(moveSpeed), m_belt(length, nullptr) {}

    void   update(int tick) override;
    bool   canAccept() const override { return !m_broken && m_powered && !m_belt.front(); }
    void   accept(Pizza* p) override  { m_belt.front() = p; }
    bool   hasOutput() const override { return m_belt.back() != nullptr; }
    Pizza* takeOutput() override;
    int    wipCount()  const override;
    MachineSnap snapshot() const override;
    void   resetState() override;
    void   tune(const MachineTune& t) override;   // + beltSpeed
};

// =============================================================================
// concrete machines
// =============================================================================
class DoughStretcher : public NonConveyorMachine {
    PizzaSize m_target;
protected:
    Pizza* transform(Pizza* p) override;
public:
    DoughStretcher(int t = 4, PizzaSize target = PizzaSize::MEDIUM)
        : NonConveyorMachine("DoughStretcher", t, 100.f), m_target(target) {}
    std::string displayName() const override { return "Dough Stretcher"; }
    std::string icon()        const override { return "DOUGH"; }
    std::string getInfo()     const override { return "Dough Stretcher"; }
};

class SauceSpreader : public NonConveyorMachine {
protected:
    Pizza* transform(Pizza* p) override;
public:
    SauceSpreader(int t = 3) : NonConveyorMachine("SauceSpreader", t, 100.f) {}
    std::string displayName() const override { return "Sauce Spreader"; }
    std::string icon()        const override { return "SAUCE"; }
    std::string getInfo()     const override { return "Sauce Spreader"; }
};

class CheeseSpreader : public NonConveyorMachine {
protected:
    Pizza* transform(Pizza* p) override;
public:
    CheeseSpreader(int t = 3) : NonConveyorMachine("CheeseSpreader", t, 100.f) {}
    std::string displayName() const override { return "Cheese Spreader"; }
    std::string icon()        const override { return "CHEESE"; }
    std::string getInfo()     const override { return "Cheese Spreader"; }
};

class ToppingApplier : public NonConveyorMachine {
protected:
    Pizza* transform(Pizza* p) override;
public:
    ToppingApplier(int t = 3) : NonConveyorMachine("ToppingApplier", t, 100.f) {}
    std::string displayName() const override { return "Topping Applier"; }
    std::string icon()        const override { return "TOPPING"; }
    std::string getInfo()     const override { return "Topping Applier"; }
};

class Oven : public NonConveyorMachine {
protected:
    Pizza* transform(Pizza* p) override;
public:
    Oven(int t = 6) : NonConveyorMachine("Oven", t, 100.f) {}
    std::string displayName() const override { return "Oven"; }
    std::string icon()        const override { return "OVEN"; }
    std::string getInfo()     const override { return "Oven"; }
};

class Cutter : public NonConveyorMachine {
protected:
    Pizza* transform(Pizza* p) override;
public:
    Cutter(int t = 3) : NonConveyorMachine("Cutter", t, 100.f) {}
    std::string displayName() const override { return "Cutter"; }
    std::string icon()        const override { return "CUT"; }
    std::string getInfo()     const override { return "Cutter"; }
};

class PackagingMachine : public NonConveyorMachine {
protected:
    Pizza* transform(Pizza* p) override;   // swap RawDough -> BoxedPizza
public:
    PackagingMachine(int t = 4) : NonConveyorMachine("PackagingMachine", t, 100.f) {}
    std::string displayName() const override { return "Packager"; }
    std::string icon()        const override { return "BOX"; }
    std::string getInfo()     const override { return "Packager"; }
};

class ConveyorBelt : public ConveyorMachine {
public:
    ConveyorBelt(int length = 3, float moveSpeed = 0.5f)
        : ConveyorMachine("ConveyorBelt", 100.f, length, moveSpeed) {}
    std::string displayName() const override { return "Conveyor"; }
    std::string icon()        const override { return "BELT"; }
    std::string getInfo()     const override { return "Conveyor belt"; }
};

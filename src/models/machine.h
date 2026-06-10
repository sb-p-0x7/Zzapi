#pragma once
// =============================================================================
// machine.h — 머신 계층
//
//   Machine(추상 루트)
//    ├ NonConveyorMachine(추상)  ── 한 번에 1개 피자를 processTicks 동안 가공
//    │   └ DoughStretcher / SauceSpreader / CheeseSpreader / ToppingApplier
//    │       / Oven / Cutter / PackagingMachine
//    └ ConveyorMachine(추상)     ── 벨트 슬롯 위로 피자 운반(가공 없음)
//        └ ConveyorBelt
//
//   * 시뮬 루프는 base 포인터로만 다룬다. if/else 타입 분기 없음.
//   * 이름/아이콘/스냅샷을 머신이 스스로 제공 (UI dynamic_cast 불필요).
//   * 새 머신 = transform()+이름/아이콘 가진 subclass 하나. 루프/UI 0줄 수정.
//   * public 데이터 멤버 없음.
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
    float       m_breakdownProb = 0.0f;   // 틱당 고장 확률 (시나리오가 설정)
    int         m_repairTicks   = 60;
    int         m_repairTimer   = 0;
    int         m_produced      = 0;     // 누적 산출 개수 (Inspector output count)

    static std::mt19937& rng();

    // 가공 완료 시 제품에 적용할 변형. 기본은 그대로 반환.
    // 포장 머신만 새 BoxedPizza로 교체해 반환(원본 delete).
    virtual Pizza* transform(Pizza* p) { return p; }

    void fillCommonSnap(MachineSnap& s) const;

public:
    Machine(std::string name, int processTicks, float durability)
        : m_name(std::move(name)), m_processTicks(processTicks),
          m_durability(durability), m_maxDurability(durability) {}
    virtual ~Machine() = default;

    // ── 자기 식별 (UI 분기 제거용) ──
    virtual std::string displayName() const = 0;
    virtual std::string icon()        const = 0;
    virtual std::string getInfo()     const = 0;   // 과제 권장 인터페이스

    // ── 시뮬 루프 인터페이스 (다형성) ──
    virtual void   update(int tick) = 0;   // 한 틱 진행
    virtual bool   canAccept() const = 0;
    virtual void   accept(Pizza* p)  = 0;
    virtual bool   hasOutput() const = 0;
    virtual Pizza* takeOutput()      = 0;
    virtual int    wipCount()  const = 0;
    virtual MachineSnap snapshot() const = 0;

    // ── 공통 상태/제어 ──
    MachineState state() const;
    bool  isBroken()  const { return m_broken; }
    bool  isPowered() const { return m_powered; }
    float healthPct() const { return m_maxDurability > 0 ? m_durability / m_maxDurability : 0.f; }
    void  setBreakdownProb(float p) { m_breakdownProb = p; }
    void  setProcessTicks(int t)    { m_processTicks = t; }
    void  forceBreak();
    void  instantRepair();
    virtual void resetState();

protected:
    bool tickHealth();              // 고장 굴림 + 수리 타이머. 진행 가능하면 true
    void wear(float amount = 1.0f);
};

// =============================================================================
// NonConveyorMachine — 고정형, 한 번에 1개
// =============================================================================
class NonConveyorMachine : public Machine {
protected:
    Pizza* m_inside = nullptr;   // 가공 중인 피자
    Pizza* m_done   = nullptr;   // 가공 끝나 배출 대기
    int    m_timer  = 0;

public:
    NonConveyorMachine(std::string name, int processTicks, float durability)
        : Machine(std::move(name), processTicks, durability) {}

    void   update(int tick) override;
    // 배출 대기물도 비어 있어야 새로 받음 → 막히면 자연 백업(backpressure)
    bool   canAccept() const override { return !m_broken && m_powered && !m_inside && !m_done; }
    void   accept(Pizza* p) override  { m_inside = p; m_timer = 0; }
    bool   hasOutput() const override { return m_done != nullptr; }
    Pizza* takeOutput() override;
    int    wipCount()  const override;
    MachineSnap snapshot() const override;
    void   resetState() override;
};

// =============================================================================
// ConveyorMachine — 벨트 운반 (가공 없음)
// =============================================================================
class ConveyorMachine : public Machine {
protected:
    int                 m_length;
    float               m_moveSpeed;       // 틱당 진행량 (0..1 누적)
    float               m_moveProgress = 0.f;
    std::vector<Pizza*> m_belt;            // nullptr = 빈 슬롯

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
};

// =============================================================================
// concrete 머신들
// =============================================================================
class DoughStretcher : public NonConveyorMachine {
    PizzaSize m_target;
protected:
    Pizza* transform(Pizza* p) override;
public:
    DoughStretcher(int t = 3, PizzaSize target = PizzaSize::MEDIUM)
        : NonConveyorMachine("DoughStretcher", t, 100.f), m_target(target) {}
    std::string displayName() const override { return "Dough Stretcher"; }
    std::string icon()        const override { return "DOUGH"; }
    std::string getInfo()     const override { return "Dough Stretcher"; }
};

class SauceSpreader : public NonConveyorMachine {
protected:
    Pizza* transform(Pizza* p) override;
public:
    SauceSpreader(int t = 2) : NonConveyorMachine("SauceSpreader", t, 100.f) {}
    std::string displayName() const override { return "Sauce Spreader"; }
    std::string icon()        const override { return "SAUCE"; }
    std::string getInfo()     const override { return "Sauce Spreader"; }
};

class CheeseSpreader : public NonConveyorMachine {
protected:
    Pizza* transform(Pizza* p) override;
public:
    CheeseSpreader(int t = 2) : NonConveyorMachine("CheeseSpreader", t, 100.f) {}
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
    Oven(int t = 8) : NonConveyorMachine("Oven", t, 100.f) {}
    std::string displayName() const override { return "Oven"; }
    std::string icon()        const override { return "OVEN"; }
    std::string getInfo()     const override { return "Oven"; }
};

class Cutter : public NonConveyorMachine {
protected:
    Pizza* transform(Pizza* p) override;
public:
    Cutter(int t = 2) : NonConveyorMachine("Cutter", t, 100.f) {}
    std::string displayName() const override { return "Cutter"; }
    std::string icon()        const override { return "CUT"; }
    std::string getInfo()     const override { return "Cutter"; }
};

class PackagingMachine : public NonConveyorMachine {
protected:
    Pizza* transform(Pizza* p) override;   // RawDough → BoxedPizza 교체
public:
    PackagingMachine(int t = 3) : NonConveyorMachine("PackagingMachine", t, 100.f) {}
    std::string displayName() const override { return "Packager"; }
    std::string icon()        const override { return "BOX"; }
    std::string getInfo()     const override { return "Packager"; }
};

class ConveyorBelt : public ConveyorMachine {
public:
    ConveyorBelt(int length = 4, float moveSpeed = 0.34f)
        : ConveyorMachine("ConveyorBelt", 100.f, length, moveSpeed) {}
    std::string displayName() const override { return "Conveyor"; }
    std::string icon()        const override { return "BELT"; }
    std::string getInfo()     const override { return "Conveyor belt"; }
};

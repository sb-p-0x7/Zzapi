#pragma once
// =============================================================================
// pizza.h — 제품 계층 (절충안)
//
//   Pizza(추상 베이스)            ← 과제: "추상 product 베이스"
//    ├ RawDough    (파이프라인 시작 concrete)
//    └ BoxedPizza  (파이프라인 끝   concrete)
//
//   흐르는 동안 RawDough가 소스·치즈·토핑 등 속성을 누적하고,
//   포장 단계에서 BoxedPizza로 교체된다.
// =============================================================================
#include "../bridge.h"   // PizzaView (값 구조체)
#include <string>

enum class DoughStage { RAW = 0, STRETCHED = 1, BAKED = 2 };
enum class PizzaSize  { SMALL = 0, MEDIUM = 1, LARGE = 2 };

class Pizza {
protected:
    int        m_id;
    DoughStage m_dough   = DoughStage::RAW;
    PizzaSize  m_size    = PizzaSize::MEDIUM;
    bool       m_sauce   = false;
    bool       m_cheese  = false;
    bool       m_topping = false;
    bool       m_cut     = false;
    bool       m_boxed   = false;

public:
    explicit Pizza(int id) : m_id(id) {}
    virtual ~Pizza() = default;

    // 과제 권장 인터페이스: 사람이 읽을 짧은 요약
    virtual std::string getInfo() const = 0;

    int        id()    const { return m_id; }
    DoughStage dough() const { return m_dough; }
    PizzaSize  size()  const { return m_size; }
    bool hasSauce()   const { return m_sauce; }
    bool hasCheese()  const { return m_cheese; }
    bool hasTopping() const { return m_topping; }
    bool isCut()      const { return m_cut; }
    bool isBoxed()    const { return m_boxed; }

    void setDough(DoughStage d) { m_dough = d; }
    void setSize(PizzaSize s)   { m_size = s; }
    void addSauce()             { m_sauce = true; }
    void addCheese()            { m_cheese = true; }
    void addTopping()           { m_topping = true; }
    void setCut()               { m_cut = true; }

    // 누적 속성을 다른 제품으로 복사 (포장 단계 교체용)
    void copyAttributesTo(Pizza& dst) const {
        dst.m_dough = m_dough; dst.m_size = m_size;
        dst.m_sauce = m_sauce; dst.m_cheese = m_cheese;
        dst.m_topping = m_topping; dst.m_cut = m_cut;
    }

    // 스냅샷용 겉모습 값 생성
    PizzaView toView() const {
        PizzaView v;
        v.id = m_id;
        v.doughStage = static_cast<int>(m_dough);
        v.size       = static_cast<int>(m_size);
        v.sauce = m_sauce; v.cheese = m_cheese;
        v.hasTopping = m_topping; v.cut = m_cut; v.boxed = m_boxed;
        return v;
    }
};

// ── 파이프라인 시작: 생 반죽 ──
class RawDough : public Pizza {
public:
    explicit RawDough(int id) : Pizza(id) {}
    std::string getInfo() const override;
};

// ── 파이프라인 끝: 포장 완료된 피자 ──
class BoxedPizza : public Pizza {
public:
    explicit BoxedPizza(int id) : Pizza(id) { m_boxed = true; }
    std::string getInfo() const override;
};

#pragma once
// =============================================================================
// pizza.h — product hierarchy (a pragmatic compromise)
//
//   Pizza (abstract base)          <- assignment: "abstract product base"
//    + RawDough    (concrete at the pipeline start)
//    + BoxedPizza  (concrete at the pipeline end)
//
//   As it flows, RawDough accumulates attributes (sauce, cheese, topping, ...),
//   and at the packaging stage it is replaced by a BoxedPizza.
// =============================================================================
#include "../bridge.h"   // PizzaView (value struct)
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

    // Assignment-recommended interface: a short human-readable summary
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

    // Copy accumulated attributes onto another product (for the packaging-stage swap)
    void copyAttributesTo(Pizza& dst) const {
        dst.m_dough = m_dough; dst.m_size = m_size;
        dst.m_sauce = m_sauce; dst.m_cheese = m_cheese;
        dst.m_topping = m_topping; dst.m_cut = m_cut;
    }

    // Build the appearance value for a snapshot
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

// -- Pipeline start: raw dough --
class RawDough : public Pizza {
public:
    explicit RawDough(int id) : Pizza(id) {}
    std::string getInfo() const override;
};

// -- Pipeline end: a packaged pizza --
class BoxedPizza : public Pizza {
public:
    explicit BoxedPizza(int id) : Pizza(id) { m_boxed = true; }
    std::string getInfo() const override;
};

#pragma once
// =============================================================================
// order.h — orders/economy (game layer)
//
//   Order      : one customer's requirements + deadline + reward + status
//   OrderBook  : manages active/completed/failed orders. Periodic generation, deadline handling, finished-good matching.
//
//   * For now only the model is implemented; Factory calls tryFulfill() when a finished good ships.
//   * Stage 4 (money -> upgrades) extends on top of the OrderBook reward.
// =============================================================================
#include "pizza.h"
#include "../bridge.h"
#include <vector>
#include <random>

enum class OrderStatus { PENDING, COMPLETED, FAILED };

class Order {
private:
    int        m_id;
    PizzaSize  m_size;
    bool       m_needSauce;
    bool       m_needCheese;
    bool       m_needTopping;
    bool       m_needCut;
    int        m_ticksLeft;
    int        m_reward;
    OrderStatus m_status = OrderStatus::PENDING;

public:
    Order(int id, PizzaSize size, bool sauce, bool cheese, bool topping,
          bool cut, int ticks, int reward)
        : m_id(id), m_size(size), m_needSauce(sauce), m_needCheese(cheese),
          m_needTopping(topping), m_needCut(cut), m_ticksLeft(ticks), m_reward(reward) {}

    int         id()        const { return m_id; }
    int         reward()    const { return m_reward; }
    int         ticksLeft() const { return m_ticksLeft; }
    OrderStatus status()    const { return m_status; }
    void        setStatus(OrderStatus s) { m_status = s; }

    bool tickExpire();                  // deadline countdown. Returns true (failed) when it hits 0
    bool matches(const Pizza& p) const; // does the finished good meet the requirements
    std::string desc() const;           // summary for snapshot/log
};

class OrderBook {
private:
    std::vector<Order> m_active;
    int m_nextId      = 1;
    int m_genEvery    = 120;    // a new order every N ticks
    int m_genTimer    = 0;
    int m_maxActive   = 5;
    int m_completed   = 0;
    int m_failed      = 0;

    static std::mt19937& rng();
    void generate();

public:
    void update(int tick);              // generate orders + handle deadlines
    int  tryFulfill(const Pizza& p);    // returns reward if matched, else 0
    void reset();

    int  completed() const { return m_completed; }
    int  failed()    const { return m_failed; }
    void fillSnap(std::vector<OrderSnap>& out) const;   // fill the bridge.h snapshot
};

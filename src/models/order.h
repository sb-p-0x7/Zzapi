#pragma once
// =============================================================================
// order.h — 주문/경제 (게임 레이어)
//
//   Order      : 손님 한 명의 요구사항 + 마감시간 + 보상 + 상태
//   OrderBook  : 활성/완료/실패 주문 관리. 주기적 생성, 마감 처리, 완성품 매칭.
//
//   * 지금은 모델만 완성해 두고, Factory가 완성품 출고 시 tryFulfill()을 호출.
//   * 4단계(돈→업그레이드)는 OrderBook 보상 위에 얹어 확장.
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

    bool tickExpire();                  // 마감 카운트다운. 0이 되면 true(실패)
    bool matches(const Pizza& p) const; // 완성품이 요구사항을 충족하나
    std::string desc() const;           // 스냅샷/로그용 요약
};

class OrderBook {
private:
    std::vector<Order> m_active;
    int m_nextId      = 1;
    int m_genEvery    = 120;    // N틱마다 새 주문
    int m_genTimer    = 0;
    int m_maxActive   = 5;
    int m_completed   = 0;
    int m_failed      = 0;

    static std::mt19937& rng();
    void generate();

public:
    void update(int tick);              // 주문 생성 + 마감 처리
    int  tryFulfill(const Pizza& p);    // 매칭되면 reward 반환, 없으면 0
    void reset();

    int  completed() const { return m_completed; }
    int  failed()    const { return m_failed; }
    void fillSnap(std::vector<OrderSnap>& out) const;   // bridge.h 스냅샷 채우기
};

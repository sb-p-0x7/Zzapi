#include "order.h"
#include <algorithm>

// =============================================================================
// Order
// =============================================================================
bool Order::tickExpire() {
    if (m_status != OrderStatus::PENDING) return false;
    if (--m_ticksLeft <= 0) { m_status = OrderStatus::FAILED; return true; }
    return false;
}

bool Order::matches(const Pizza& p) const {
    if (!p.isBoxed())          return false;     // finished goods only
    if (p.size() != m_size)    return false;
    if (m_needSauce   && !p.hasSauce())   return false;
    if (m_needCheese  && !p.hasCheese())  return false;
    if (m_needTopping && !p.hasTopping()) return false;
    if (m_needCut     && !p.isCut())      return false;
    return true;
}

std::string Order::desc() const {
    static const char* sz[] = {"S", "M", "L"};
    std::string s = "Order #" + std::to_string(m_id) + " [" + sz[(int)m_size] + "]";
    if (m_needSauce)   s += " sauce";
    if (m_needCheese)  s += " cheese";
    if (m_needTopping) s += " topping";
    if (m_needCut)     s += " cut";
    return s;
}

// =============================================================================
// OrderBook
// =============================================================================
std::mt19937& OrderBook::rng() {
    static std::mt19937 gen(std::random_device{}());
    return gen;
}

void OrderBook::generate() {
    std::bernoulli_distribution        coin(0.5);
    // The factory always produces Medium pizzas, so orders are kept Medium to stay fulfillable.
    // (Variable-size production is a future gameplay extension.)
    PizzaSize size  = PizzaSize::MEDIUM;
    bool sauce      = coin(rng());
    bool cheese     = coin(rng());
    bool topping    = coin(rng());
    bool cut        = coin(rng());

    int reward = 10;
    reward += (sauce ? 3 : 0) + (cheese ? 3 : 0) + (topping ? 5 : 0) + (cut ? 2 : 0);
    reward += static_cast<int>(size) * 4;

    std::uniform_int_distribution<int> timeD(300, 600);
    m_active.emplace_back(m_nextId++, size, sauce, cheese, topping, cut,
                          timeD(rng()), reward);
}

void OrderBook::update(int /*tick*/) {
    // Handle deadlines
    for (Order& o : m_active) {
        if (o.tickExpire()) ++m_failed;
    }
    // Remove completed/failed orders
    m_active.erase(
        std::remove_if(m_active.begin(), m_active.end(),
            [](const Order& o) { return o.status() != OrderStatus::PENDING; }),
        m_active.end());

    // Generate a new order
    if (++m_genTimer >= m_genEvery) {
        m_genTimer = 0;
        if ((int)m_active.size() < m_maxActive) generate();
    }
}

int OrderBook::tryFulfill(const Pizza& p) {
    for (Order& o : m_active) {
        if (o.status() == OrderStatus::PENDING && o.matches(p)) {
            o.setStatus(OrderStatus::COMPLETED);
            ++m_completed;
            return o.reward();
        }
    }
    return 0;   // no matching order
}

void OrderBook::reset() {
    m_active.clear();
    m_nextId = 1; m_genTimer = 0;
    m_completed = m_failed = 0;
}

void OrderBook::fillSnap(std::vector<OrderSnap>& out) const {
    out.clear();
    out.reserve(m_active.size());
    for (const Order& o : m_active) {
        OrderSnap s;
        s.id = o.id();
        s.desc = o.desc();
        s.ticksLeft = o.ticksLeft();
        s.reward = o.reward();
        out.push_back(std::move(s));
    }
}

#include "order.h"

Order::Order(int id, PizzaSize size, bool sauce, bool cheese, bool bake, int time, int reward)
    : id(id), requiredSize(size), requiresSauce(sauce), requiresCheese(cheese),
      requiresBake(bake), timeLeft(time), reward(reward), status(OrderStatus::PENDING) {}

int Order::getId() const { return id; }
PizzaSize Order::getRequiredSize() const { return requiredSize; }
bool Order::getRequiresSauce() const { return requiresSauce; }
bool Order::getRequiresCheese() const { return requiresCheese; }
bool Order::getRequiresBake() const { return requiresBake; }
int Order::getTimeLeft() const { return timeLeft; }
int Order::getReward() const { return reward; }
OrderStatus Order::getStatus() const { return status; }

void Order::setStatus(OrderStatus newStatus) { status = newStatus; }

bool Order::tick() {
    if (status != OrderStatus::PENDING) return true;
    
    if (timeLeft > 0) {
        timeLeft--;
        if (timeLeft == 0) {
            status = OrderStatus::FAILED;
            return false;
        }
    }
    return true;
}

bool Order::checkMatch(Pizza* pizza) const {
    if (!pizza) return false;
    
    if (pizza->getSize() != requiredSize) return false;
    if (pizza->getHasSauce() != requiresSauce) return false;
    if (pizza->getHasCheese() != requiresCheese) return false;
    if (pizza->getIsBaked() != requiresBake) return false;
    
    return true;
}

#include <cstdlib>

OrderManager::OrderManager() : nextOrderId(1), tickCount(0) {}

OrderManager::~OrderManager() {
    for (Order* o : activeOrders) delete o;
    for (Order* o : completedOrders) delete o;
    for (Order* o : failedOrders) delete o;
}

void OrderManager::tick() {
    tickCount++;
    
    // 예: 600 프레임(약 10초)마다 주문 1개씩 생성
    if (tickCount % 600 == 0) {
        generateRandomOrder();
    }
    
    // 활성 주문들 tick 처리
    for (auto it = activeOrders.begin(); it != activeOrders.end(); ) {
        Order* order = *it;
        if (!order->tick()) {
            // 주문 실패
            failedOrders.push_back(order);
            it = activeOrders.erase(it);
        } else {
            ++it;
        }
    }
}

bool OrderManager::verifyPizza(Pizza* pizza) {
    if (!pizza) return false;
    
    for (auto it = activeOrders.begin(); it != activeOrders.end(); ++it) {
        Order* order = *it;
        if (order->checkMatch(pizza)) {
            // 조건이 맞는 주문을 찾음
            order->setStatus(OrderStatus::COMPLETED);
            completedOrders.push_back(order);
            activeOrders.erase(it);
            return true;
        }
    }
    
    // 맞는 주문이 없음
    return false;
}

void OrderManager::generateRandomOrder() {
    // 랜덤으로 피자 스펙 결정
    int sizeRand = rand() % 3;
    PizzaSize size = PizzaSize::MEDIUM;
    if (sizeRand == 0) size = PizzaSize::SMALL;
    else if (sizeRand == 2) size = PizzaSize::LARGE;
    
    bool needsSauce = (rand() % 2) == 1;
    bool needsCheese = (rand() % 2) == 1;
    bool needsBake = (rand() % 2) == 1;
    
    int timeLimit = 1800 + (rand() % 1200); // 30~50초 정도의 제한시간
    int reward = 100; // 임시 기본 보상
    if (needsSauce) reward += 20;
    if (needsCheese) reward += 30;
    if (needsBake) reward += 50;
    if (size == PizzaSize::LARGE) reward += 40;
    
    Order* newOrder = new Order(nextOrderId++, size, needsSauce, needsCheese, needsBake, timeLimit, reward);
    activeOrders.push_back(newOrder);
}

const std::vector<Order*>& OrderManager::getActiveOrders() const { return activeOrders; }
const std::vector<Order*>& OrderManager::getCompletedOrders() const { return completedOrders; }
const std::vector<Order*>& OrderManager::getFailedOrders() const { return failedOrders; }

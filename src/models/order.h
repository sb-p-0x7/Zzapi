#pragma once

#include "pizza.h"

enum class OrderStatus {
    PENDING,
    COMPLETED,
    FAILED
};

class Order {
private:
    int id;
    PizzaSize requiredSize;
    bool requiresSauce;
    bool requiresCheese;
    bool requiresBake;
    
    int timeLeft; // 프레임 단위의 남은 시간
    int reward;
    OrderStatus status;

public:
    Order(int id, PizzaSize size, bool sauce, bool cheese, bool bake, int time, int reward);

    int getId() const;
    PizzaSize getRequiredSize() const;
    bool getRequiresSauce() const;
    bool getRequiresCheese() const;
    bool getRequiresBake() const;
    
    int getTimeLeft() const;
    int getReward() const;
    OrderStatus getStatus() const;

    void setStatus(OrderStatus newStatus);
    
    // 매 프레임 호출, timeLeft 감소. 시간이 0이 되면 false 반환(실패)
    bool tick();
    
    // 피자가 이 주문의 조건에 맞는지 확인
    bool checkMatch(Pizza* pizza) const;
};

class OrderManager {
private:
    std::vector<Order*> activeOrders;
    std::vector<Order*> completedOrders;
    std::vector<Order*> failedOrders;
    
    int nextOrderId;
    int tickCount;

public:
    OrderManager();
    ~OrderManager();

    // 매 프레임마다 호출: 새 주문 생성 및 남은 시간 처리
    void tick();

    // 특정 피자가 현재 주문들을 만족하는지 검사
    // 만족하면 true를 반환하고, 해당 주문을 completedOrders로 이동
    bool verifyPizza(Pizza* pizza);

    // Getters
    const std::vector<Order*>& getActiveOrders() const;
    const std::vector<Order*>& getCompletedOrders() const;
    const std::vector<Order*>& getFailedOrders() const;
    
private:
    void generateRandomOrder();
};

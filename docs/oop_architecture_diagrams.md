# Zzapi OOP Architecture Diagrams

> Purpose: explain the class hierarchy, object relationships, and runtime behavior of the pizza factory simulator.

---

## 1. Overall Architecture

```mermaid
---
config:
  layout: elk
---
flowchart TB
    %% === UI Layer / View ===
    subgraph UI["UI Layer / View"]
        class UI ui;
        DashboardView["DashboardView<br>renders FactorySnap<br>sets FactoryCmd"]
        BeltRender["belt namespace<br>DrawBelt / DrawBeltArc"]
    end

    %% === Boundary Contract ===
    subgraph Boundary["Boundary Contract: bridge.h"]
        class Boundary boundary;
        FactorySnap["FactorySnap<br>read-only value snapshot"]
        FactoryCmd["FactoryCmd<br>one-frame command flags"]
        DTOs["PizzaView / MachineSnap / OrderSnap<br>plain data structs"]
    end

    %% === Controller Layer ===
    subgraph Control["Controller Layer"]
        class Control control;
        FactoryController["FactoryController<br>maps cmd to Factory API<br>controls tick cadence"]
    end

    %% === Model Layer / Simulation Backend ===
    subgraph Model["Model Layer / Simulation Backend"]
        class Model model;
        Factory["Factory<br>owns pipeline and simulation state"]
        Machine["Machine hierarchy<br>polymorphic production units"]
        Pizza["Pizza hierarchy<br>products moving through pipeline"]
        Scenario["Scenario hierarchy<br>runtime configuration"]
        OrderBook["OrderBook<br>orders and rewards"]
    end

    %% === Connections ===
    App["App<br>frame coordinator"] --> DashboardView
    App --> FactoryController
    App --> Factory

    Factory --> FactorySnap
    Factory --> Machine
    Factory --> Pizza
    Factory --> Scenario
    Factory --> OrderBook

    Machine --> DTOs
    Pizza --> DTOs
    OrderBook --> DTOs
    DTOs --> FactorySnap

    FactorySnap --> DashboardView
    DashboardView --> FactoryCmd
    DashboardView --> BeltRender

    FactoryCmd --> FactoryController
    FactoryController --> Factory

    %% === Style Definitions ===
    classDef ui stroke:#818cf8,fill:#eef2ff;
    classDef boundary stroke:#a3e635,fill:#f7fee7;
    classDef control stroke:#fb923c,fill:#fff7ed;
    classDef model stroke:#2dd4bf,fill:#f0fdfa;
```

`App` coordinates the UI, controller, and factory once per frame. `DashboardView` never accesses real `Machine` or `Pizza` objects; it only renders a copied value snapshot called `FactorySnap`. User input is written into `FactoryCmd` as one-frame command flags, and `FactoryController` translates those flags into public `Factory` API calls. This keeps the UI and simulation backend decoupled through `bridge.h`.

**Key OOP point:** the model owns behavior and state, while the view receives only data-transfer objects. This is a clear separation of responsibilities.

---

## 2. Main Class Hierarchy

```mermaid
classDiagram
    class Pizza {
        <<abstract>>
        #int m_id
        #DoughStage m_dough
        #PizzaSize m_size
        #bool m_sauce
        #bool m_cheese
        #bool m_topping
        #bool m_cut
        #bool m_boxed
        +getInfo() string
        +toView() PizzaView
        +copyAttributesTo(Pizza& dst) void
    }

    class RawDough {
        +getInfo() string
    }

    class BoxedPizza {
        +getInfo() string
    }

    Pizza <|-- RawDough
    Pizza <|-- BoxedPizza
```
```mermaid
classDiagram
    class Machine {
        <<abstract>>
        #string m_name
        #int m_processTicks
        #float m_durability
        #bool m_broken
        #bool m_powered
        +displayName() string
        +icon() string
        +getInfo() string
        +update(int tick) void
        +canAccept() bool
        +accept(Pizza* p) void
        +hasOutput() bool
        +takeOutput() Pizza*
        +snapshot() MachineSnap
        #transform(Pizza* p) Pizza*
    }

    class NonConveyorMachine {
        <<abstract>>
        #Pizza* m_inside
        #Pizza* m_done
        #int m_timer
        +update(int tick) void
        +snapshot() MachineSnap
    }

    class ConveyorMachine {
        <<abstract>>
        #int m_length
        #float m_moveSpeed
        #vector~Pizza*~ m_belt
        +update(int tick) void
        +snapshot() MachineSnap
    }

    Machine <|-- NonConveyorMachine
    Machine <|-- ConveyorMachine

    NonConveyorMachine <|-- DoughStretcher
    NonConveyorMachine <|-- SauceSpreader
    NonConveyorMachine <|-- CheeseSpreader
    NonConveyorMachine <|-- ToppingApplier
    NonConveyorMachine <|-- Oven
    NonConveyorMachine <|-- Cutter
    NonConveyorMachine <|-- PackagingMachine
    ConveyorMachine <|-- ConveyorBelt
```
```mermaid
classDiagram
    class Scenario {
        <<abstract>>
        +name() string
        +apply(Factory& f) void
    }

    Scenario <|-- NormalFlow
    Scenario <|-- Bottleneck
    Scenario <|-- RandomBreakdown
    Scenario <|-- Overflow
    Scenario <|-- GameMode
```

The project has three important inheritance structures. First, `Pizza` is an abstract product, specialized into the starting product `RawDough` and the final product `BoxedPizza`. Second, `Machine` is an abstract production unit, split into `NonConveyorMachine`, which processes one pizza at a time, and `ConveyorMachine`, which carries pizzas through belt slots. Third, `Scenario` is an abstract configuration strategy; each concrete scenario changes the factory by calling the public configuration API of `Factory`.

**Key OOP point:** the simulation loop depends on abstract base classes, not concrete classes. This demonstrates abstraction and polymorphism.

---

## 3. Object Relationship Diagram

```mermaid
erDiagram
    FACTORY ||--|{ MACHINE : "owns pipeline"
    FACTORY ||--|| ORDERBOOK : "owns"
    ORDERBOOK ||--o{ ORDER : "manages"
    FACTORY }o--|| SCENARIO : "configured by"
    MACHINE ||--o| PIZZA : "processes one"
    MACHINE ||--o{ PIZZA : "carries many on belt"
    ORDER }o--o| PIZZA : "fulfilled by matching"

    FACTORY {
        long tick
        bool running
        int speed
        int scenario
        int spawnEvery
        int money
        int finished
        int lost
        int breakdowns
    }

    MACHINE {
        string name
        int processTicks
        float durability
        bool broken
        bool powered
        float breakdownProb
    }

    PIZZA {
        int id
        DoughStage dough
        PizzaSize size
        bool sauce
        bool cheese
        bool topping
        bool cut
        bool boxed
    }

    ORDERBOOK {
        int nextId
        int genEvery
        int maxActive
        int completed
        int failed
    }

    ORDER {
        int id
        PizzaSize size
        int ticksLeft
        int reward
        OrderStatus status
    }

    SCENARIO {
        string name
    }
```

`Factory` is the aggregate root of the simulation. It owns a pipeline of many `Machine` objects and, in game mode, also owns an `OrderBook`. A `Machine` either processes a pizza directly or carries pizzas in conveyor slots before passing them to the next stage. `OrderBook` manages many `Order` objects and calculates rewards when a finished `Pizza` matches an active order.

**Key OOP point:** ownership is centralized in `Factory`, while each object still keeps its own responsibilities. This supports encapsulation and keeps object lifetimes understandable.

---

## 4. Runtime Frame Flow

```mermaid
sequenceDiagram
    participant Main as main.cpp
    participant App
    participant Factory
    participant View as DashboardView
    participant Cmd as FactoryCmd
    participant Controller as FactoryController

    Main->>App: Update()
    App->>Factory: snapshot()
    Factory-->>App: FactorySnap
    App->>View: Render(snap, cmd)
    View->>Cmd: set flags from UI input
    App->>Controller: applyCmd(cmd)
    Controller->>Factory: start / pause / reset / tune / repair
    App->>Cmd: clear command
    App->>Controller: advance(deltaTime)
    Controller->>Factory: step() zero or more times
```

Every frame runs through `App::Update()`. First, `Factory` creates a read-only `FactorySnap`, and `DashboardView` renders that snapshot. If the user clicks buttons or changes sliders, the view does not mutate the model directly; it only writes command flags into `FactoryCmd`. Then `FactoryController` reads the command and calls the public API of `Factory`. Finally, based on accumulated real time, `Factory::step()` may be executed zero or more times.

**Key OOP point:** the view is passive with respect to the model. User intent is represented as a command object and applied by the controller.

---

## 5. Simulation Tick Flow

```mermaid
flowchart TD
    Start["Factory::step()"] --> Tick["increase m_tick"]
    Tick --> UpdateMachines["for each Machine*:<br/>m->update(tick)"]
    UpdateMachines --> Transfer["Factory::transfer()<br/>move output to next machine"]
    Transfer --> Last{"last machine output?"}
    Last -->|yes| Ship["ship finished pizza<br/>try order fulfillment<br/>delete pizza"]
    Last -->|no| NextReady{"next machine canAccept()?"}
    NextReady -->|yes| Move["next.accept(current.takeOutput())"]
    NextReady -->|no| Lost["mark product as lost<br/>delete pizza"]
    Ship --> SpawnCheck{"tick % spawnEvery == 0?"}
    Move --> SpawnCheck
    Lost --> SpawnCheck
    SpawnCheck -->|yes| Spawn["spawn RawDough<br/>into first machine"]
    SpawnCheck -->|no| Orders
    Spawn --> Orders{"orders enabled?"}
    Orders -->|yes| UpdateOrders["OrderBook::update(tick)"]
    Orders -->|no| Breakdowns
    UpdateOrders --> Breakdowns["detectBreakdowns()<br/>append event log"]
    Breakdowns --> End["end of one logical tick"]
```

`Factory::step()` represents one logical simulation tick. It first calls `update()` on every machine. The important part is that `Factory` does not inspect the concrete machine type. All machines are stored as `Machine*`, and each concrete class performs its own version of `update()`. Then the factory transfers outputs from back to front. If the next machine cannot accept an item, the product is counted as lost. When a pizza leaves the final machine, it is shipped, and in game mode the order reward is also calculated.

**Key OOP point:** polymorphism removes type-specific branching from the simulation loop.

---

## 6. Machine Polymorphism

```mermaid
flowchart LR
    FactoryStep["Factory::step()"] --> Loop["for Machine* m in pipeline"]
    Loop --> Update["m->update(tick)"]

    Update --> NonConv["NonConveyorMachine::update()<br/>process one pizza<br/>timer reaches processTicks"]
    Update --> Conv["ConveyorMachine::update()<br/>move pizzas through slots<br/>using moveSpeed"]

    NonConv --> Transform["transform(Pizza*)"]
    Transform --> Dough["DoughStretcher:<br/>set STRETCHED + size"]
    Transform --> Sauce["SauceSpreader:<br/>add sauce"]
    Transform --> Cheese["CheeseSpreader:<br/>add cheese"]
    Transform --> Topping["ToppingApplier:<br/>add topping"]
    Transform --> Oven["Oven:<br/>set BAKED"]
    Transform --> Cutter["Cutter:<br/>set cut"]
    Transform --> Packager["PackagingMachine:<br/>replace RawDough with BoxedPizza"]

    Conv --> ConveyorBelt["ConveyorBelt:<br/>concrete belt machine"]
```

All machines share the `Machine` interface, but their internal behavior is different. `NonConveyorMachine` stores one pizza internally and calls `transform()` when its processing timer finishes. `ConveyorMachine`, on the other hand, moves pizzas through multiple belt slots. `Factory` does not need to know this difference; it only uses common methods such as `update()`, `canAccept()`, and `takeOutput()`.

**Key OOP point:** new machine behavior can be added by subclassing, while the factory loop stays stable.

---

## 7. Boundary Data Objects

```mermaid
classDiagram
    class FactorySnap {
        +long tick
        +bool running
        +int speed
        +int scenario
        +vector~MachineSnap~ machines
        +vector~OrderSnap~ orders
        +vector~string~ eventLog
        +int money
        +int finishedGoods
        +int wipCount
        +int totalBreakdowns
        +int lostProducts
    }

    class MachineSnap {
        +int id
        +string name
        +string icon
        +MachineState state
        +float healthPct
        +float progressPct
        +bool isConveyor
        +ConveyorSnap conveyor
        +PizzaView pizzaInside
    }

    class PizzaView {
        +int id
        +int doughStage
        +int size
        +bool sauce
        +bool cheese
        +bool hasTopping
        +bool cut
        +bool boxed
    }

    class FactoryCmd {
        +bool start
        +bool pause
        +bool reset
        +int speed
        +int scenario
        +int selectedMachine
        +bool forceBreak
        +bool instantRepair
        +bool clearLog
        +MachineTune tune
    }

    FactorySnap *-- MachineSnap
    FactorySnap *-- OrderSnap
    MachineSnap *-- PizzaView
    MachineSnap *-- ConveyorSnap
    FactoryCmd *-- MachineTune
```

The structs in `bridge.h` are plain data objects without behavior. `FactorySnap` is a read-only copy sent from the backend to the UI, while `FactoryCmd` is a command object sent from the UI to the backend. Since the UI only uses these values, it does not depend on the internal implementation of `Machine`, `Pizza`, or `Factory`.

**Key OOP point:** this is an explicit boundary between layers, reducing coupling and protecting encapsulation.

---

## 8. Summary

This project is a C++ simulator that models a pizza factory using object-oriented design. `Factory` is the central simulation object that manages the machine pipeline, orders, statistics, and event logs. Each machine inherits from the abstract `Machine` class, and its behavior is handled polymorphically. Therefore, `Factory::step()` does not check concrete machine types; it only calls the common interface through `Machine*`. The UI does not directly manipulate model objects. Instead, it communicates with the backend only through `FactorySnap` and `FactoryCmd`. This design demonstrates abstraction, inheritance, polymorphism, encapsulation, and layer separation.

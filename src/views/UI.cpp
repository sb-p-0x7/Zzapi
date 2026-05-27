#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl3.h"

#include <SDL.h>
#include <SDL_opengl.h>

#include <string>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <ctime>

// ─────────────────────────────────────────────
//  Simulation state (fake backend)
// ─────────────────────────────────────────────

enum class MachineState { Working, Broken, Maintenance, Idle };

struct MachineData {
    std::string name;
    MachineState state;
    float health;        // 0..1
    float progress;      // 0..1  (current pizza processing)
    int   queue;
    int   output;
    float processTime;   // ticks per pizza

    // animation
    float anim;          // 0..1 belt position
};

static const char* StateStr(MachineState s) {
    switch(s) {
        case MachineState::Working:     return "WORKING";
        case MachineState::Broken:      return "BROKEN!";
        case MachineState::Maintenance: return "MAINTENANCE";
        case MachineState::Idle:        return "IDLE";
    }
    return "";
}

static ImVec4 StateColor(MachineState s) {
    switch(s) {
        case MachineState::Working:     return {0.2f,0.9f,0.3f,1.f};
        case MachineState::Broken:      return {0.95f,0.2f,0.2f,1.f};
        case MachineState::Maintenance: return {0.95f,0.75f,0.1f,1.f};
        case MachineState::Idle:        return {0.6f,0.6f,0.6f,1.f};
    }
    return {1,1,1,1};
}

// ─────────────────────────────────────────────
//  Global sim vars
// ─────────────────────────────────────────────
static bool  g_running       = false;
static float g_speed         = 1.0f;       // 0.1 .. 5.0
static int   g_scenario      = 0;
static const char* g_scenarios[] = { "Normal", "High Demand", "Frequent Breakdown", "All Broken" };

static int   g_pizzaCount    = 0;
static int   g_waiting       = 0;
static int   g_lost          = 0;
static int   g_breakdowns    = 0;
static float g_ticks         = 0.f;

static int   g_inspectorIdx  = 0;

static std::vector<MachineData> g_machines = {
    {"DoughStretcher",   MachineState::Working,     0.92f, 0.0f, 0, 0, 4.0f, 0.f},
    {"SauceSpreader",    MachineState::Broken,       0.00f, 0.0f, 2, 0, 3.5f, 0.f},
    {"CheeseSpreader",   MachineState::Maintenance,  0.45f, 0.0f, 1, 0, 3.0f, 0.f},
    {"Oven",             MachineState::Working,      0.78f, 0.0f, 0, 0, 8.0f, 0.f},
    {"Cutter",           MachineState::Working,      0.85f, 0.0f, 0, 0, 2.0f, 0.f},
    {"PackagingMachine", MachineState::Working,      0.95f, 0.0f, 0, 0, 2.5f, 0.f},
};

// Simple pizza items on conveyor belt (for visual)
struct BeltItem {
    float x;   // 0..1 across full belt
    int   stage; // which machine segment
};
static std::vector<BeltItem> g_beltItems;

static float g_repairTimer[6] = {0,0,0,0,0,0};

// ─────────────────────────────────────────────
//  Tick / update
// ─────────────────────────────────────────────
static void SimTick(float dt) {
    if (!g_running) return;

    float spd = g_speed * dt;
    g_ticks += spd * 10.f;

    // Update each machine
    for (int i = 0; i < (int)g_machines.size(); i++) {
        auto& m = g_machines[i];

        if (m.state == MachineState::Broken) {
            g_repairTimer[i] += spd;
            // Auto repair after ~30 ticks (unless scenario = All Broken)
            if (g_repairTimer[i] > 30.f && g_scenario != 3) {
                m.state  = MachineState::Working;
                m.health = 0.6f;
                g_repairTimer[i] = 0;
            }
            continue;
        }
        if (m.state == MachineState::Maintenance) {
            m.health += spd * 0.02f;
            if (m.health >= 1.0f) {
                m.health = 1.0f;
                m.state  = MachineState::Working;
            }
            continue;
        }

        // Working / Idle
        if (m.queue > 0) {
            m.state = MachineState::Working;
            m.progress += spd / m.processTime;
            m.anim     += spd * 0.8f;
            if (m.anim > 1.f) m.anim -= 1.f;

            if (m.progress >= 1.0f) {
                m.progress = 0.f;
                m.output++;
                m.queue--;
                if (i == (int)g_machines.size()-1) {
                    g_pizzaCount++;
                    if (g_waiting > 0) g_waiting--;
                } else {
                    g_machines[i+1].queue++;
                }

                // Degrade health
                m.health -= (0.02f + (float)rand()/RAND_MAX * 0.03f);
                if (m.health < 0) m.health = 0;

                // Random breakdown
                float breakChance = (g_scenario == 2) ? 0.15f : 0.04f;
                if ((float)rand()/RAND_MAX < breakChance || m.health <= 0) {
                    m.state = MachineState::Broken;
                    m.health = 0;
                    m.progress = 0;
                    g_breakdowns++;
                    g_repairTimer[i] = 0;
                }
            }
        } else {
            m.state = MachineState::Idle;
        }
    }

    // Produce new dough balls
    static float doughTimer = 0;
    float rate = (g_scenario == 1) ? 0.8f : 1.6f;
    doughTimer += spd;
    if (doughTimer >= rate) {
        doughTimer = 0;
        g_waiting++;
        if (g_machines[0].state == MachineState::Working ||
            g_machines[0].state == MachineState::Idle) {
            g_machines[0].queue++;
        }
    }

    // Cap waiting / lost
    if (g_waiting > 20) { g_lost += g_waiting - 20; g_waiting = 20; }
}

// ─────────────────────────────────────────────
//  Draw helpers
// ─────────────────────────────────────────────

// Draw a health bar inside current window
static void DrawHealthBar(float health, float width) {
    ImVec2 pos = ImGui::GetCursorScreenPos();
    float h = 10.f;
    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddRectFilled(pos, {pos.x+width, pos.y+h}, IM_COL32(60,60,60,255), 3.f);
    ImVec4 col = (health > 0.6f) ? ImVec4(0.2f,0.85f,0.3f,1.f) :
                 (health > 0.3f) ? ImVec4(0.95f,0.75f,0.1f,1.f) :
                                   ImVec4(0.95f,0.2f,0.2f,1.f);
    dl->AddRectFilled(pos, {pos.x + width*health, pos.y+h},
        ImGui::ColorConvertFloat4ToU32(col), 3.f);
    dl->AddRect(pos, {pos.x+width, pos.y+h}, IM_COL32(120,120,120,200), 3.f);
    ImGui::Dummy({width, h});
}

// Draw progress bar
static void DrawProgressBar(float progress, float width) {
    ImVec2 pos = ImGui::GetCursorScreenPos();
    float h = 8.f;
    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddRectFilled(pos, {pos.x+width, pos.y+h}, IM_COL32(40,40,40,255), 2.f);
    dl->AddRectFilled(pos, {pos.x + width*progress, pos.y+h}, IM_COL32(80,180,255,220), 2.f);
    dl->AddRect(pos, {pos.x+width, pos.y+h}, IM_COL32(100,100,100,180), 2.f);
    ImGui::Dummy({width, h});
}

// Draw the conveyor belt simulation panel
static void DrawConveyorPanel(float panelW, float panelH) {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 base    = ImGui::GetCursorScreenPos();

    // Background
    dl->AddRectFilled(base, {base.x+panelW, base.y+panelH}, IM_COL32(22,24,30,255), 6.f);
    dl->AddRect      (base, {base.x+panelW, base.y+panelH}, IM_COL32(80,80,100,180), 6.f);

    int   N       = (int)g_machines.size();
    // Margins for left "dough" label and right ":) pizza!" label
    float marginL = 52.f;
    float marginR = 56.f;
    float usableW = panelW - marginL - marginR;
    float segW    = usableW / N;
    float beltY   = base.y + panelH * 0.60f;
    float beltH   = 16.f;

    // Draw conveyor belt track
    dl->AddRectFilled({base.x+marginL, beltY-1}, {base.x+marginL+usableW, beltY+beltH+1},
        IM_COL32(50,50,60,255));

    // Animated belt stripes
    static float beltOffset = 0;
    if (g_running) beltOffset += g_speed * 0.8f * ImGui::GetIO().DeltaTime;
    if (beltOffset > 20.f) beltOffset -= 20.f;
    for (float sx = base.x+marginL - 20.f + beltOffset; sx < base.x+marginL+usableW; sx += 20.f) {
        float x0 = (sx < base.x+marginL) ? base.x+marginL : sx;
        float x1 = (sx+14.f > base.x+marginL+usableW) ? base.x+marginL+usableW : sx+14.f;
        if (x1 > x0)
            dl->AddRectFilled({x0, beltY}, {x1, beltY+beltH}, IM_COL32(65,65,78,255));
    }

    // Machines: each segment gets a small inner gap so boxes never overflow
    float machGap  = segW * 0.05f;
    float machW    = segW - machGap * 2.f;
    float machH    = panelH * 0.52f;
    float machTop  = base.y + panelH * 0.04f;

    for (int i = 0; i < N; i++) {
        auto& m = g_machines[i];
        float cx = base.x + marginL + segW * i + segW * 0.5f;
        float mx = cx - machW * 0.5f;

        // Machine box
        ImU32 boxCol = (m.state == MachineState::Broken)      ? IM_COL32(90,20,20,230) :
                       (m.state == MachineState::Maintenance)  ? IM_COL32(80,70,10,230) :
                       (m.state == MachineState::Working)      ? IM_COL32(20,40,65,230) :
                                                                  IM_COL32(35,35,45,230);
        dl->AddRectFilled({mx, machTop}, {mx+machW, machTop+machH}, boxCol, 5.f);
        dl->AddRect      ({mx, machTop}, {mx+machW, machTop+machH},
            ImGui::ColorConvertFloat4ToU32(StateColor(m.state)), 5.f, 0, 1.5f);

        // Machine name (small)
        std::string shortName = m.name.substr(0, (m.name.size()>10?9:m.name.size()));
        ImVec2 tSz = ImGui::CalcTextSize(shortName.c_str());
        dl->AddText({cx - tSz.x*0.5f, machTop+4}, IM_COL32(200,200,220,255), shortName.c_str());

        // Pizza icon (animated spinning) inside machine
        float iconY = machTop + machH * 0.38f;
        if (m.state == MachineState::Working && m.queue > 0) {
            float r = machH * 0.18f;
            float rot = m.anim * 6.28f;
            ImVec2 center = {cx, iconY};
            dl->AddCircleFilled(center, r, IM_COL32(230,180,80,220));
            dl->AddCircle      (center, r, IM_COL32(180,100,40,255), 24, 1.5f);
            // Slices
            for (int s=0; s<6; s++) {
                float a = rot + s * 3.14159f/3.f;
                dl->AddLine(center,
                    {center.x + cosf(a)*r, center.y + sinf(a)*r},
                    IM_COL32(180,100,40,180), 1.f);
            }
        } else if (m.state == MachineState::Broken) {
            // X mark
            float r = machH * 0.14f;
            dl->AddLine({cx-r, iconY-r},{cx+r, iconY+r}, IM_COL32(255,60,60,220), 2.5f);
            dl->AddLine({cx+r, iconY-r},{cx-r, iconY+r}, IM_COL32(255,60,60,220), 2.5f);
        } else if (m.state == MachineState::Maintenance) {
            // Wrench icon (simplified)
            dl->AddCircle({cx, iconY}, machH*0.12f, IM_COL32(255,200,50,200), 12, 2.f);
        }

        // State label
        const char* sStr = StateStr(m.state);
        ImVec4 sc = StateColor(m.state);
        ImVec2 sSz = ImGui::CalcTextSize(sStr);
        dl->AddText({cx - sSz.x*0.5f, machTop+machH-20},
            ImGui::ColorConvertFloat4ToU32(sc), sStr);

        // Health mini-bar
        float bw = machW * 0.8f;
        float bx = cx - bw*0.5f;
        float by = machTop + machH - 10;
        dl->AddRectFilled({bx,by},{bx+bw,by+5}, IM_COL32(40,40,40,200), 2.f);
        ImVec4 hc = (m.health>0.6f)?ImVec4(0.2f,0.85f,0.3f,1.f):
                    (m.health>0.3f)?ImVec4(0.95f,0.75f,0.1f,1.f):
                                    ImVec4(0.95f,0.2f,0.2f,1.f);
        dl->AddRectFilled({bx,by},{bx+bw*m.health,by+5},
            ImGui::ColorConvertFloat4ToU32(hc), 2.f);

        // Queue count on belt
        float qx = cx + segW*0.25f;
        float qy = beltY + beltH*0.5f - 7;
        char qbuf[16]; snprintf(qbuf,sizeof(qbuf),"%d", m.queue);
        ImVec2 qSz = ImGui::CalcTextSize(qbuf);
        if (m.queue > 0) {
            dl->AddCircleFilled({qx, qy+7}, 10.f, IM_COL32(230,180,80,200));
            dl->AddText({qx-qSz.x*0.5f, qy}, IM_COL32(40,20,0,255), qbuf);
        }

        // Progress bar below machine (if working)
        if (m.state == MachineState::Working) {
            float pbw = machW;
            float pbx = cx - pbw*0.5f;
            float pby = machTop + machH + 4;
            dl->AddRectFilled({pbx,pby},{pbx+pbw,pby+4}, IM_COL32(30,30,40,200), 2.f);
            dl->AddRectFilled({pbx,pby},{pbx+pbw*m.progress,pby+4}, IM_COL32(80,160,255,220), 2.f);
        }

        // Separator line between segments
        if (i < N-1) {
            float sx2 = base.x + marginL + segW*(i+1);
            dl->AddLine({sx2, base.y+4},{sx2, base.y+panelH-4}, IM_COL32(60,60,80,120), 1.f);
        }

        // Arrow between segments
        if (i < N-1) {
            float ax = cx + segW*0.5f - 3;
            float ay = beltY + beltH*0.5f;
            dl->AddTriangleFilled({ax-5,ay-5},{ax+6,ay},{ax-5,ay+5}, IM_COL32(150,150,180,160));
        }
    }

    // Raw dough (input)
    {
        float ax = base.x + marginL - 14.f;
        float ay = beltY + beltH*0.5f;
        dl->AddTriangleFilled({ax-5,ay-5},{ax+6,ay},{ax-5,ay+5}, IM_COL32(150,150,180,160));
        dl->AddText({base.x+2, ay-16}, IM_COL32(180,160,120,200), "dough");
    }

    // Output (customer)
    {
        float ex = base.x + marginL + usableW + 10;
        float ey = beltY + beltH*0.5f;
        dl->AddText({ex, ey-18}, IM_COL32(150,220,150,220), ":)");
        dl->AddText({ex, ey+2}, IM_COL32(150,220,150,180), "pizza!");
    }

    ImGui::Dummy({panelW, panelH});
}

// ─────────────────────────────────────────────
//  Main
// ─────────────────────────────────────────────
int main(int argc, char* argv[]) {
    srand((unsigned)time(nullptr));

    SDL_Init(SDL_INIT_VIDEO);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

    SDL_Window* window = SDL_CreateWindow(
        "🍕 Pizza Factory Simulation",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        1400, 820,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE
    );
    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.FontGlobalScale = 1.1f;

    // Style
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding    = 8.f;
    style.FrameRounding     = 5.f;
    style.GrabRounding      = 4.f;
    style.ItemSpacing       = {8.f, 6.f};
    style.WindowPadding     = {12.f, 12.f};
    style.Colors[ImGuiCol_WindowBg]       = {0.10f,0.11f,0.14f,1.f};
    style.Colors[ImGuiCol_FrameBg]        = {0.15f,0.16f,0.20f,1.f};
    style.Colors[ImGuiCol_FrameBgHovered] = {0.20f,0.22f,0.28f,1.f};
    style.Colors[ImGuiCol_Button]         = {0.20f,0.35f,0.55f,1.f};
    style.Colors[ImGuiCol_ButtonHovered]  = {0.28f,0.48f,0.72f,1.f};
    style.Colors[ImGuiCol_ButtonActive]   = {0.35f,0.55f,0.80f,1.f};
    style.Colors[ImGuiCol_SliderGrab]     = {0.45f,0.65f,0.90f,1.f};
    style.Colors[ImGuiCol_Header]         = {0.22f,0.38f,0.60f,1.f};
    style.Colors[ImGuiCol_HeaderHovered]  = {0.28f,0.48f,0.72f,1.f};
    style.Colors[ImGuiCol_Tab]            = {0.15f,0.20f,0.30f,1.f};
    style.Colors[ImGuiCol_TabActive]      = {0.25f,0.42f,0.65f,1.f};
    style.Colors[ImGuiCol_TitleBgActive]  = {0.14f,0.22f,0.36f,1.f};

    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init("#version 130");

    bool running = true;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT) running = false;
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        float dt = io.DeltaTime;
        SimTick(dt);

        // ── Main window (fullscreen) ──────────────────────────────
        ImGui::SetNextWindowPos({0,0});
        ImGui::SetNextWindowSize(io.DisplaySize);
        ImGui::Begin("##root", nullptr,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus |
            ImGuiWindowFlags_NoScrollbar);

        // ── Toolbar ───────────────────────────────────────────────
        {
            // Start/Stop button
            if (g_running) {
                ImGui::PushStyleColor(ImGuiCol_Button,        {0.65f,0.15f,0.15f,1.f});
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, {0.80f,0.20f,0.20f,1.f});
                if (ImGui::Button(" Stop  ")) g_running = false;
            } else {
                ImGui::PushStyleColor(ImGuiCol_Button,        {0.15f,0.55f,0.25f,1.f});
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, {0.20f,0.70f,0.32f,1.f});
                if (ImGui::Button(" Start ")) g_running = true;
            }
            ImGui::PopStyleColor(2);

            ImGui::SameLine();

            // Reset
            ImGui::PushStyleColor(ImGuiCol_Button,        {0.30f,0.25f,0.10f,1.f});
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, {0.45f,0.38f,0.15f,1.f});
            if (ImGui::Button(" Reset ")) {
                g_running = false; g_pizzaCount = 0; g_waiting = 0;
                g_lost = 0; g_breakdowns = 0; g_ticks = 0;
                for (auto& m : g_machines) {
                    m.state = MachineState::Working; m.health = 0.9f;
                    m.progress = 0; m.queue = 0; m.output = 0; m.anim = 0;
                }
                for (auto& t : g_repairTimer) t = 0;
            }
            ImGui::PopStyleColor(2);

            ImGui::SameLine();
            ImGui::SetNextItemWidth(160);
            ImGui::SliderFloat("Speed", &g_speed, 0.1f, 5.0f, "%.1fx");

            ImGui::SameLine();
            ImGui::SetNextItemWidth(160);
            ImGui::Combo("Scenario", &g_scenario, g_scenarios, 4);
        }

        ImGui::Spacing();

        // ── Conveyor belt panel ───────────────────────────────────
        float availW = ImGui::GetContentRegionAvail().x;
        // Reserve space for: stats(90+spacing) + inspector label row(~24) + inspector child(~220) + spacing/padding
        float reservedH = 90.f + 24.f + 220.f + ImGui::GetStyle().ItemSpacing.y * 6 + 16.f;
        float conveyorH = ImGui::GetContentRegionAvail().y - reservedH;
        if (conveyorH < 120.f) conveyorH = 120.f;
        DrawConveyorPanel(availW, conveyorH);

        ImGui::Spacing();

        // ── Stats panel ───────────────────────────────────────────
        {
            ImGui::PushStyleColor(ImGuiCol_ChildBg, {0.12f,0.13f,0.17f,1.f});
            ImGui::BeginChild("##stats", {0, 90}, true);

            ImGui::Columns(2, "statcols", false);
            ImGui::SetColumnWidth(0, availW * 0.65f);

            // Left: counts
            ImGui::TextColored({0.4f,0.9f,0.5f,1.f}, "  ✔  %d customers got pizza.", g_pizzaCount);
            ImGui::TextColored({0.9f,0.8f,0.3f,1.f}, "  ⏳ %d customers are waiting.", g_waiting);
            ImGui::TextColored({0.9f,0.3f,0.3f,1.f}, "  ✘  %d customers were lost.", g_lost);
            ImGui::TextColored({0.7f,0.5f,1.0f,1.f}, "  ⚙  There were %d breakdown(s).", g_breakdowns);

            ImGui::NextColumn();

            // Right: time
            ImGui::TextColored({0.6f,0.7f,0.9f,1.f}, "time:  %.0f  ticks", g_ticks);
            char runStr[32];
            snprintf(runStr, sizeof(runStr), "%s", g_running ? "▶ RUNNING" : "⏹ STOPPED");
            ImGui::TextColored(g_running ? ImVec4{0.3f,1.f,0.4f,1.f} : ImVec4{0.6f,0.6f,0.6f,1.f},
                "%s", runStr);

            ImGui::Columns(1);
            ImGui::EndChild();
            ImGui::PopStyleColor();
        }

        ImGui::Spacing();

        // ── Inspector ─────────────────────────────────────────────
        ImGui::Text("Inspector:");
        ImGui::SameLine();
        for (int i = 0; i < (int)g_machines.size(); i++) {
            char label[32];
            snprintf(label, sizeof(label), " %s ##tab%d", g_machines[i].name.c_str(), i);
            bool active = (g_inspectorIdx == i);
            if (active) {
                ImGui::PushStyleColor(ImGuiCol_Button, {0.25f,0.42f,0.68f,1.f});
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, {0.30f,0.50f,0.78f,1.f});
            }
            if (ImGui::SmallButton(g_machines[i].name.c_str()))
                g_inspectorIdx = i;
            if (active) ImGui::PopStyleColor(2);
            if (i < (int)g_machines.size()-1) ImGui::SameLine();
        }

        // Inspector detail box
        {
            ImGui::PushStyleColor(ImGuiCol_ChildBg, {0.12f,0.13f,0.17f,1.f});
            ImGui::BeginChild("##inspector", {360, ImGui::GetContentRegionAvail().y}, true);

            auto& m = g_machines[g_inspectorIdx];

            ImGui::TextColored({0.7f,0.85f,1.0f,1.f}, "Machine:  %s", m.name.c_str());
            ImGui::Separator();

            ImGui::Text("State:");    ImGui::SameLine(120);
            ImGui::TextColored(StateColor(m.state), "%s", StateStr(m.state));

            ImGui::Text("Health:");   ImGui::SameLine(120);
            DrawHealthBar(m.health, 160);

            ImGui::Text("Progress:"); ImGui::SameLine(120);
            DrawProgressBar(m.progress, 160);

            ImGui::Text("Queue:");    ImGui::SameLine(120);
            ImGui::Text("%d  item(s)", m.queue);

            ImGui::Text("Output:");   ImGui::SameLine(120);
            ImGui::Text("%d  pizza(s)", m.output);

            ImGui::Text("Proc.Time:");ImGui::SameLine(120);
            ImGui::Text("%.1f  ticks/pizza", m.processTime);

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            // Force Break
            ImGui::PushStyleColor(ImGuiCol_Button,        {0.55f,0.12f,0.12f,1.f});
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, {0.75f,0.18f,0.18f,1.f});
            if (ImGui::Button("  Force Break  ")) {
                if (m.state != MachineState::Broken) {
                    m.state = MachineState::Broken;
                    m.health = 0; m.progress = 0;
                    g_breakdowns++;
                    g_repairTimer[g_inspectorIdx] = 0;
                }
            }
            ImGui::PopStyleColor(2);

            ImGui::SameLine();

            // Instant Repair
            ImGui::PushStyleColor(ImGuiCol_Button,        {0.12f,0.45f,0.20f,1.f});
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, {0.18f,0.60f,0.28f,1.f});
            if (ImGui::Button(" Instant Repair ")) {
                m.state = MachineState::Working;
                m.health = 1.0f;
                m.progress = 0;
                g_repairTimer[g_inspectorIdx] = 0;
            }
            ImGui::PopStyleColor(2);

            ImGui::EndChild();
            ImGui::PopStyleColor();
        }

        ImGui::End();

        // Render
        ImGui::Render();
        int w, h;
        SDL_GetWindowSize(window, &w, &h);
        glViewport(0, 0, w, h);
        glClearColor(0.08f, 0.09f, 0.11f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
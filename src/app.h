#pragma once

// =============================================================================
// Application class
// Top-level app class that manages the MVC components.
// Called from the ImGui loop in main.cpp.
// =============================================================================

class App
{
public:
    /// Initialize the app (create the model, view, and controller)
    void Init();

    /// Called every frame (between ImGui::NewFrame and ImGui::Render)
    void Update();

    /// Cleanup on app shutdown
    void Shutdown();
};

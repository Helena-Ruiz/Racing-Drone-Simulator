#include "ImGuiManager.h"

ImGuiManager::ImGuiManager(GLFWwindow* window) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    SetupStyle();
}

ImGuiManager::~ImGuiManager() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void ImGuiManager::BeginFrame() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void ImGuiManager::EndFrame() {
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void ImGuiManager::SetupStyle() {
    ImGuiStyle& style = ImGui::GetStyle();

    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.08f, 0.08f, 0.12f, 0.94f);
    style.Colors[ImGuiCol_ChildBg] = ImVec4(0.08f, 0.08f, 0.12f, 0.94f);
    style.Colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.12f, 0.94f);
    style.Colors[ImGuiCol_Border] = ImVec4(0.18f, 0.18f, 0.23f, 1.0f);
    style.Colors[ImGuiCol_FrameBg] = ImVec4(0.20f, 0.20f, 0.25f, 1.0f);
    style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.28f, 0.28f, 0.35f, 1.0f);
    style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.35f, 0.35f, 0.45f, 1.0f);
    style.Colors[ImGuiCol_TitleBg] = ImVec4(0.15f, 0.15f, 0.20f, 1.0f);
    style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.15f, 0.15f, 0.20f, 1.0f);
    style.Colors[ImGuiCol_Button] = ImVec4(0.00f, 0.47f, 0.71f, 1.0f);
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.00f, 0.60f, 0.90f, 1.0f);
    style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.00f, 0.70f, 1.00f, 1.0f);
    style.Colors[ImGuiCol_Header] = ImVec4(0.00f, 0.47f, 0.71f, 0.31f);
    style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.00f, 0.47f, 0.71f, 0.51f);
    style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.00f, 0.47f, 0.71f, 0.71f);
    style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.00f, 0.47f, 0.71f, 1.0f);
    style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.00f, 0.60f, 0.90f, 1.0f);
    style.Colors[ImGuiCol_CheckMark] = ImVec4(0.00f, 0.47f, 0.71f, 1.0f);
    style.Colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.0f);
    style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.00f, 0.47f, 0.71f, 0.35f);

    // Arrodoniments i espaiat
    style.WindowPadding = ImVec2(15, 15);
    style.FramePadding = ImVec2(10, 8);
    style.ItemSpacing = ImVec2(12, 8);
    style.ItemInnerSpacing = ImVec2(8, 6);
    style.IndentSpacing = 25.0f;
    style.ScrollbarSize = 15.0f;
    style.GrabMinSize = 10.0f;

    // Bordes arrodonits
    style.WindowRounding = 12.0f;
    style.ChildRounding = 12.0f;
    style.FrameRounding = 8.0f;
    style.PopupRounding = 12.0f;
    style.ScrollbarRounding = 9.0f;
    style.GrabRounding = 8.0f;
    style.TabRounding = 8.0f;

    // Ombras i profunditat
    style.WindowBorderSize = 1.0f;
    style.FrameBorderSize = 0.0f;
    style.PopupBorderSize = 1.0f;
}
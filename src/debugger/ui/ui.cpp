#include <fwkes/desktop/ui/ui.hpp>

#include <fonts/codicons.h>

#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>
#include <imgui_memory_editor.h>

using namespace ui;

Ui::Ui(SDL_Window *win, SDL_Renderer *renderer, Vm &vm)
    : m_regs{vm}, m_memory{vm}, m_renderer{renderer}, m_vm{vm} {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |=
        ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_DockingEnable;

    io.Fonts->AddFontDefault();
    ImFontConfig fc;
    fc.MergeMode = true;
    fc.GlyphMinAdvanceX = 16.0f;
    fc.GlyphOffset.y = 5.f;
    io.Fonts->AddFontFromFileTTF("assets/fonts/codicons.ttf", 16.0f, &fc);

    ImGui::StyleColorsDark();

    ImGui_ImplSDL3_InitForSDLRenderer(win, renderer);
    ImGui_ImplSDLRenderer3_Init(renderer);
}

Ui::~Ui() {
    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
}

void Ui::update() {
    ImGui_ImplSDLRenderer3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    main_menu_bar();

    if (m_show_demo_window) {
        ImGui::ShowDemoWindow(&m_show_demo_window);
    }

    m_memory.render();
    m_regs.render();
}

void Ui::render() {
    ImGui::Render();
    ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), m_renderer);
}

void Ui::main_menu_bar() {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::MenuItem(ICON_CI_THREE_BARS " Registers"))
            m_regs.show();

        ImGui::Separator();

        if (ImGui::MenuItem(ICON_CI_FILE_BINARY " Memory"))
            m_memory.show();

        ImGui::Separator();

        if (ImGui::MenuItem(ICON_CI_WINDOW " Demo Window"))
            m_show_demo_window = true;

        ImGui::EndMainMenuBar();
    }
}

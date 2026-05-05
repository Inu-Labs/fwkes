#include <fwkes/debugger/ui/ui.hpp>

#include <fwkes/debugger/imgui.hpp>

#include <string_view>

using namespace ui;

Ui::Ui(SDL_Window *win, SDL_Renderer *renderer, Emulator &emu, MessageQueue &msg_queue)
    : disassembler{emu}, regs{emu}, memory_view{emu}, playback_recorder{emu}, canvas{nullptr},
      m_renderer{renderer}, m_emu{emu}, m_msg_queue{msg_queue} {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO &io = ImGui::GetIO();
    io.IniFilename = NULL;
    io.ConfigFlags |=
        ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_DockingEnable;
    m_io = &ImGui::GetIO();
    io.Fonts->AddFontFromFileTTF("/usr/share/fonts/TTF/RobotoMono-Regular.ttf", 18.0f, NULL);

    ImFontConfig fc;
    fc.MergeMode = true;
    fc.GlyphMinAdvanceX = 16.0f;
    fc.GlyphOffset.y = 5.f;
    io.Fonts->AddFontFromFileTTF("assets/fonts/codicons.ttf", 16.0f, &fc);

    ImGuiStyle &style = ImGui::GetStyle();
    style.WindowRounding = 0.f;
    style.WindowBorderSize = 1.f;

    ImGui::StyleColorsDark();

    ImGui_ImplSDL3_InitForSDLRenderer(win, renderer);
    ImGui_ImplSDLRenderer3_Init(renderer);

    disassembler.show();
    memory_view.show();
    regs.show();
    canvas.show();
    playback_recorder.show();
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

    begin_dockspace();

    input();
    main_menu_bar();
    tool_bar();
    status_bar();
    quit_popup();
    error_popup();

    this->disassembler.render();
    this->memory_view.render();
    this->regs.render();
    this->canvas.render();
    this->playback_recorder.update();
    this->playback_recorder.render();

    if (m_show_demo_window) {
        ImGui::ShowDemoWindow(&m_show_demo_window);
    }

    if (ImGuiFileDialog::Instance()->Display("LoadRomDlgKey")) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            std::string filepath_name =
                ImGuiFileDialog::Instance()->GetFilePathName();

            // TODO: display message in status bar

            m_msg_queue.emplace(MessageId::Load, MessageLoad{filepath_name});
        }

        ImGuiFileDialog::Instance()->Close();
    }

    end_dockspace();
}

void Ui::render() {
    ImGui::Render();
    ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), m_renderer);
}

void Ui::show_error(const std::string &err) {
    m_curr_err = err;
    m_open_error_popup = true;
}

void Ui::request_quit() {
    m_open_quit_popup = true;
}

void Ui::input() {
    if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
        if (m_emu.state() == EmuState::Paused) {
            m_msg_queue.emplace(MessageId::Resume);
        } else {
            m_msg_queue.emplace(MessageId::Pause);
        }
    } else if (ImGui::IsKeyChordPressed(ImGuiMod_Ctrl | ImGuiKey_R)) {
        m_msg_queue.emplace(MessageId::Reload);
    } else if (ImGui::IsKeyChordPressed(ImGuiMod_Ctrl | ImGuiKey_Q)) {
        m_msg_queue.emplace(MessageId::Quit);
    }
}

void Ui::main_menu_bar() {
    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem(ICON_CI_FILE_CODE " Load ROM")) {
                IGFD::FileDialogConfig fdc;
                fdc.path = ".";
                fdc.flags = ImGuiFileDialogFlags_Modal |
                            ImGuiFileDialogFlags_HideColumnType;

                ImGuiFileDialog::Instance()->OpenDialog(
                    "LoadRomDlgKey", "Choose ROM", ".*", fdc
                );
            }

            if (ImGui::MenuItem(ICON_CI_CHROME_CLOSE " Exit")) {
                m_msg_queue.emplace(MessageId::Quit);
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Debug")) {
            ImGui::MenuItem(
                ICON_CI_FILE_CODE " Disassembler", nullptr,
                &this->disassembler.show_flag()
            );

            ImGui::MenuItem(
                ICON_CI_THREE_BARS " Register View", nullptr,
                &this->regs.show_flag()
            );

            ImGui::MenuItem(
                ICON_CI_FILE_BINARY " Memory View", nullptr,
                &this->memory_view.show_flag()
            );

            ImGui::MenuItem(
                ICON_CI_FILE_BINARY " Playback Recorder", nullptr,
                &this->playback_recorder.show_flag()
            );

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Help")) {
            ImGui::MenuItem(
                ICON_CI_WINDOW " Demo Window", nullptr, &m_show_demo_window
            );

            ImGui::EndMenu();
        }

        ImGui::EndMenuBar();
    }
}

void Ui::begin_dockspace() {
    ImGuiViewport *viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);
    ImGui::SetNextWindowViewport(viewport->ID);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

    ImGuiWindowFlags window_flags =
        ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking |
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

    ImGui::Begin("Dockspace", nullptr, window_flags);
    ImGui::PopStyleVar(3);

    ImGuiID dockspace_id = ImGui::GetID("Dockspace");
    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);

    static bool first_time = true;

    if (first_time) {
        ImGui::DockBuilderRemoveNode(dockspace_id);
        ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
        ImGui::DockBuilderSetNodeSize(dockspace_id, ImGui::GetWindowSize());

        //
        // +----------------------------------------------------+
        // | PANEL 1 (TOOL BAR)                                 |
        // +----------------------------------------------------+
        // |                          |                         |
        // | PANEL 2                  | PANEL 3                 |
        // |                          |                         |
        // |                          |                         |
        // |                          |                         |
        // |                          |                         |
        // +--------------------------+-------------------------+
        // |                          |                         |
        // | PANEL 4                  | PANEL 5                 |
        // |                          |                         |
        // |                          |                         |
        // |                          |                         |
        // |                          |                         |
        // +--------------------------+-------------------------+
        // | PANEL 6 (STATUS BAR)                               |
        // +----------------------------------------------------+
        //

        ImGuiID dock_id_main = dockspace_id;

        ImGuiID panel_1 = ImGui::DockBuilderSplitNode(
            dock_id_main, ImGuiDir_Up, 0.030f, nullptr, &dock_id_main
        );

        ImGuiID panel_6 = ImGui::DockBuilderSplitNode(
            dock_id_main, ImGuiDir_Down, 0.030f, nullptr, &dock_id_main
        );

        ImGuiID panel_45;
        ImGuiID panel_23 = ImGui::DockBuilderSplitNode(
            dock_id_main, ImGuiDir_Up, 0.50f, nullptr, &panel_45
        );
        ImGuiID panel_4;
        ImGuiID panel_5 = ImGui::DockBuilderSplitNode(
            panel_45, ImGuiDir_Right, 0.50f, nullptr, &panel_4
        );

        ImGuiID panel_2;
        ImGuiID panel_3 = ImGui::DockBuilderSplitNode(
            panel_23, ImGuiDir_Right, 0.30f, nullptr, &panel_2
        );

        if (ImGuiDockNode *node = ImGui::DockBuilderGetNode(panel_1)) {
            node->LocalFlags |= ImGuiDockNodeFlags_NoTabBar |
                                ImGuiDockNodeFlags_NoResizeY |
                                ImGuiDockNodeFlags_NoDocking;
        }

        if (ImGuiDockNode *node = ImGui::DockBuilderGetNode(panel_6)) {
            node->LocalFlags |= ImGuiDockNodeFlags_NoTabBar |
                                ImGuiDockNodeFlags_NoResizeY |
                                ImGuiDockNodeFlags_NoDocking;
        }

        ImGui::DockBuilderDockWindow("Tool Bar", panel_1);
        ImGui::DockBuilderDockWindow("Canvas", panel_2);
        ImGui::DockBuilderDockWindow("Register View", panel_3);
        ImGui::DockBuilderDockWindow("Disassembler", panel_4);
        ImGui::DockBuilderDockWindow("Memory View", panel_5);
        ImGui::DockBuilderDockWindow("Status Bar", panel_6);

        ImGui::DockBuilderFinish(dockspace_id);

        first_time = false;
    }
}

void Ui::end_dockspace() { ImGui::End(); }

std::string_view emu_state_to_str(EmuState state) {
    switch (state) {
    case EmuState::Idle:
        return "IDLE";
    case EmuState::Running:
        return "RUNNING";
    case EmuState::Paused:
        return "PAUSED";
    }
}

Rgb emu_state_color(EmuState state) {
    switch (state) {
    case EmuState::Idle:
        return 0x82b2e6;
    case EmuState::Running:
        return 0xaeffaa;
    case EmuState::Paused:
        return 0xffdea9;
    }
}

void Ui::status_bar() {
    float h = ImGui::GetFrameHeight();

    ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(h / 6, h / 6));

    if (ImGui::Begin("Status Bar", nullptr, flags)) {
        ImGui::PopStyleVar(1);

        float font_size = ImGui::GetTextLineHeight();
        float vcenter = (h - font_size) * 0.5f;
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + vcenter);

        ImGui::TextUnformatted("State:");

        ImGui::SameLine();

        ImGui::TextColored(
            ImGuiExt::rgb_to_imvec4(emu_state_color(m_emu.state())), "%s",
            emu_state_to_str(m_emu.state()).data()
        );

        ImGui::SameLine();

        ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);

        ImGui::SameLine();

        ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
    } else {
        ImGui::PopStyleVar(1);
    }

    ImGui::End();
}

void Ui::tool_bar() {
    float h = ImGui::GetFrameHeight();

    ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(h / 6, h / 6));

    if (ImGui::Begin("Tool Bar", nullptr, flags)) {
        ImGui::PopStyleVar(1);

        ImGui::BeginDisabled(!m_emu.loaded());

        if (m_emu.state() == EmuState::Idle) {
            if (ImGuiExt::toolbar_btn(ICON_CI_DEBUG_START, 0x46cc56)) {
                m_msg_queue.emplace(MessageId::Start);
            }
        } else {
            if (ImGuiExt::toolbar_btn(ICON_CI_DEBUG_STOP, 0xa1260d)) {
                m_msg_queue.emplace(MessageId::Stop);
            }
        }

        ImGui::SameLine();
        ImGui::EndDisabled();

        ImGui::BeginDisabled(
            m_emu.state() != EmuState::Running && m_emu.state() != EmuState::Paused
        );

        if (m_emu.state() == EmuState::Running || m_emu.state() == EmuState::Idle) {
            if (ImGuiExt::toolbar_btn(ICON_CI_DEBUG_PAUSE, 0x007acc)) {
                m_msg_queue.emplace(MessageId::Pause);
            }
        } else {
            if (ImGuiExt::toolbar_btn(ICON_CI_DEBUG_CONTINUE, 0x007acc)) {
                m_msg_queue.emplace(MessageId::Resume);
            }
        }

        ImGui::SameLine();

        ImGui::EndDisabled();
        ImGui::BeginDisabled(m_emu.state() != EmuState::Paused);

        ImGuiExt::toolbar_btn(ICON_CI_DEBUG_STEP_BACK, 0x0093ef);
        ImGui::SameLine();

        if (ImGuiExt::toolbar_btn(ICON_CI_DEBUG_STEP_INTO, 0x0093ef)) {
            m_msg_queue.emplace(MessageId::StepIn);
        }

        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
            ImGui::SetItemTooltip("Step into");
        }

        ImGui::SameLine();
        ImGuiExt::toolbar_btn(ICON_CI_DEBUG_STEP_OUT, 0x0093ef);
        ImGui::SameLine();
        ImGuiExt::toolbar_btn(ICON_CI_DEBUG_STEP_OVER, 0x0093ef);

        ImGui::EndDisabled();
    } else {
        ImGui::PopStyleVar(1);
    }

    ImGui::End();
}

void Ui::quit_popup() {
    if (m_open_quit_popup) {
        ImGui::OpenPopup("Quit");
        m_open_quit_popup = false;
    }

    if (!ImGui::BeginPopupModal(
            "Quit", nullptr, ImGuiWindowFlags_AlwaysAutoResize
        )) {
        return;
    }

    ImGui::TextUnformatted("Are you sure you want to exit?");
    ImGui::Dummy({0, 10});

    if (ImGui::Button("Yes")) {
        m_msg_queue.emplace(MessageId::ForcedQuit);
    }

    ImGui::SameLine();

    if (ImGui::Button("Cancel")) {
        ImGui::CloseCurrentPopup();
    }

    ImGui::EndPopup();
}

void Ui::error_popup() {
    if (m_open_error_popup) {
        ImGui::OpenPopup("Error");
        m_open_error_popup = false;
    }

    if (!ImGui::BeginPopupModal("Error", nullptr, ImGuiWindowFlags_None)) {
        return;
    }

    ImGui::TextWrapped("%s", m_curr_err.c_str());
    ImGui::Dummy({0, 10});

    if (ImGui::Button("Continue")) {
        ImGui::CloseCurrentPopup();
    }

    ImGui::EndPopup();
}



// main.cpp - SDL3 + OpenGL + ImGui + ImPlot
// Butterfly Spread Visualizer

#include "imgui.h"
#include "implot.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_opengl3.h"
#include <SDL3/SDL.h>

// 根據平台選擇 OpenGL標頭檔
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <SDL3/SDL_opengles2.h>
#else
#include <SDL3/SDL_opengl.h>
#endif

#include <stdio.h>
#include <vector>
#include <cmath>
#include <algorithm>
#include <string>
#include <iostream>

// ----------------------------- Helper Functions -----------------------------
// 用來包裝 SliderScalar，使其用起來像 SliderDouble
bool SliderDouble(const char* label, double* v, double v_min, double v_max, const char* format = "%.3f", ImGuiSliderFlags flags = 0) {
    return ImGui::SliderScalar(label, ImGuiDataType_Double, v, &v_min, &v_max, format, flags);
}

// 用來包裝 InputScalar，使其用起來像 InputDouble
bool InputDouble(const char* label, double* v, double step = 0.0, double step_fast = 0.0, const char* format = "%.6f", ImGuiInputTextFlags flags = 0) {
    return ImGui::InputScalar(label, ImGuiDataType_Double, v, step > 0.0 ? &step : NULL, step_fast > 0.0 ? &step_fast : NULL, format, flags);
}

// ----------------------------- Math Logic -----------------------------
static inline double norm_cdf(double x) {
    constexpr double INV_SQRT2 = 0.7071067811865475244008443621048490;
    return 0.5 * std::erfc(-x * INV_SQRT2);
}

static inline double black_scholes_call(double S, double K, double T, double r, double sigma) {
    if (T <= 0.0) return std::max(0.0, S - K);
    if (S <= 0.0 || K <= 0.0 || sigma <= 0.0) return 0.0;
    const double sqrtT = std::sqrt(T);
    const double d1 = (std::log(S / K) + (r + 0.5 * sigma * sigma) * T) / (sigma * sqrtT);
    const double d2 = d1 - sigma * sqrtT;
    return S * norm_cdf(d1) - K * std::exp(-r * T) * norm_cdf(d2);
}

static inline double call_payoff(double S, double K) {
    return std::max(0.0, S - K);
}


void LoadChineseFont(ImGuiIO& io) {
    const char* font_paths[] = {
        "msyh.ttc",                              // 1. 優先找執行檔旁邊的字型
        "c:\\Windows\\Fonts\\msyh.ttc",          // 2. Windows 微軟正黑體
        "c:\\Windows\\Fonts\\simhei.ttf",        // 3. Windows 黑體
        "/System/Library/Fonts/PingFang.ttc",    // 4. MacOS 蘋方
        "/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc" // 5. Linux Noto
    };

    bool loaded = false;
    for (const char* path : font_paths) {
        FILE* f = fopen(path, "rb");
        if (f) {
            fclose(f);
            io.Fonts->AddFontFromFileTTF(path, 22.0f, NULL, io.Fonts->GetGlyphRangesChineseFull());
            printf("Loaded font: %s\n", path);
            loaded = true;
            break;
        }
    }
    if (io.Fonts == NULL) {
        printf("Error: Failed to load font!\n");
    }
    if (!loaded) {
        io.Fonts->AddFontDefault();
        printf("Warning: No Chinese font found.\n");
    }
}

void EnableWindowsConsole() {
    AllocConsole(); // 向 Windows 要一個黑視窗
    FILE* fp;
    freopen_s(&fp, "CONOUT$", "w", stdout); // 重導 stdout
    freopen_s(&fp, "CONOUT$", "w", stderr); // 重導 stderr

    // 讓 std::cout 也能運作
    std::ios::sync_with_stdio(true);
}

// ----------------------------- Main -----------------------------
int main(int, char**) {
    SetConsoleOutputCP(65001);
    EnableWindowsConsole();
    SDL_SetHint(SDL_HINT_IME_IMPLEMENTED_UI, "0");
    // 1. Setup SDL
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD)) {
        printf("Error: SDL_Init(): %s\n", SDL_GetError());
        return -1;
    }
    //SDL_SetHint(SDL_HINT_IME_IMPLEMENTED_UI, "composition,candidates");
    // 監控輸入設備 (包含鍵盤與 IME) -> 這是最關鍵的
    SDL_SetLogPriority(SDL_LOG_CATEGORY_INPUT, SDL_LOG_PRIORITY_DEBUG);
    // 監控視窗系統 (包含視窗訊息)
    SDL_SetLogPriority(SDL_LOG_CATEGORY_VIDEO, SDL_LOG_PRIORITY_DEBUG);
    // 2. Decide GL+GLSL versions
#if defined(IMGUI_IMPL_OPENGL_ES2)
    // GL ES 2.0 + GLSL 100
    const char* glsl_version = "#version 100";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#elif defined(__APPLE__)
    // GL 3.2 Core + GLSL 150
    const char* glsl_version = "#version 150";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG); // Always required on Mac
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
#else
    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#endif

    // 3. Create Window with Graphics Context
    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);
    SDL_Window* window = SDL_CreateWindow("Butterfly Spread Visualizer (SDL3 + OpenGL)", 1400, 820, window_flags);
    if (!window) {
        printf("Error: SDL_CreateWindow(): %s\n", SDL_GetError());
        return -1;
    }

    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    if (!gl_context) {
        printf("Error: SDL_GL_CreateContext(): %s\n", SDL_GetError());
        return -1;
    }

    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(1); // Enable vsync

    // 4. Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsLight();

    LoadChineseFont(io);


    // 5. Setup Platform/Renderer backends
    ImGui_ImplSDL3_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // App State (邏輯參數)
    double current_price = 95.0;
    double iv_pct = 18.0;
    int days_to_expiry = 27;
    double risk_free_pct = 4.0;
    double strike_atm = 100.0;
    double width = 5.0;
    bool show_explain = true;
    ImVec4 clear_color = ImVec4(1.0f, 1.0f, 1.0f, 1.00f);

    // 6. Main Loop
    bool done = false;
    while (!done) {
        // Poll and handle events
        SDL_Event event;
        while (SDL_PollEvent(&event)) {

            // --- 除錯代碼 Start ---
            // 監聽文字編輯事件 (IME 正在選字/組字時)
            if (event.type == SDL_EVENT_TEXT_EDITING) {
                printf("[IME Editing] Text: %s, Start: %d, Length: %d\n",
                    event.edit.text, event.edit.start, event.edit.length);
            }
            // 監聽文字輸入事件 (按下 Enter 確定文字後)
            else if (event.type == SDL_EVENT_TEXT_INPUT) {
                printf("[IME Input] Text: %s\n", event.text.text);
            }
            // 監聽鍵盤按鍵 (確認鍵盤還活著)
            else if (event.type == SDL_EVENT_KEY_DOWN) {
                printf("[Key Down] Scancode: %d\n", event.key.scancode);
            }
            // --- 除錯代碼 End ---

            ImGui_ImplSDL3_ProcessEvent(&event);
            if (event.type == SDL_EVENT_QUIT)
                done = true;
            if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && event.window.windowID == SDL_GetWindowID(window))
                done = true;
        }
        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        // --- UI Logic Start (保持不變) ---
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(io.DisplaySize);
        ImGui::Begin("##Host", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus);

        static char str[256] = "";
        ImGui::InputText("Test", str, IM_ARRAYSIZE(str));

        ImGui_ImplSDL3_ProcessEvent(&event);

        if (event.type == SDL_EVENT_QUIT)
            done = true;
        if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && event.window.windowID == SDL_GetWindowID(window))
            done = true;

        ImGui::BeginChild("##Sidebar", ImVec2(600.0f, 0), true);
        ImGui::Text("1. 市場參數");
        ImGui::Separator();

        InputDouble("當前股價 ($)", &current_price, 1.0, 5.0, "%.2f");
        if (current_price < 0.01) current_price = 0.01;
        SliderDouble("隱含波動率 (IV %)", &iv_pct, 1.0, 150.0, "%.0f");
        ImGui::SliderInt("距離到期天數", &days_to_expiry, 0, 90);
        InputDouble("無風險利率 (%)", &risk_free_pct, 0.1, 1.0, "%.2f");

        ImGui::Spacing();
        ImGui::Text("2. 策略設定 (蝶式)");
        ImGui::Separator();
        InputDouble("中間履約價 (ATM)", &strike_atm, 1.0, 5.0, "%.2f");
        InputDouble("履約價間距 (Width)", &width, 0.5, 1.0, "%.2f");
        if (width < 0.1) width = 0.1;

        double K_low = strike_atm - width;
        double K_mid = strike_atm;
        double K_high = strike_atm + width;

        ImGui::Spacing();
        ImGui::TextColored(ImVec4(0.2f, 0.4f, 0.8f, 1), "  Buy 1 Call @ %.2f", K_low);
        ImGui::TextColored(ImVec4(0.8f, 0.2f, 0.2f, 1), "  Sell 2 Calls @ %.2f", K_mid);
        ImGui::TextColored(ImVec4(0.2f, 0.4f, 0.8f, 1), "  Buy 1 Call @ %.2f", K_high);

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Checkbox("顯示書中概念對應", &show_explain);
        ImGui::EndChild();

        ImGui::SameLine();

        ImGui::BeginChild("##Content", ImVec2(0, 0), false);
        ImGui::Text("選擇權策略數學分析：蝶式價差 (Butterfly Spread)");
        ImGui::TextWrapped("此工具模擬書中強調的「期望值與時間價值」概念。觀察「當前曲線 (T+0)」如何隨著「時間流逝」與「波動率變化」而向到期損益線收斂。");
        ImGui::Spacing();

        double sigma = iv_pct / 100.0;
        double r = risk_free_pct / 100.0;
        double T = std::max(0.0, days_to_expiry / 365.0);
        double entry_cost = (black_scholes_call(current_price, K_low, T, r, sigma) +
            black_scholes_call(current_price, K_high, T, r, sigma)) -
            (2.0 * black_scholes_call(current_price, K_mid, T, r, sigma));

        const int n_points = 200;
        double x_min = current_price * 0.75;
        double x_max = current_price * 1.25;
        static std::vector<double> xs(n_points), ys_exp(n_points), ys_cur(n_points);

        double y_min = 1e9, y_max = -1e9;
        for (int i = 0; i < n_points; ++i) {
            double s = x_min + (x_max - x_min) * i / (n_points - 1);
            xs[i] = s;
            double val_exp = call_payoff(s, K_low) - 2.0 * call_payoff(s, K_mid) + call_payoff(s, K_high);
            ys_exp[i] = val_exp - entry_cost;
            double val_cur = black_scholes_call(s, K_low, T, r, sigma) -
                2.0 * black_scholes_call(s, K_mid, T, r, sigma) +
                black_scholes_call(s, K_high, T, r, sigma);
            ys_cur[i] = val_cur - entry_cost;

            if (ys_exp[i] < y_min) y_min = ys_exp[i]; if (ys_exp[i] > y_max) y_max = ys_exp[i];
            if (ys_cur[i] < y_min) y_min = ys_cur[i]; if (ys_cur[i] > y_max) y_max = ys_cur[i];
        }
        y_min = std::min(y_min, 0.0) - 1.0;
        y_max = std::max(y_max, 0.0) + 1.0;

        ImGui::Text("蝶式價差損益圖 (成本: $%.2f)", entry_cost);
        if (ImPlot::BeginPlot("##ButterflyPlot", ImVec2(-1, 500))) {
            ImPlot::SetupAxes("標的股價 (Stock Price)", "損益 (P&L)");
            ImPlot::SetupAxisLimits(ImAxis_X1, x_min, x_max, ImGuiCond_Always);
            ImPlot::SetupAxisLimits(ImAxis_Y1, y_min, y_max, ImGuiCond_Always);

            // [兼容性] 手動畫參考線
            ImPlotRect limits = ImPlot::GetPlotLimits();
            double h_xs[2] = { limits.X.Min, limits.X.Max }; double h_ys[2] = { 0.0, 0.0 };
            ImPlot::SetNextLineStyle(ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
            ImPlot::PlotLine("##Zero", h_xs, h_ys, 2);

            double v_xs[2] = { current_price, current_price }; double v_ys[2] = { limits.Y.Min, limits.Y.Max };
            ImPlot::SetNextLineStyle(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), 1.0f);
            ImPlot::PlotLine("現價", v_xs, v_ys, 2);

            ImPlot::SetNextLineStyle(ImVec4(0.9f, 0.2f, 0.2f, 1.0f), 2.0f);
            ImPlot::PlotLine("到期損益 (Expiration)", xs.data(), ys_exp.data(), n_points);

            ImPlot::SetNextLineStyle(ImVec4(0.2f, 0.4f, 0.9f, 1.0f), 3.0f);
            ImPlot::PushStyleVar(ImPlotStyleVar_FillAlpha, 0.2f);
            ImPlot::PlotShaded("當前損益區域", xs.data(), ys_cur.data(), n_points, 0.0);
            ImPlot::PopStyleVar();
            ImPlot::PlotLine("當前損益 (T+0)", xs.data(), ys_cur.data(), n_points);
            ImPlot::EndPlot();
        }

        if (show_explain) {
            ImGui::Separator();
            ImGui::Text("書中概念對應:");
            ImGui::BulletText("期望值區域 (The Tent)：紅色三角形區域是獲利目標區。");
            ImGui::BulletText("時間價值 (Time Decay)：減少「距離到期天數」，藍線會逐漸隆起貼近紅線。");
            ImGui::BulletText("波動率風險 (Vega Risk)：增加 IV，藍線會變得更平坦，代表獲利空間被壓縮。");
        }
        ImGui::EndChild();
        ImGui::End();
        // --- UI Logic End ---

        // Rendering
        ImGui::Render();
        glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImPlot::DestroyContext();
    ImGui::DestroyContext();

    SDL_GL_DestroyContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}


// main.cpp - SDL3 + SDLGPU3 + ImGui + ImPlot
// Butterfly Spread Visualizer

#include "imgui.h"
#include "implot.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_sdlgpu3.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_gpu.h>

#include <stdio.h>
#include <vector>
#include <cmath>
#include <algorithm>
#include <string>
#include <iostream>

#ifdef _WIN32
#define NOMINMAX
#include <Windows.h>
#endif

// ----------------------------- Helper Functions -----------------------------
// 用來包裝 SliderScalar，使其用起來像 SliderDouble
static bool SliderDouble(const char* label, double* v, double v_min, double v_max,
    const char* format = "%.3f", ImGuiSliderFlags flags = 0)
{
    return ImGui::SliderScalar(label, ImGuiDataType_Double, v, &v_min, &v_max, format, flags);
}

// 用來包裝 InputScalar，使其用起來像 InputDouble
static bool InputDouble(const char* label, double* v, double step = 0.0, double step_fast = 0.0,
    const char* format = "%.6f", ImGuiInputTextFlags flags = 0)
{
    return ImGui::InputScalar(label, ImGuiDataType_Double, v,
        step > 0.0 ? &step : NULL,
        step_fast > 0.0 ? &step_fast : NULL,
        format, flags);
}

// ----------------------------- Math Logic -----------------------------
static inline double norm_cdf(double x)
{
    constexpr double INV_SQRT2 = 0.7071067811865475244008443621048490;
    return 0.5 * std::erfc(-x * INV_SQRT2);
}

static inline double black_scholes_call(double S, double K, double T, double r, double sigma)
{
    if (T <= 0.0) return std::max(0.0, S - K);
    if (S <= 0.0 || K <= 0.0 || sigma <= 0.0) return 0.0;

    const double sqrtT = std::sqrt(T);
    const double d1 = (std::log(S / K) + (r + 0.5 * sigma * sigma) * T) / (sigma * sqrtT);
    const double d2 = d1 - sigma * sqrtT;
    return S * norm_cdf(d1) - K * std::exp(-r * T) * norm_cdf(d2);
}

static inline double call_payoff(double S, double K)
{
    return std::max(0.0, S - K);
}

static void LoadChineseFont(ImGuiIO& io)
{
    const char* font_paths[] = {
        "msyh.ttc",
        "c:\\Windows\\Fonts\\msyh.ttc",
        "c:\\Windows\\Fonts\\simhei.ttf",
        "/System/Library/Fonts/PingFang.ttc",
        "/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc"
    };

    bool loaded = false;
    for (const char* path : font_paths)
    {
        FILE* f = fopen(path, "rb");
        if (f)
        {
            fclose(f);
            io.Fonts->AddFontFromFileTTF(path, 22.0f, NULL, io.Fonts->GetGlyphRangesChineseFull());
            printf("Loaded font: %s\n", path);
            loaded = true;
            break;
        }
    }

    if (!loaded)
    {
        io.Fonts->AddFontDefault();
        printf("Warning: No Chinese font found.\n");
    }
}

#ifdef _WIN32
static void EnableWindowsConsole()
{
    AllocConsole();
    FILE* fp;
    freopen_s(&fp, "CONOUT$", "w", stdout);
    freopen_s(&fp, "CONOUT$", "w", stderr);
    std::ios::sync_with_stdio(true);
}
#endif

// ----------------------------- Main -----------------------------
int main(int, char**)
{
#ifdef _WIN32
    SetConsoleOutputCP(65001);
    EnableWindowsConsole();
#endif

    SDL_SetHint(SDL_HINT_IME_IMPLEMENTED_UI, "0");

    // 1. Setup SDL
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD))
    {
        printf("Error: SDL_Init(): %s\n", SDL_GetError());
        return -1;
    }

    SDL_SetLogPriority(SDL_LOG_CATEGORY_INPUT, SDL_LOG_PRIORITY_DEBUG);
    SDL_SetLogPriority(SDL_LOG_CATEGORY_VIDEO, SDL_LOG_PRIORITY_DEBUG);

    // 2. Create Window (NO OpenGL)
    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);
    SDL_Window* window = SDL_CreateWindow("Butterfly Spread Visualizer (SDL3 + SDLGPU3)", 1400, 820, window_flags);
    if (!window)
    {
        printf("Error: SDL_CreateWindow(): %s\n", SDL_GetError());
        SDL_Quit();
        return -1;
    }

    // 3. Create GPU Device + claim swapchain
    SDL_GPUDevice* gpu_device =
        SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_DXIL | SDL_GPU_SHADERFORMAT_METALLIB,
            true, nullptr);
    if (!gpu_device)
    {
        printf("Error: SDL_CreateGPUDevice(): %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }

    if (!SDL_ClaimWindowForGPUDevice(gpu_device, window))
    {
        printf("Error: SDL_ClaimWindowForGPUDevice(): %s\n", SDL_GetError());
        SDL_DestroyGPUDevice(gpu_device);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }

    SDL_SetGPUSwapchainParameters(gpu_device, window,
        SDL_GPU_SWAPCHAINCOMPOSITION_SDR,
        SDL_GPU_PRESENTMODE_MAILBOX);

    // 4. Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

    ImGui::StyleColorsLight();
    LoadChineseFont(io);

    // 5. Setup Platform/Renderer backends
    ImGui_ImplSDL3_InitForSDLGPU(window);

    ImGui_ImplSDLGPU3_InitInfo init_info = {};
    init_info.Device = gpu_device;
    init_info.ColorTargetFormat = SDL_GetGPUSwapchainTextureFormat(gpu_device, window);
    init_info.MSAASamples = SDL_GPU_SAMPLECOUNT_1;

    if (!ImGui_ImplSDLGPU3_Init(&init_info))
    {
        printf("Error: ImGui_ImplSDLGPU3_Init failed.\n");
        ImGui_ImplSDL3_Shutdown();
        ImPlot::DestroyContext();
        ImGui::DestroyContext();
        SDL_ReleaseWindowFromGPUDevice(gpu_device, window);
        SDL_DestroyGPUDevice(gpu_device);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }

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
    while (!done)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            // --- 除錯代碼 Start ---
            if (event.type == SDL_EVENT_TEXT_EDITING)
            {
                printf("[IME Editing] Text: %s, Start: %d, Length: %d\n",
                    event.edit.text, event.edit.start, event.edit.length);
            }
            else if (event.type == SDL_EVENT_TEXT_INPUT)
            {
                printf("[IME Input] Text: %s\n", event.text.text);
            }
            else if (event.type == SDL_EVENT_KEY_DOWN)
            {
                printf("[Key Down] Scancode: %d\n", event.key.scancode);
            }
            // --- 除錯代碼 End ---

            ImGui_ImplSDL3_ProcessEvent(&event);

            if (event.type == SDL_EVENT_QUIT)
                done = true;
            if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED &&
                event.window.windowID == SDL_GetWindowID(window))
                done = true;
        }

        if (SDL_GetWindowFlags(window) & SDL_WINDOW_MINIMIZED)
        {
            SDL_Delay(10);
            continue;
        }

        // Start the Dear ImGui frame
        ImGui_ImplSDLGPU3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        // --- UI Logic Start (保持不變) ---
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(io.DisplaySize);
        ImGui::Begin("##Host", nullptr,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus);

        static char str[256] = "";
        ImGui::InputText("Test", str, IM_ARRAYSIZE(str));

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

        double entry_cost =
            (black_scholes_call(current_price, K_low, T, r, sigma) +
                black_scholes_call(current_price, K_high, T, r, sigma)) -
            (2.0 * black_scholes_call(current_price, K_mid, T, r, sigma));

        const int n_points = 200;
        double x_min = current_price * 0.75;
        double x_max = current_price * 1.25;
        static std::vector<double> xs(n_points), ys_exp(n_points), ys_cur(n_points);

        double y_min = 1e9, y_max = -1e9;
        for (int i = 0; i < n_points; ++i)
        {
            double s = x_min + (x_max - x_min) * i / (n_points - 1);
            xs[i] = s;

            double val_exp = call_payoff(s, K_low) - 2.0 * call_payoff(s, K_mid) + call_payoff(s, K_high);
            ys_exp[i] = val_exp - entry_cost;

            double val_cur =
                black_scholes_call(s, K_low, T, r, sigma) -
                2.0 * black_scholes_call(s, K_mid, T, r, sigma) +
                black_scholes_call(s, K_high, T, r, sigma);
            ys_cur[i] = val_cur - entry_cost;

            if (ys_exp[i] < y_min) y_min = ys_exp[i]; if (ys_exp[i] > y_max) y_max = ys_exp[i];
            if (ys_cur[i] < y_min) y_min = ys_cur[i]; if (ys_cur[i] > y_max) y_max = ys_cur[i];
        }

        y_min = std::min(y_min, 0.0) - 1.0;
        y_max = std::max(y_max, 0.0) + 1.0;

        ImGui::Text("蝶式價差損益圖 (成本: $%.2f)", entry_cost);
        if (ImPlot::BeginPlot("##ButterflyPlot", ImVec2(-1, 500)))
        {
            ImPlot::SetupAxes("標的股價 (Stock Price)", "損益 (P&L)");
            ImPlot::SetupAxisLimits(ImAxis_X1, x_min, x_max, ImGuiCond_Always);
            ImPlot::SetupAxisLimits(ImAxis_Y1, y_min, y_max, ImGuiCond_Always);

            ImPlotRect limits = ImPlot::GetPlotLimits();
            double h_xs[2] = { limits.X.Min, limits.X.Max };
            double h_ys[2] = { 0.0, 0.0 };
            ImPlot::SetNextLineStyle(ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
            ImPlot::PlotLine("##Zero", h_xs, h_ys, 2);

            double v_xs[2] = { current_price, current_price };
            double v_ys[2] = { limits.Y.Min, limits.Y.Max };
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

        if (show_explain)
        {
            ImGui::Separator();
            ImGui::Text("書中概念對應:");
            ImGui::BulletText("期望值區域 (The Tent)：紅色三角形區域是獲利目標區。");
            ImGui::BulletText("時間價值 (Time Decay)：減少「距離到期天數」，藍線會逐漸隆起貼近紅線。");
            ImGui::BulletText("波動率風險 (Vega Risk)：增加 IV，藍線會變得更平坦，代表獲利空間被壓縮。");
        }

        ImGui::EndChild();
        ImGui::End();
        // --- UI Logic End ---

        // Rendering (SDLGPU3) — 依照官方範例順序
        ImGui::Render();
        ImDrawData* draw_data = ImGui::GetDrawData();
        const bool is_minimized = (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f);

        SDL_GPUCommandBuffer* command_buffer = SDL_AcquireGPUCommandBuffer(gpu_device);

        SDL_GPUTexture* swapchain_texture = nullptr;
        SDL_AcquireGPUSwapchainTexture(command_buffer, window, &swapchain_texture, nullptr, nullptr);

        if (swapchain_texture != nullptr && !is_minimized)
        {
            // 這行必做：上傳 vertex/index buffer
            ImGui_ImplSDLGPU3_PrepareDrawData(draw_data, command_buffer);

            SDL_GPUColorTargetInfo target_info = {};
            target_info.texture = swapchain_texture;
            target_info.clear_color = SDL_FColor{ clear_color.x, clear_color.y, clear_color.z, clear_color.w };
            target_info.load_op = SDL_GPU_LOADOP_CLEAR;
            target_info.store_op = SDL_GPU_STOREOP_STORE;
            target_info.mip_level = 0;
            target_info.layer_or_depth_plane = 0;
            target_info.cycle = false;

            SDL_GPURenderPass* render_pass = SDL_BeginGPURenderPass(command_buffer, &target_info, 1, nullptr);
            if (render_pass)
            {
                ImGui_ImplSDLGPU3_RenderDrawData(draw_data, command_buffer, render_pass);
                SDL_EndGPURenderPass(render_pass);
            }
            else
            {
                printf("Error: SDL_BeginGPURenderPass(): %s\n", SDL_GetError());
            }
        }

        SDL_SubmitGPUCommandBuffer(command_buffer);
    }

    // Cleanup
    SDL_WaitForGPUIdle(gpu_device);

    ImGui_ImplSDL3_Shutdown();
    ImGui_ImplSDLGPU3_Shutdown();
    ImPlot::DestroyContext();
    ImGui::DestroyContext();

    SDL_ReleaseWindowFromGPUDevice(gpu_device, window);
    SDL_DestroyGPUDevice(gpu_device);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}

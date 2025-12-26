static void ImGui_ImplSDL3_UpdateIme()
{
    ImGui_ImplSDL3_Data* bd = ImGui_ImplSDL3_GetBackendData();
    ImGuiPlatformImeData* data = &bd->ImeData;
    SDL_Window* window = SDL_GetKeyboardFocus();

    // Stop previous input
    if ((!(data->WantVisible || data->WantTextInput) || bd->ImeWindow != window) && bd->ImeWindow != nullptr)
    {
        SDL_StopTextInput(bd->ImeWindow);
        bd->ImeWindow = nullptr;
    }
    if ((!bd->ImeDirty && bd->ImeWindow == window) || (window == NULL))
        return;

    // Start/update current input
    bd->ImeDirty = false;
    if (data->WantVisible)
    {
        SDL_Rect r;
        r.x = (int)data->InputPos.x;
        r.y = (int)data->InputPos.y;
        r.h = (int)data->InputLineHeight;
        if (r.h < 20) {
            r.h = 20;
        }
        r.w = (int)(data->InputLineHeight * 8);
        
        SDL_SetTextInputArea(window, &r, 0);
        bd->ImeWindow = window;
    }
    if (!SDL_TextInputActive(window) && (data->WantVisible || data->WantTextInput))
        SDL_StartTextInput(window);
}
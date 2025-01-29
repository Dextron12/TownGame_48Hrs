

#ifndef WINDOW_HPP
#define WINDOW_HPP

#include "SDL.h"
#include "SDL_image.h"

#include "FileSystem.hpp"


#include <iostream>


#include <functional>

#ifdef IMPL_IMGUI
#include "imgui.h"
#include "backends/imgui_impl_sdl2.h"
#include "backends/imgui_impl_sdlrenderer2.h"
#endif


//A Timer class that can be started/stoped/paused. Warning outputs raw time in ms.To convert to seconds divide result by 1000
class Timer {
private:
    Uint32 m_StartTicks; //The clock time when the timer started.

    Uint32 m_PausedTicks; //The ticks stored when the timer was paused.

    //Timer status;
    bool Paused;
    bool Started;
public:
    Timer() {
        m_StartTicks = 0;
        m_PausedTicks = 0;

        Paused = false;
        Started = false;
    }

    //Clock actions
    void start() {
        Started = true;
        Paused = false;

        m_StartTicks = SDL_GetTicks();
        m_PausedTicks = 0;
    }

    void stop() {
        Started = false;
        Paused = false;

        m_StartTicks = 0;
        m_PausedTicks = 0;
    }

    void pause() {
        if (Started && !Paused) {
            Paused = true;

            //calculate paused ticks;
            m_PausedTicks = SDL_GetTicks() - m_StartTicks;
            m_StartTicks = 0;
        }
    }

    void unpause() {
        //If timer is running & paused
        if (Started && Paused) {
            Paused = false;

            m_StartTicks = SDL_GetTicks() - m_PausedTicks;
            m_PausedTicks = 0;
        }
    }

    Uint32 getTicks() {
        Uint32 time = 0;

        if (Started) {
            if (Paused){
                time = m_PausedTicks;
            }
            else {
                time = SDL_GetTicks() - m_StartTicks;
            }
        }
        return time;
    }

    //Checks the status of the timer
    bool isStarted() {
        return Started;
    }

    bool isPaused() {
        return Paused && Started;
    }
};

// Forward declarations for ImGui and SDL (you would include your actual ImGui setup here)
namespace ImGui {
    void NewFrame();
    void Render();
}

class Window {
public:
    Window(const char* title, int windowWidth, int windowHeight, Uint32 windowFlags)
        : width(windowWidth), height(windowHeight) {

        if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
            std::cout << "Failed to initiate SDL. Aborting" << std::endl;
            return;
        }

        window = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, windowWidth, windowHeight, windowFlags);
        if (window == NULL) {
            std::cout << "Failed to create a window. Aborting.\n" << SDL_GetError() << std::endl;
            return;
        }

        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED);
        if (renderer == NULL) {
            std::cout << "Failed to create a SDL renderer context. Aborting.\n" << SDL_GetError() << std::endl;
            return;
        }

        if (!IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG)) {
            std::cout << "Failed to initiate SDL Image. Aborting.\n" << SDL_GetError() << std::endl;
            return;
        }

        appState = true;

#ifdef IMPL_IMGUI
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();

        ImGuiIO& io = ImGui::GetIO();
        (void)io;

        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

        // Change ImGui Font
        std::string defFont = fs.joinToExecDir("Assets\\Fonts\\Roboto-Light.ttf");
        ImFont* customFont = io.Fonts->AddFontFromFileTTF(defFont.c_str(), 16.0f);
        if (customFont == nullptr) {
            std::printf("Error loading font: %s\n", defFont.c_str());
        }

        io.FontDefault = customFont;

        ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
        ImGui_ImplSDLRenderer2_Init(renderer);

        // Initialize the callback function for GUI rendering
        renderGUICallback = std::bind(&Window::renderGUI, this, std::placeholders::_1);
#endif
    }

    void update() {
        while (SDL_PollEvent(&event)) {
#ifdef IMPL_IMGUI
            ImGui_ImplSDL2_ProcessEvent(&event);
#endif

            if (event.type == SDL_QUIT) {
                appState = false;
            }

            if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
                SDL_GetWindowSize(window, &width, &height);
                windowResized = true;
            }
        }

        Keys = SDL_GetKeyboardState(NULL);

        Uint32 mouseData = SDL_GetMouseState(&MousePos.x, &MousePos.y);
        switch (mouseData) {
        case 1:
            mouse_LeftClick = true;
            break;
        case 2:
            // middle click
            break;
        case 4:
            mouse_RightClick = true;
            break;
        default:
            mouse_LeftClick = false;
            mouse_RightClick = false;
            break;
        }

        // Update deltaTime
        LAST = NOW;
        NOW = SDL_GetPerformanceCounter();
        deltaTime = ((NOW - LAST) * 1000) / SDL_GetPerformanceFrequency();

#ifdef IMPL_IMGUI
        updateImGui();
#endif

        SDL_RenderPresent(renderer);
    }

#ifdef IMPL_IMGUI
    void updateImGui() {
        ImGui_ImplSDL2_NewFrame();
        ImGui_ImplSDLRenderer2_NewFrame();
        ImGui::NewFrame();

        // Call the custom render callback
        if (renderGUICallback) {
            renderGUICallback(this);
        }

        ImGui::Render();
        ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), renderer);
    }

    // Virtual function to render the GUI, can be overridden by custom callback
    virtual void renderGUI(Window* win) {
        ImGui::Begin("My Window");
        ImGui::Text("Color");
        ImGui::End();
    }

    // Setter to assign an external render callback function
    void setRenderCallback(std::function<void(Window*)> cb) {
        renderGUICallback = cb;
    }
#endif

    ~Window() {
#ifdef IMPL_IMGUI
        ImGui_ImplSDLRenderer2_Shutdown();
        ImGui_ImplSDL2_Shutdown();
        ImGui::DestroyContext();
#endif
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        IMG_Quit();
        SDL_Quit();
    }

public:
    SDL_Window* window;
    SDL_Renderer* renderer;

    SDL_Event event;

    float game_version = 0.1;
    FileSystem fs;

    bool appState = false;
    int width, height;
    double deltaTime = 0;

    //bool Keys[322];
    const Uint8* Keys = SDL_GetKeyboardState(NULL);
    bool keyUp = true;

    // Mouse Events
    SDL_Point MousePos;
    bool mouse_LeftClick = false;
    bool mouse_RightClick = false;

    //Window Events
    bool windowResized = false;

#ifdef IMPL_IMGUI
    // Use std::function instead of function pointer for callback flexibility
    std::function<void(Window*)> renderGUICallback;
#endif

private:
    double LAST;
    double NOW = SDL_GetPerformanceCounter();
};

#endif // WINDOW_H

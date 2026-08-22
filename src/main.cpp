#include <SDL3/SDL.h>
#include <iostream>
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>
#include <array>
#include <unordered_set>
#include <algorithm>

void draw_imgui();
void draw();
void push_dummy_data();

struct Pos {
    int x, y;

    bool operator==(const Pos& other) const {
        return x == other.x && y == other.y;
    }
};

struct PosHasher {
    std::size_t operator()(const Pos& p) const {
        return std::hash<int>{}(p.x) ^
            (std::hash<int>{}(p.y) << 1);
    }
};

struct AppState {
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    bool vsync = false;
    double deltaTime = 0.0;
    double lastTime = 0.0;
};

struct GameState {
    std::unordered_set<Pos, PosHasher> cells;
    bool isPlaying = false;
    float accumulator = 0.0f;
    float steptime = 1.0f;
    int stepcount = 0;
};

struct EditorState {
    int step = 0;
    float cameraZoom = 1.0f; // 100%
    float cameraX = 0.0f;
    float cameraY = 0.0f;
    bool moveableCanvas = false;
};

AppState appState{};
GameState gameState{};
EditorState editorState{};

int main() {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << '\n';
        return 1;
    }

    appState.window = SDL_CreateWindow(
        "replife",
        1280,
        720,
        0
    );

    if (!appState.window) {
        std::cerr << "SDL_CreateWindow failed: " << SDL_GetError() << '\n';
        SDL_DestroyWindow(appState.window);
        SDL_Quit();
        return 1;
    }

    appState.renderer = SDL_CreateRenderer(appState.window, nullptr);

    if (!appState.renderer) {
        std::cerr << "Failed to setup sdl renderer" << SDL_GetError() << '\n';
        SDL_DestroyRenderer(appState.renderer);
        SDL_DestroyWindow(appState.window);
        SDL_Quit();
        return 1;
    }

    // --- ImGui init ---
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::StyleColorsDark();

    ImGui_ImplSDL3_InitForSDLRenderer(appState.window, appState.renderer);
    ImGui_ImplSDLRenderer3_Init(appState.renderer);
    // -------------------


    bool running = true;
    SDL_Event event;

    appState.lastTime = static_cast<double>(SDL_GetTicks()) / 1000.0;

    push_dummy_data();

    while (running) {
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL3_ProcessEvent(&event);
            if (event.type == SDL_EVENT_QUIT)
                running = false;
            if (event.type == SDL_EVENT_MOUSE_WHEEL && !io.WantCaptureMouse) {
                float factor = (event.wheel.y > 0) ? 1.1f : 1.0f / 1.1f;
                editorState.cameraZoom = std::clamp(editorState.cameraZoom * factor, 0.1f, 10.0f);
            }
            if ((event.type == SDL_EVENT_MOUSE_BUTTON_DOWN && (event.button.button == SDL_BUTTON_LEFT || event.button.button == SDL_BUTTON_MIDDLE))) {
                editorState.moveableCanvas = true;
            }
            if (event.type == SDL_EVENT_MOUSE_BUTTON_UP) {
                editorState.moveableCanvas = false;
            }
            if (event.type == SDL_EVENT_MOUSE_MOTION && editorState.moveableCanvas) {
                editorState.cameraX += event.motion.xrel;
                editorState.cameraY += event.motion.yrel;
            }
        }

        // Rendering...
        draw_imgui();
        draw();
    }

    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
    SDL_DestroyWindow(appState.window);
    SDL_Quit();

    return 0;
}

void push_dummy_data() {
    std::vector<Pos> data = {
        //Pos{0, 0},
        Pos{1, 1},
        Pos{0, 1},
        //Pos{-1, 0},
        Pos{0, -1},
        Pos{5, 3},
        Pos{-7, 4},
        Pos{20, -15},
        Pos{-35, 22},
        //Pos{100, -80}
    };


    for (auto& pos : data)
        gameState.cells.insert(pos);
}

void draw_imgui() {
    // Start ImGui frame
    ImGui_ImplSDLRenderer3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    ImGui::Begin("Debug");
    
    ImGui::Text("graphics api: %s", SDL_GetRendererName(appState.renderer));
    ImGui::Text("fps: %.3f", ImGui::GetIO().Framerate);
    ImGui::Text("step: %i", gameState.stepcount);
    ImGui::InputInt("step", &editorState.step, 1, 100, 0);

    if (ImGui::Button("Goto")) {
        
    };
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("goto a specific step");
    }

    ImGui::SliderFloat("step time", &gameState.steptime, 0.1f, 100.0f, "%.3f", 0);
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("time taken (in seconds) to complete 1 step of convoy's game of life.");
    }

    if (ImGui::Checkbox("v-sync", &appState.vsync)) {
        SDL_SetRenderVSync(appState.renderer, appState.vsync);
    }

    float blocksAway = 100.0f * 50.0f * editorState.cameraZoom;

    if (std::abs(editorState.cameraX) > blocksAway || std::abs(editorState.cameraY) > blocksAway) {
        if (ImGui::Button("Reset Camera")) {
            editorState.cameraY = 0.0f;
            editorState.cameraX = 0.0f;
            editorState.cameraZoom = 1.0f;
        };
    }

    ImGui::Checkbox("play simulation", &gameState.isPlaying);
    
    ImGui::End();

    ImGui::Render();
}

void step() {
    // process 1 step of convoy's game of live
    gameState.stepcount++;
}

void calculate_delta_time() {
    double curTime = static_cast<double>(SDL_GetTicks()) / 1000.0;
    appState.deltaTime = curTime - appState.lastTime;
    appState.lastTime = curTime;
}

void submitSquareDraw(float x, float y, std::vector<SDL_Vertex>& vertices, std::vector<int>& indices, float squareSide = 10) {
    unsigned int start = vertices.size();

    // square vertices
    vertices.push_back({
        x, y,
        1.0f, 1.0f, 1.0f, 1.0f,
        0.0f, 0.0f
    });

    vertices.push_back({
        x + squareSide, y,
        1.0f, 1.0f, 1.0f, 1.0f,
        1.0f, 0.0f
    });

    vertices.push_back({
        x + squareSide, y - squareSide,
        1.0f, 1.0f, 1.0f, 1.0f,
        1.0f, 1.0f
    });

    vertices.push_back({
        x, y - squareSide,
        1.0f, 1.0f, 1.0f, 1.0f,
        0.0f, 1.0f
    });

    // 1 square
    indices.push_back(start + 0);
    indices.push_back(start + 1);
    indices.push_back(start + 2);

    indices.push_back(start + 2);
    indices.push_back(start + 3);
    indices.push_back(start + 0);
}

void draw_cells() {
    std::vector<SDL_Vertex> vertices;
    std::vector<int> indices;

    int w, h;
    SDL_GetWindowSize(appState.window, &w, &h);

    for (const auto& pos : gameState.cells) {
        // TODO: discard cells if it's out of the visible boundary
        float squareSide = 50.0f * editorState.cameraZoom;
        submitSquareDraw(editorState.cameraX + static_cast<float>(w / 2) + pos.x * squareSide, editorState.cameraY + static_cast<float>(h / 2) + -pos.y * squareSide, vertices, indices, squareSide);
    }

    SDL_RenderGeometry(appState.renderer, nullptr, vertices.data(), vertices.size(), indices.data(), indices.size());
}

void draw() {
    calculate_delta_time();

    // step logic
    if (gameState.isPlaying) {
        gameState.accumulator += appState.deltaTime;
        while (gameState.accumulator >= gameState.steptime) {
            gameState.accumulator -= gameState.steptime;
            step();
        }
    }
    
    SDL_SetRenderDrawColor(appState.renderer, 0, 0, 0, 255);
    SDL_RenderClear(appState.renderer);

    draw_cells();
    
    ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), appState.renderer);

    SDL_RenderPresent(appState.renderer);
}
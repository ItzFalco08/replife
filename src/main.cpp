#include <SDL3/SDL.h>
#include <iostream>
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>
#include <array>
#include <unordered_set>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <future>
#include <thread>

namespace fs = std::filesystem;

void draw_imgui();
void draw();
//void push_dummy_data();
void load_patterns_info();
void load_pattern(const std::string& filePath);

struct Vec2 {
    int x, y;

    bool operator==(const Vec2& other) const {
        return x == other.x && y == other.y;
    }

    Vec2 operator+(const Vec2& other) const {
        return { x + other.x, y + other.y };
    }

    Vec2 operator-(const Vec2& other) const {
        return { x - other.x, y - other.y };
    }

    Vec2& operator+=(const Vec2& other) {
        x += other.x;
        y += other.y;
        return *this;
    }

    Vec2& operator-=(const Vec2& other) {
        x -= other.x;
        y -= other.y;
        return *this;
    }

    Vec2& operator*=(int scalar) {
        x *= scalar;
        y *= scalar;
        return *this;
    }
};

struct Vec2f {
    float x, y;

    bool operator==(const Vec2f& other) const {
        return x == other.x && y == other.y;
    }

    Vec2f operator+(const Vec2f& other) const {
        return { x + other.x, y + other.y };
    }

    Vec2f operator-(const Vec2f& other) const {
        return { x - other.x, y - other.y };
    }

    Vec2f& operator+=(const Vec2f& other) {
        x += other.x;
        y += other.y;
        return *this;
    }

    Vec2f& operator-=(const Vec2f& other) {
        x -= other.x;
        y -= other.y;
        return *this;
    }

    Vec2f& operator*=(float scalar) {
        x *= scalar;
        y *= scalar;
        return *this;
    }
};

struct Vec2Hasher {
    std::size_t operator()(const Vec2& p) const {
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
    std::unordered_set<Vec2, Vec2Hasher> cells;
    bool isPlaying = false;
    float accumulator = 0.0f;
    float steptime = 1.0f;
    int stepcount = 0;
};

struct EditorState {
    int step = 0;
    float cameraZoom = 1.0f;
    Vec2f cameraDrag{ 0.0f, 0.0f };
    bool moveableCanvas = false;
    int nrCellsOutsideBoundary = 0;
    int nrCellsVisible = 0;
    int selectedPatternIndex = -1;
};

using StepResult = std::pair<
    std::vector<Vec2>,
    std::vector<Vec2>
>;

std::vector<std::string> patternFiles;
std::future<StepResult> asyncfuture;

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

    //push_dummy_data();
    load_patterns_info();

    while (running) {
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL3_ProcessEvent(&event);
            if (event.type == SDL_EVENT_QUIT)
                running = false;
            if (event.type == SDL_EVENT_MOUSE_WHEEL && !io.WantCaptureMouse) {
                float mouseX, mouseY;
                int w, h;
                SDL_GetMouseState(&mouseX, &mouseY);
                SDL_GetWindowSize(appState.window, &w, &h);

                float factor = (event.wheel.y > 0) ? 1.1f : 1.0f / 1.1f;
                editorState.cameraZoom = std::clamp(editorState.cameraZoom * factor, 0.02f, 10.0f);

                editorState.cameraDrag *= factor;
            }
            if ((event.type == SDL_EVENT_MOUSE_BUTTON_DOWN && (event.button.button == SDL_BUTTON_LEFT || event.button.button == SDL_BUTTON_MIDDLE))) {
                editorState.moveableCanvas = true;
            }
            if (event.type == SDL_EVENT_MOUSE_BUTTON_UP) {
                editorState.moveableCanvas = false;
            }
            if (event.type == SDL_EVENT_MOUSE_MOTION && editorState.moveableCanvas && !io.WantCaptureMouse) {
                editorState.cameraDrag += {event.motion.xrel, event.motion.yrel};
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

void load_patterns_info() {
    if (!fs::exists(RLE_DIR)) {
        std::cerr << "patterns_rle directory not found: " << fs::absolute(RLE_DIR) << '\n';
        return;
    }

    patternFiles.clear();

    for (const auto& entry : fs::directory_iterator(RLE_DIR)) {
        if (entry.path().extension() == ".rle")
            patternFiles.push_back(entry.path().string());
    }
}

void load_pattern(const std::string& filePath) {
    gameState.cells.clear();

    std::ifstream file(filePath);
    if (!file) {
        std::cerr << "Failed to open pattern file: " << filePath << '\n';
        return;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string bfr_str = buffer.str();
    const char* bfr_ptr = bfr_str.c_str();
    const char* bfr_end = bfr_str.size() + bfr_ptr;



    int count = 0;
    int x = 0;
    int y = 0;


    // parse rle
    while (bfr_ptr < bfr_end) {
        // skip comments
        if (*bfr_ptr == '#') {
            while (bfr_ptr < bfr_end && *bfr_ptr != '\n') {
                bfr_ptr++;
            }
            bfr_ptr++; // skip the newline itself
            continue;
        }

        // skip newlines
        if (*bfr_ptr == '\n') {
            bfr_ptr++;
            continue;
        }

        // skip header
        if (*bfr_ptr == 'x') {
            while (bfr_ptr < bfr_end && *bfr_ptr != '\n') {
                bfr_ptr++;
            }
            bfr_ptr++;
            continue;
        }

        char c = *bfr_ptr;

        if (std::isdigit(static_cast<unsigned char>(c))) {
            count = count * 10 + (c - '0');
        }
        else if (c == 'b') {
            x += (count == 0) ? 1 : count;
            count = 0;
        }
        else if (c == 'o') {
            int n = (count == 0) ? 1 : count;

            for (int i = 0; i < n; i++) {
                gameState.cells.insert(Vec2{ i + x, y });
            }

            x = n + x;
            count = 0;
        }
        else if (c == '$') {
            int n = (count == 0) ? 1 : count;
            y += n;
            x = 0;
            count = 0;
        }
        else if (c == '!') {
            break;
        }

        bfr_ptr++;
    }
}

void draw_imgui() {
    // Start ImGui frame
    ImGui_ImplSDLRenderer3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    ImGui::Begin("Debug");

    if (ImGui::BeginTabBar("DebugTabs")) {

        // --- Stats: read-only info about current performance/state ---
        if (ImGui::BeginTabItem("Stats")) {
            ImGui::Text("graphics api: %s", SDL_GetRendererName(appState.renderer));
            ImGui::Text("fps: %.3f", ImGui::GetIO().Framerate);
            ImGui::Separator();
            ImGui::Text("step: %i", gameState.stepcount);
            ImGui::Text("total cells: %i", editorState.nrCellsOutsideBoundary + editorState.nrCellsVisible);
            ImGui::Text("cells visible: %i", editorState.nrCellsVisible);
            ImGui::Text("cells culled: %i", editorState.nrCellsOutsideBoundary);
            ImGui::EndTabItem();
        }

        // --- Simulation: playback controls, speed, jumping to a step ---
        if (ImGui::BeginTabItem("Simulation")) {
            ImGui::Checkbox("play simulation", &gameState.isPlaying);

            ImGui::SliderFloat("step time", &gameState.steptime, 0.01f, 1.0f, "%.3f", 0);
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("time taken (in seconds) to complete 1 step of conway's game of life.");
            }

            ImGui::InputInt("step", &editorState.step, 1, 100, 0);
            ImGui::SameLine();
            if (ImGui::Button("Goto")) {
                // TODO: jump simulation to editorState.step
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("goto a specific step");
            }

            if (!patternFiles.empty()) {
                // Build a list of display names (filename without extension), only once per frame
                std::vector<std::string> names;
                names.reserve(patternFiles.size());
                for (const auto& path : patternFiles)
                    names.push_back(std::filesystem::path(path).stem().string());

                // ImGui::ListBox needs const char* const*, so build that view too
                std::vector<const char*> namePtrs;
                namePtrs.reserve(names.size());
                for (const auto& n : names)
                    namePtrs.push_back(n.c_str());

                ImGui::ListBox("##patternList", &editorState.selectedPatternIndex,
                    namePtrs.data(), static_cast<int>(namePtrs.size()), 6 /* visible rows */);

                ImGui::BeginDisabled(editorState.selectedPatternIndex < 0);
                if (ImGui::Button("Load Pattern")) {
                    if (asyncfuture.valid())
                        asyncfuture.get();

                    gameState.cells.clear();
                    gameState.accumulator = 0.0;
                    gameState.isPlaying = false;
                    editorState.step = 0;
                    load_pattern(patternFiles[editorState.selectedPatternIndex]);
                }
                ImGui::EndDisabled();
            }
            else {
                ImGui::TextDisabled("No patterns found in patterns_rle/");
            }

            ImGui::EndTabItem();
        }

        // --- Camera: view/render settings ---
        if (ImGui::BeginTabItem("Camera")) {
            if (ImGui::Checkbox("v-sync", &appState.vsync)) {
                SDL_SetRenderVSync(appState.renderer, appState.vsync);
            }

            ImGui::Text("zoom: %.2fx", editorState.cameraZoom);
            ImGui::Text("drag: (%.1f, %.1f)", editorState.cameraDrag.x, editorState.cameraDrag.y);

            float blocksAway = 100.0f * 50.0f * editorState.cameraZoom;
            if (std::abs(editorState.cameraDrag.x) > blocksAway || std::abs(editorState.cameraDrag.y) > blocksAway) {
                if (ImGui::Button("Reset Camera")) {
                    editorState.cameraDrag = { 0.0f, 0.0f };
                    editorState.cameraZoom = 1.0f;
                }
            }

            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    ImGui::End();

    ImGui::Render();
}

StepResult step(const std::unordered_set<Vec2, Vec2Hasher>& cells) {
    constexpr Vec2 neighbours[8] = {
        {-1, 0}, {-1, -1}, {0, -1}, {1, -1},
        {1, 0}, {1, 1}, {0, 1}, {-1, 1},
    };

    std::vector<Vec2> toErase;
    std::vector<Vec2> toBirth;
    std::unordered_set<Vec2, Vec2Hasher> deadNeighbours;

    for (auto& cell : cells) {
        int nc = 0;
        for (const auto& neighbour : neighbours) {
            if (cells.contains(cell + neighbour)) nc++;
            else deadNeighbours.insert(cell + neighbour);
        }
        if (nc > 3 || nc < 2) toErase.push_back(cell);
    }

    for (auto& deadCell : deadNeighbours) {
        int nc = 0;
        for (const auto& neighbour : neighbours)
            if (cells.contains(deadCell + neighbour)) nc++;
        if (nc == 3) toBirth.push_back(deadCell);
    }

    return { std::move(toErase), std::move(toBirth) };
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

    editorState.nrCellsOutsideBoundary = 0;
    editorState.nrCellsVisible = 0;

    for (const auto& pos : gameState.cells) {
        float squareSide = 50.0f * editorState.cameraZoom;

        // discard cells if it's out of the visible boundary
        float screenX = editorState.cameraDrag.x + w / 2.0f + pos.x * squareSide;
        float screenY = editorState.cameraDrag.y + h / 2.0f - pos.y * squareSide;

        if (screenX + squareSide < 0 || screenX > w ||
            screenY < 0 || screenY - squareSide > h)
        {
            editorState.nrCellsOutsideBoundary++;
            continue;
        }

        editorState.nrCellsVisible++;

        submitSquareDraw(editorState.cameraDrag.x + static_cast<float>(w / 2) + pos.x * squareSide, editorState.cameraDrag.y + static_cast<float>(h / 2) + -pos.y * squareSide, vertices, indices, squareSide);
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

            // gets called once per steptime
            if (asyncfuture.valid()) {
                asyncfuture.wait();
                StepResult res = asyncfuture.get();
                for (const auto& toerase : res.first) gameState.cells.erase(toerase);
                for (const auto& tobirth : res.second) gameState.cells.insert(tobirth);
            }

            asyncfuture = std::async(std::launch::async, step, std::ref(gameState.cells));
        }
    }
    
    SDL_SetRenderDrawColor(appState.renderer, 0, 0, 0, 255);
    SDL_RenderClear(appState.renderer);

    draw_cells();
    
    ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), appState.renderer);

    SDL_RenderPresent(appState.renderer);
}
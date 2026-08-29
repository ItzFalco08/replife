#pragma once
#include <functional>
#include <unordered_map>

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
    float cpuComputeTime = 0.0f;
};

struct StepScratch {
    std::vector<Vec2> toErase;
    std::vector<Vec2> toBirth;
    std::unordered_map<Vec2, int, Vec2Hasher> neighbourCounts;
    double computeTime = 0.0;
};

struct StepResult {
    std::vector<Vec2> toErase{};
    std::vector<Vec2> toBirth{};
    double computeTime = 0.0;
};

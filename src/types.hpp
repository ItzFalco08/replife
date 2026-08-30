#pragma once
#include <functional>
#include <unordered_map>

using pos_t = int64_t;

inline constexpr pos_t pack(int32_t x, int32_t y) noexcept {
    return (static_cast<int64_t>(static_cast<uint32_t>(x)) << 32)
        | static_cast<uint32_t>(y);
}

inline constexpr int32_t unpackX(pos_t key) noexcept {
    return static_cast<int32_t>(static_cast<uint32_t>(key >> 32));
}

inline constexpr int32_t unpackY(pos_t key) noexcept {
    return static_cast<int32_t>(static_cast<uint32_t>(key & 0xFFFFFFFFu));
}

inline constexpr pos_t addDelta(pos_t cell, pos_t delta) noexcept {
    return pack(unpackX(cell) + unpackX(delta), unpackY(cell) + unpackY(delta));
}


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

struct AppState {
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    bool vsync = false;
    double deltaTime = 0.0;
    double lastTime = 0.0;
};

struct GameState {
    std::unordered_set<pos_t> cells;
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
    std::vector<pos_t> toErase;
    std::vector<pos_t> toBirth;
    std::unordered_set<pos_t> candidates;
    std::unordered_set<pos_t> nextCandidates;

    double computeTime = 0.0;
};

struct StepResult {
    std::vector<pos_t> toErase{};
    std::vector<pos_t> toBirth{};
    double computeTime = 0.0;
};

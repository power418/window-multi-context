#pragma once

#include <cstdint>
#include <map>
#include <algorithm>

namespace ui {

struct Rect {
    float x0, y0, x1, y1;
    
    constexpr bool empty() const noexcept {
        return x0 >= x1 || y0 >= y1;
    }
    
    constexpr Rect clipped(const Rect& clip) const noexcept {
        return {
            std::max(x0, clip.x0),
            std::max(y0, clip.y0),
            std::min(x1, clip.x1),
            std::min(y1, clip.y1)
        };
    }
};

constexpr float hoverSCurve(float amount) noexcept {
    const float t = std::clamp(amount, 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

/**
 * @brief Represents the interactive state of a UI component.
 */
struct InteractionState {
    float hoverAmount = 0.0f;
    float pressAmount = 0.0f;
    bool isHovered = false;
    bool isPressed = false;
};

/**
 * @brief Manages persistent hover states for smooth animations.
 */
class HoverTracker {
public:
    float update(uint32_t id, bool isHovered, float speed = 0.22f) noexcept {
        float& amount = states_[id];
        const float step = std::clamp(speed, 0.0f, 1.0f);
        if (isHovered) {
            amount += (1.0f - amount) * step;
        } else {
            amount *= (1.0f - step);
        }
        if (amount < 0.001f) amount = 0.0f;
        if (amount > 0.999f) amount = 1.0f;
        return hoverSCurve(amount);
    }

    float rawAmount(uint32_t id) const noexcept {
        const auto it = states_.find(id);
        return it == states_.end() ? 0.0f : it->second;
    }

private:
    std::map<uint32_t, float> states_;
};

class HoverUtility {
public:
    /**
     * @brief Check if a point is within a rectangle.
     */
    static constexpr bool containsPoint(const Rect& rect, float x, float y) noexcept {
        return !rect.empty() && x >= rect.x0 && x <= rect.x1 && y >= rect.y0 && y <= rect.y1;
    }

    static constexpr bool containsPointClipped(const Rect& rect, const Rect& clip, float x, float y) noexcept {
        return containsPoint(rect.clipped(clip), x, y);
    }

    /**
     * @brief Linear interpolation between two floats.
     */
    static constexpr float lerp(float a, float b, float t) noexcept {
        return a + (b - a) * std::clamp(t, 0.0f, 1.0f);
    }

    /**
     * @brief Smooth 0..1 interpolation curve for hover/press animations.
     */
    static constexpr float sCurve(float amount) noexcept {
        return hoverSCurve(amount);
    }

    /**
     * @brief Extract RGB from a standard 24-bit hex color (0xRRGGBB).
     */
    static constexpr uint8_t getR(uint32_t color) noexcept { return (color >> 16) & 0xFF; }
    static constexpr uint8_t getG(uint32_t color) noexcept { return (color >> 8) & 0xFF; }
    static constexpr uint8_t getB(uint32_t color) noexcept { return color & 0xFF; }
    static constexpr uint32_t makeColor(uint8_t r, uint8_t g, uint8_t b) noexcept {
        return (r << 16) | (g << 8) | b;
    }

    /**
     * @brief Linear interpolation between two 24-bit RGB colors.
     */
    static constexpr uint32_t lerpColor(uint32_t a, uint32_t b, float t) noexcept {
        const float alpha = std::clamp(t, 0.0f, 1.0f);
        uint8_t r = static_cast<uint8_t>(lerp(static_cast<float>(getR(a)), static_cast<float>(getR(b)), alpha));
        uint8_t g = static_cast<uint8_t>(lerp(static_cast<float>(getG(a)), static_cast<float>(getG(b)), alpha));
        uint8_t b_comp = static_cast<uint8_t>(lerp(static_cast<float>(getB(a)), static_cast<float>(getB(b)), alpha));
        return makeColor(r, g, b_comp);
    }

    /**
     * @brief Apply a hover highlight to a base 24-bit RGB color.
     */
    static constexpr uint32_t applyHover(uint32_t base, float amount) noexcept {
        if (amount <= 0.0f) return base;
        
        // Lighten the color slightly for hover
        const uint8_t r = static_cast<uint8_t>(std::min(255.0f, static_cast<float>(getR(base)) + 60.0f * amount));
        const uint8_t g = static_cast<uint8_t>(std::min(255.0f, static_cast<float>(getG(base)) + 60.0f * amount));
        const uint8_t b_comp = static_cast<uint8_t>(std::min(255.0f, static_cast<float>(getB(base)) + 60.0f * amount));
        return makeColor(r, g, b_comp);
    }
};

} // namespace ui

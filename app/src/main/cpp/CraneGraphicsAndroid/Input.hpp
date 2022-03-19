#pragma once

#include <array>
#include <map>
#include <unordered_map>
#include <string>

namespace Crane
{
    struct Input
    {
        std::pair<float, float> mousePos = {0.0f, 0.0f};
        std::pair<float, float> mousePosPrev = {0.0f, 0.0f};
        std::pair<float, float> scrollOffset = {0.0f, 0.0f};
        std::unordered_map<std::string, bool> keys = {
            {"lctrl", false},
            {"a", false},
            {"w", false},
            {"s", false},
            {"d", false},
            {"f", false},
            {"mouseButtonLeft", false},
            {"mouseButtonRight", false},
            {"mouseButtonMiddle", false}};

        void buttonDown(std::string buttonName)
        {
            keys[buttonName] = true;
        }

        void buttonUp(std::string buttonName)
        {
            keys[buttonName] = false;
        }

        void setMousePosition(float x, float y)
        {
            mousePosPrev = mousePos;
            mousePos = {x, y};
        }

        void setMouseScroll(float x, float y)
        {
            scrollOffset = {x, y};
        }
    };
}
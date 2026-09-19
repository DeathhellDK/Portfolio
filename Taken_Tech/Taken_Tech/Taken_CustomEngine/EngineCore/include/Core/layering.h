/**
 * @file      layering.h
 * @author    Jethro Sung
 * @email     sung.h
 * @date      2026-01-30
 *
 * @brief     Declares the engine Layering system used to control update and
 *            render behavior for groups of game objects.
 *
 * The layering system provides a lightweight and efficient way to enable or
 * disable entire categories of objects at runtime. Each layer stores two flags:
 *  - updateEnabled: controls whether objects in that layer run logic/physics.
 *  - renderEnabled: controls whether objects in that layer are drawn.
 *
 * This design avoids per-object visibility/update checks by centralizing the
 * state in a small Layering container. Systems can check a single flag per
 * layer before iterating large object sets, improving performance and keeping
 * logic clean.
 *
 * The default layer set includes:
 *  - Background: static background visuals
 *  - World:      main gameplay entities (player, enemies, walls, items, etc.)
 *  - UI:         HUD and menu widgets
 *  - Debug:      debug overlays, editor visuals, gizmos, etc.
 *
 * @version 1.0
 * @copyright Copyright (C) 2025
 * DigiPen Institute of Technology. Reproduction or disclosure of this file or
 * its contents without the prior written consent of DigiPen Institute of
 * Technology is prohibited.
 */
#pragma once
#include <array>
#include <cstdint>

namespace eng
{
    enum class LayerId : std::uint8_t
    {
        Background = 0,
        World,   // game objects layer
        UI,
        Debug,
        Count
    };

    struct LayerState
    {
        bool updateEnabled = true; // logic/physics
        bool renderEnabled = true; // visibility
    };

    class Layering
    {
    public:
        void SetUpdateEnabled(LayerId id, bool enabled) { states_[Idx(id)].updateEnabled = enabled; }
        void SetRenderEnabled(LayerId id, bool enabled) { states_[Idx(id)].renderEnabled = enabled; }

        bool UpdateEnabled(LayerId id) const { return states_[Idx(id)].updateEnabled; }
        bool RenderEnabled(LayerId id) const { return states_[Idx(id)].renderEnabled; }

        void ToggleUpdate(LayerId id) { states_[Idx(id)].updateEnabled = !states_[Idx(id)].updateEnabled; }
        void ToggleRender(LayerId id) { states_[Idx(id)].renderEnabled = !states_[Idx(id)].renderEnabled; }

        const LayerState& Get(LayerId id) const { return states_[Idx(id)]; }
        LayerState& Get(LayerId id) { return states_[Idx(id)]; }

    private:
        static constexpr std::size_t Idx(LayerId id) { return static_cast<std::size_t>(id); }
        std::array<LayerState, static_cast<std::size_t>(LayerId::Count)> states_{};
    };
}
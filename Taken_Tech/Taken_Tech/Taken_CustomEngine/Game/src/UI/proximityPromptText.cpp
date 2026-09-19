/**
 * @file      proximityPromptText.cpp
 * @author    Woh Kye Le
 * @email     w.kyele
 * @date      2026-04-03
 *
 * @brief     Implements proximity prompt lookup helpers.
 *
 * @version   1.0
 * @copyright
 * Copyright (C) 2026 DigiPen Institute of Technology.
 * Reproduction or disclosure of this file or its contents without the
 * prior written consent of DigiPen Institute of Technology is prohibited.
 */
#include "UI/proximityPromptText.h"
#include <json.hpp>
#include <fstream>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <string>

#include "Core/gameApp.h"
#include "Core/engine.hpp"
#include "Core/resourceManager.h"
#include "Mechanics/interaction.hpp"

#include <GLFW/glfw3.h>

namespace UI
{
    using json = nlohmann::json;

    static bool s_loaded = false;
    static std::unordered_map<PuzzleKind, ProximityPromptSpec> s_table;
    static ProximityPromptSpec s_firstAbsorb;
    static bool s_hasFirstAbsorb = false;
    static ProximityPromptSpec s_spawnIntro;
    static bool s_hasSpawnIntro = false;
    static ProximityPromptSpec s_firstMonsterEncounter;
    static bool s_hasFirstMonsterEncounter = false;
    static ProximityPromptSpec s_firstKeyDrop;
    static bool s_hasFirstKeyDrop = false;
    static ProximityPromptSpec s_secondKeyDrop;
    static bool s_hasSecondKeyDrop = false;
    static ProximityPromptSpec s_thirdKeyDrop;
    static bool s_hasThirdKeyDrop = false;
    static ProximityPromptSpec s_generatorRoom1;
    static bool s_hasGeneratorRoom1 = false;
    static float s_generatorRoom1DelaySec = 6.0f;
    static float s_generatorRoom1EnemyRadius = 200.0f;

    struct Room5SequenceStep
    {
        int x = 0;
        int y = 0;
        float wx = 0.0f;
        float wy = 0.0f;
        bool hasWorld = false;
        std::string text;
        float range = 0.0f;
    };

    static std::vector<Room5SequenceStep> s_room5Sequence;
    static bool s_hasRoom5Sequence = false;
    static ProximityPromptSpec s_room5SequenceStyle;
    static float s_room5SequenceTileSize = 100.0f;
    static int s_room5SequenceGridHeight = 25;

    static std::unordered_set<Entity> s_completedFirstEncounter;
    static std::unordered_set<Entity> s_dismissedByActive;
    static Entity s_lastProximityEntity = INVALID_ENTITY;
    static bool s_spawnIntroArmed = false;
    static bool s_spawnIntroCompleted = false;
    static Vector2 s_spawnIntroOrigin{};
    static bool s_spawnIntroOriginSet = false;
    static bool s_firstMonsterEncounterTriggered = false;
    static bool s_firstMonsterEncounterCompleted = false;
    static bool s_firstKeyDropTriggered = false;
    static bool s_firstKeyDropCompleted = false;
    static Entity s_firstKeyEntity = INVALID_ENTITY;
    static bool s_firstKeyDropSeen = false;
    static Vector2 s_firstKeyDropOrigin{};
    static bool s_firstKeyDropOriginSet = false;
    static bool s_secondKeyDropTriggered = false;
    static bool s_secondKeyDropCompleted = false;
    static Entity s_secondKeyEntity = INVALID_ENTITY;
    static bool s_secondKeyDropSeen = false;
    static Vector2 s_secondKeyDropOrigin{};
    static bool s_secondKeyDropOriginSet = false;
    static bool s_thirdKeyDropTriggered = false;
    static bool s_thirdKeyDropCompleted = false;
    static Entity s_thirdKeyEntity = INVALID_ENTITY;
    static bool s_thirdKeyDropSeen = false;
    static Vector2 s_thirdKeyDropOrigin{};
    static bool s_thirdKeyDropOriginSet = false;

    static bool s_generatorRoom1Triggered = false;
    static bool s_generatorRoom1Completed = false;
    static bool s_generatorRoom1SecondSeen = false;
    static float s_generatorRoom1Timer = 0.0f;
    static Vector2 s_generatorRoom1Origin{};
    static bool s_generatorRoom1OriginSet = false;

    static int s_room5SequenceIndex = 0;
    static bool s_room5SequenceSeen = false;
    static double s_room5EndCutsceneStartTime = -1.0;
    static bool s_room5EndCutsceneRequested = false;

    /**
     * @brief Resets proximity prompt state when restarting gameplay without the intro cutscene.
     */
    void ResetPromptsForRetry()
    {
        s_completedFirstEncounter.clear();
        s_dismissedByActive.clear();
        s_lastProximityEntity = INVALID_ENTITY;

        s_firstKeyDropTriggered = false;
        s_firstKeyDropCompleted = false;
        s_firstKeyEntity = INVALID_ENTITY;
        s_firstKeyDropSeen = false;
        s_firstKeyDropOrigin = {};
        s_firstKeyDropOriginSet = false;

        s_secondKeyDropTriggered = false;
        s_secondKeyDropCompleted = false;
        s_secondKeyEntity = INVALID_ENTITY;
        s_secondKeyDropSeen = false;
        s_secondKeyDropOrigin = {};
        s_secondKeyDropOriginSet = false;

        s_thirdKeyDropTriggered = false;
        s_thirdKeyDropCompleted = false;
        s_thirdKeyEntity = INVALID_ENTITY;
        s_thirdKeyDropSeen = false;
        s_thirdKeyDropOrigin = {};
        s_thirdKeyDropOriginSet = false;

        s_generatorRoom1Triggered = false;
        s_generatorRoom1Completed = false;
        s_generatorRoom1SecondSeen = false;
        s_generatorRoom1Timer = 0.0f;
        s_generatorRoom1Origin = {};
        s_generatorRoom1OriginSet = false;

        s_room5SequenceIndex = 0;
        s_room5SequenceSeen = false;
        s_room5EndCutsceneStartTime = -1.0;
        s_room5EndCutsceneRequested = false;

        s_spawnIntroArmed = false;
        s_spawnIntroCompleted = true;
        s_spawnIntroOrigin = {};
        s_spawnIntroOriginSet = false;

        s_firstMonsterEncounterTriggered = true;
        s_firstMonsterEncounterCompleted = true;
    }

    /**
     * @brief Resets proximity prompt state when starting a new run that includes the intro cutscene.
     */
    void ResetPromptsForCutsceneIntro()
    {
        s_completedFirstEncounter.clear();
        s_dismissedByActive.clear();
        s_lastProximityEntity = INVALID_ENTITY;

        s_firstKeyDropTriggered = false;
        s_firstKeyDropCompleted = false;
        s_firstKeyEntity = INVALID_ENTITY;
        s_firstKeyDropSeen = false;
        s_firstKeyDropOrigin = {};
        s_firstKeyDropOriginSet = false;

        s_secondKeyDropTriggered = false;
        s_secondKeyDropCompleted = false;
        s_secondKeyEntity = INVALID_ENTITY;
        s_secondKeyDropSeen = false;
        s_secondKeyDropOrigin = {};
        s_secondKeyDropOriginSet = false;

        s_thirdKeyDropTriggered = false;
        s_thirdKeyDropCompleted = false;
        s_thirdKeyEntity = INVALID_ENTITY;
        s_thirdKeyDropSeen = false;
        s_thirdKeyDropOrigin = {};
        s_thirdKeyDropOriginSet = false;

        s_generatorRoom1Triggered = false;
        s_generatorRoom1Completed = false;
        s_generatorRoom1SecondSeen = false;
        s_generatorRoom1Timer = 0.0f;
        s_generatorRoom1Origin = {};
        s_generatorRoom1OriginSet = false;

        s_room5SequenceIndex = 0;
        s_room5SequenceSeen = false;
        s_room5EndCutsceneStartTime = -1.0;
        s_room5EndCutsceneRequested = false;

        s_spawnIntroArmed = false;
        s_spawnIntroCompleted = false;
        s_spawnIntroOrigin = {};
        s_spawnIntroOriginSet = false;

        s_firstMonsterEncounterTriggered = false;
        s_firstMonsterEncounterCompleted = false;
    }

    /**
     * @brief Converts a kind name string to a PuzzleKind value.
     * @param s Kind name string from json.
     * @return PuzzleKind value.
     */
    static PuzzleKind ParseKind(const std::string& s) {
        if (s == "None") return PuzzleKind::None;
        if (s == "BurrowWall") return PuzzleKind::BurrowWall;
        if (s == "Lever") return PuzzleKind::Lever;
        if (s == "Plate") return PuzzleKind::Plate;
        if (s == "DoorLock") return PuzzleKind::DoorLock;
        if (s == "Switch") return PuzzleKind::Switch;
        if (s == "ShootTarget") return PuzzleKind::ShootTarget;
        if (s == "HealingMemory") return PuzzleKind::HealingMemory;
        if (s == "BurrowFloor") return PuzzleKind::BurrowFloor;
        if (s == "MutationHealer") return PuzzleKind::MutationHealer;
        return PuzzleKind::None;
    }

    /**
     * @brief Parses a color value into packed rgba format.
     * @param j Json value holding rgba.
     * @return Packed rgba as 0xRRGGBBAA.
     */
    static uint32_t ParseRGBA(const json& j){
        if (j.is_string()) {
            std::string hex = j.get<std::string>();
            // Accept "FFFFFFFF" or "0xFFFFFFFF"
            if (hex.rfind("0x", 0) == 0 || hex.rfind("0X", 0) == 0) hex = hex.substr(2);
            if (hex.size() == 8) {
                uint32_t v = 0;
                for (char c : hex) {
                    v <<= 4;
                    if (c >= '0' && c <= '9') v |= (c - '0');
                    else if (c >= 'a' && c <= 'f') v |= (c - 'a' + 10);
                    else if (c >= 'A' && c <= 'F') v |= (c - 'A' + 10);
                }
                return v;
            }
        }
        if (j.is_number_unsigned()) return j.get<uint32_t>();
        if (j.is_array() && j.size() == 4) {
            uint32_t r = j[0].get<uint32_t>() & 0xFF;
            uint32_t g = j[1].get<uint32_t>() & 0xFF;
            uint32_t b = j[2].get<uint32_t>() & 0xFF;
            uint32_t a = j[3].get<uint32_t>() & 0xFF;
            return (
                r << 24) | (g << 16) | (b << 8) | a;
        }
        return 0xFFFFFFFFu;
    }

    /**
     * @brief Loads proximity prompt configuration from json once.
     */
    static void LoadTableOnce(){
        if (s_loaded) return;
        s_loaded = true;
        try {
            std::string path = std::string(ASSET_ROOT_DIR) + "/scene/proximityPromptText.json";
            std::ifstream in(path);
            if (!in.is_open()) return;
            json j; in >> j;

            if (j.contains("kinds") && j["kinds"].is_object()) {
                for (auto it = j["kinds"].begin(); it != j["kinds"].end(); ++it) {
                    PuzzleKind k = ParseKind(it.key());
                    const json& cfg = it.value();
                    ProximityPromptSpec spec;
                    if (cfg.contains("text")) spec.text = cfg["text"].get<std::string>();
                    if (cfg.contains("first")) spec.firstText = cfg["first"].get<std::string>();
                    if (cfg.contains("repeat")) spec.repeatText = cfg["repeat"].get<std::string>();
                    if (cfg.contains("font")) spec.font = cfg["font"].get<std::string>();
                    if (cfg.contains("size")) spec.sizePx = cfg["size"].get<float>();
                    if (cfg.contains("range")) spec.range = cfg["range"].get<float>();
                    if (cfg.contains("rgba")) spec.rgba = ParseRGBA(cfg["rgba"]);
                    if (spec.firstText.empty() && !spec.text.empty()) spec.firstText = spec.text;
                    if (spec.repeatText.empty() && !spec.text.empty()) spec.repeatText = spec.text;
                    s_table[k] = spec;
                }
            }

            if (j.contains("events") && j["events"].is_object()) {
                const json& ev = j["events"];
                if (ev.contains("FirstAbsorb") && ev["FirstAbsorb"].is_object()) {
                    const json& cfg = ev["FirstAbsorb"];
                    ProximityPromptSpec spec;
                    if (cfg.contains("text")) spec.text = cfg["text"].get<std::string>();
                    if (cfg.contains("first")) spec.firstText = cfg["first"].get<std::string>();
                    if (cfg.contains("repeat")) spec.repeatText = cfg["repeat"].get<std::string>();
                    if (cfg.contains("font")) spec.font = cfg["font"].get<std::string>();
                    if (cfg.contains("size")) spec.sizePx = cfg["size"].get<float>();
                    if (cfg.contains("range")) spec.range = cfg["range"].get<float>();
                    if (cfg.contains("rgba")) spec.rgba = ParseRGBA(cfg["rgba"]);
                    if (spec.firstText.empty() && !spec.text.empty()) spec.firstText = spec.text;
                    if (spec.repeatText.empty() && !spec.text.empty()) spec.repeatText = spec.text;
                    s_firstAbsorb = spec;
                    s_hasFirstAbsorb = true;
                }
                if (ev.contains("SpawnIntro") && ev["SpawnIntro"].is_object()) {
                    const json& cfg = ev["SpawnIntro"];
                    ProximityPromptSpec spec;
                    if (cfg.contains("text")) spec.text = cfg["text"].get<std::string>();
                    if (cfg.contains("first")) spec.firstText = cfg["first"].get<std::string>();
                    if (cfg.contains("repeat")) spec.repeatText = cfg["repeat"].get<std::string>();
                    if (cfg.contains("font")) spec.font = cfg["font"].get<std::string>();
                    if (cfg.contains("size")) spec.sizePx = cfg["size"].get<float>();
                    if (cfg.contains("range")) spec.range = cfg["range"].get<float>();
                    if (cfg.contains("rgba")) spec.rgba = ParseRGBA(cfg["rgba"]);
                    if (spec.firstText.empty() && !spec.text.empty()) spec.firstText = spec.text;
                    if (spec.repeatText.empty() && !spec.text.empty()) spec.repeatText = spec.text;
                    s_spawnIntro = spec;
                    s_hasSpawnIntro = true;
                }
                if (ev.contains("FirstMonsterEncounter") && ev["FirstMonsterEncounter"].is_object()) {
                    const json& cfg = ev["FirstMonsterEncounter"];
                    ProximityPromptSpec spec;
                    if (cfg.contains("text")) spec.text = cfg["text"].get<std::string>();
                    if (cfg.contains("first")) spec.firstText = cfg["first"].get<std::string>();
                    if (cfg.contains("repeat")) spec.repeatText = cfg["repeat"].get<std::string>();
                    if (cfg.contains("font")) spec.font = cfg["font"].get<std::string>();
                    if (cfg.contains("size")) spec.sizePx = cfg["size"].get<float>();
                    if (cfg.contains("range")) spec.range = cfg["range"].get<float>();
                    if (cfg.contains("rgba")) spec.rgba = ParseRGBA(cfg["rgba"]);
                    if (spec.firstText.empty() && !spec.text.empty()) spec.firstText = spec.text;
                    if (spec.repeatText.empty() && !spec.text.empty()) spec.repeatText = spec.text;
                    s_firstMonsterEncounter = spec;
                    s_hasFirstMonsterEncounter = true;
                }
                if (ev.contains("FirstKeyDrop") && ev["FirstKeyDrop"].is_object()) {
                    const json& cfg = ev["FirstKeyDrop"];
                    ProximityPromptSpec spec;
                    if (cfg.contains("text")) spec.text = cfg["text"].get<std::string>();
                    if (cfg.contains("first")) spec.firstText = cfg["first"].get<std::string>();
                    if (cfg.contains("repeat")) spec.repeatText = cfg["repeat"].get<std::string>();
                    if (cfg.contains("font")) spec.font = cfg["font"].get<std::string>();
                    if (cfg.contains("size")) spec.sizePx = cfg["size"].get<float>();
                    if (cfg.contains("range")) spec.range = cfg["range"].get<float>();
                    if (cfg.contains("rgba")) spec.rgba = ParseRGBA(cfg["rgba"]);
                    if (spec.firstText.empty() && !spec.text.empty()) spec.firstText = spec.text;
                    if (spec.repeatText.empty() && !spec.text.empty()) spec.repeatText = spec.text;
                    s_firstKeyDrop = spec;
                    s_hasFirstKeyDrop = true;
                }
                if (ev.contains("SecondKeyDrop") && ev["SecondKeyDrop"].is_object()) {
                    const json& cfg = ev["SecondKeyDrop"];
                    ProximityPromptSpec spec;
                    if (cfg.contains("text")) spec.text = cfg["text"].get<std::string>();
                    if (cfg.contains("first")) spec.firstText = cfg["first"].get<std::string>();
                    if (cfg.contains("repeat")) spec.repeatText = cfg["repeat"].get<std::string>();
                    if (cfg.contains("font")) spec.font = cfg["font"].get<std::string>();
                    if (cfg.contains("size")) spec.sizePx = cfg["size"].get<float>();
                    if (cfg.contains("range")) spec.range = cfg["range"].get<float>();
                    if (cfg.contains("rgba")) spec.rgba = ParseRGBA(cfg["rgba"]);
                    if (spec.firstText.empty() && !spec.text.empty()) spec.firstText = spec.text;
                    if (spec.repeatText.empty() && !spec.text.empty()) spec.repeatText = spec.text;
                    s_secondKeyDrop = spec;
                    s_hasSecondKeyDrop = true;
                }
                if (ev.contains("ThirdKeyDrop") && ev["ThirdKeyDrop"].is_object()) {
                    const json& cfg = ev["ThirdKeyDrop"];
                    ProximityPromptSpec spec;
                    if (cfg.contains("text")) spec.text = cfg["text"].get<std::string>();
                    if (cfg.contains("first")) spec.firstText = cfg["first"].get<std::string>();
                    if (cfg.contains("repeat")) spec.repeatText = cfg["repeat"].get<std::string>();
                    if (cfg.contains("font")) spec.font = cfg["font"].get<std::string>();
                    if (cfg.contains("size")) spec.sizePx = cfg["size"].get<float>();
                    if (cfg.contains("range")) spec.range = cfg["range"].get<float>();
                    if (cfg.contains("rgba")) spec.rgba = ParseRGBA(cfg["rgba"]);
                    if (spec.firstText.empty() && !spec.text.empty()) spec.firstText = spec.text;
                    if (spec.repeatText.empty() && !spec.text.empty()) spec.repeatText = spec.text;
                    s_thirdKeyDrop = spec;
                    s_hasThirdKeyDrop = true;
                }
                if (ev.contains("GeneratorRoom1") && ev["GeneratorRoom1"].is_object()) {
                    const json& cfg = ev["GeneratorRoom1"];
                    ProximityPromptSpec spec;
                    if (cfg.contains("text")) spec.text = cfg["text"].get<std::string>();
                    if (cfg.contains("first")) spec.firstText = cfg["first"].get<std::string>();
                    if (cfg.contains("repeat")) spec.repeatText = cfg["repeat"].get<std::string>();
                    if (cfg.contains("font")) spec.font = cfg["font"].get<std::string>();
                    if (cfg.contains("size")) spec.sizePx = cfg["size"].get<float>();
                    if (cfg.contains("range")) spec.range = cfg["range"].get<float>();
                    if (cfg.contains("rgba")) spec.rgba = ParseRGBA(cfg["rgba"]);
                    if (cfg.contains("delay")) s_generatorRoom1DelaySec = cfg["delay"].get<float>();
                    if (cfg.contains("enemyRadius")) s_generatorRoom1EnemyRadius = cfg["enemyRadius"].get<float>();
                    if (spec.firstText.empty() && !spec.text.empty()) spec.firstText = spec.text;
                    if (spec.repeatText.empty() && !spec.text.empty()) spec.repeatText = spec.text;
                    s_generatorRoom1 = spec;
                    s_hasGeneratorRoom1 = true;
                }
                if (ev.contains("Room5Sequence") && ev["Room5Sequence"].is_object()) {
                    const json& cfg = ev["Room5Sequence"];
                    if (cfg.contains("tileSize")) s_room5SequenceTileSize = cfg["tileSize"].get<float>();
                    if (cfg.contains("gridHeight")) s_room5SequenceGridHeight = cfg["gridHeight"].get<int>();

                    s_room5SequenceStyle = {};
                    if (cfg.contains("font")) s_room5SequenceStyle.font = cfg["font"].get<std::string>();
                    if (cfg.contains("size")) s_room5SequenceStyle.sizePx = cfg["size"].get<float>();
                    if (cfg.contains("range")) s_room5SequenceStyle.range = cfg["range"].get<float>();
                    if (cfg.contains("rgba")) s_room5SequenceStyle.rgba = ParseRGBA(cfg["rgba"]);

                    s_room5Sequence.clear();
                    if (cfg.contains("steps") && cfg["steps"].is_array()) {
                        for (const auto& s : cfg["steps"]) {
                            if (!s.is_object()) continue;
                            Room5SequenceStep step;
                            if (s.contains("x")) step.x = s["x"].get<int>();
                            if (s.contains("y")) step.y = s["y"].get<int>();
                            if (s.contains("wx")) { step.wx = s["wx"].get<float>(); step.hasWorld = true; }
                            if (s.contains("wy")) { step.wy = s["wy"].get<float>(); step.hasWorld = true; }
                            if (s.contains("text")) step.text = s["text"].get<std::string>();
                            if (s.contains("range")) step.range = s["range"].get<float>();
                            if (!step.text.empty()) s_room5Sequence.push_back(step);
                        }
                    }
                    s_hasRoom5Sequence = !s_room5Sequence.empty();
                }
            }
        }
        catch (...) {
        }
    }

    /**
     * @brief Returns proximity prompt specification for a given entities kind.
     * @param kind Entities classification used as a key.
     * @return ProximityPromptSpec loaded from json or empty if not configured.
     */
    ProximityPromptSpec GetPromptForEntitiesKind(PuzzleKind kind){
        LoadTableOnce();
        auto it = s_table.find(kind);
        if (it != s_table.end()) return it->second;
        return {};
    }

    /**
     * @brief Returns proximity prompt specification for the first ability absorption tutorial.
     * @return ProximityPromptSpec loaded from json or empty if not configured.
     */
    ProximityPromptSpec GetPromptForFirstAbsorb(){
        LoadTableOnce();
        if (!s_hasFirstAbsorb) return {};
        return s_firstAbsorb;
    }

    /**
     * @brief Returns active proximity prompt for the player based on current world state.
     * @param app The game application used to query entities and puzzle state.
     * @param player The player entity.
     * @return ProximityPromptResult containing text style when active.
     */
    ProximityPromptResult GetActivePromptForPlayer(GameApp& app, Entity player)
    {
        ProximityPromptResult out;
        if (player == INVALID_ENTITY) return out;

        Transform* pt = app.GetTransform(player);
        if (!pt) return out;

        const Vector2 pPos = pt->GetPosition();
        const Vector2 pSize = pt->GetScale();
        const Vector2 pCenter{ pPos.x + pSize.x * 0.5f, pPos.y + pSize.y * 0.5f };

        const float baseRange = 160.0f;

        {
            if (!s_spawnIntroCompleted && s_hasSpawnIntro) {
                if (!s_spawnIntroArmed) {
                    s_spawnIntroArmed = true;
                    s_spawnIntroOrigin = pCenter;
                    s_spawnIntroOriginSet = true;
                }

                if (s_spawnIntroArmed && s_spawnIntroOriginSet) {
                    const std::string text = !s_spawnIntro.text.empty() ? s_spawnIntro.text : s_spawnIntro.firstText;
                    const float range = (s_spawnIntro.range > 0.0f) ? s_spawnIntro.range : 100.0f;
                    const float dx = pCenter.x - s_spawnIntroOrigin.x;
                    const float dy = pCenter.y - s_spawnIntroOrigin.y;
                    const float d2 = dx * dx + dy * dy;
                    const float r2 = range * range;

                    if (d2 > r2) {
                        s_spawnIntroCompleted = true;
                        s_spawnIntroArmed = false;
                    }
                    else if (!text.empty()) {
                        out.show = true;
                        out.text = text;
                        out.font = s_spawnIntro.font;
                        out.sizePx = s_spawnIntro.sizePx;
                        out.rgba = s_spawnIntro.rgba;
                        return out;
                    }
                }
            }
        }

        {
            if (s_spawnIntroCompleted && !s_firstMonsterEncounterCompleted && s_hasFirstMonsterEncounter) {
                const std::string text = !s_firstMonsterEncounter.text.empty() ? s_firstMonsterEncounter.text : s_firstMonsterEncounter.firstText;
                const float range = (s_firstMonsterEncounter.range > 0.0f) ? s_firstMonsterEncounter.range : baseRange;
                const float r2 = range * range;

                bool enemyInRange = false;
                for (const auto& kv : app.GetAllColliders()) {
                    const Entity e = kv.first;
                    if (!app.GetEnemyController(e)) continue;
                    Transform* t = app.GetTransform(e);
                    if (!t) continue;

                    Vector2 ePos = t->GetPosition();
                    Vector2 eSize = t->GetScale();
                    if (Collider* c = app.GetCollider(e)) eSize = c->size;

                    const Vector2 eCenter{ ePos.x + eSize.x * 0.5f, ePos.y + eSize.y * 0.5f };
                    const float dx = pCenter.x - eCenter.x;
                    const float dy = pCenter.y - eCenter.y;
                    const float d2 = dx * dx + dy * dy;
                    if (d2 <= r2) { enemyInRange = true; break; }
                }

                if (!s_firstMonsterEncounterTriggered && enemyInRange) s_firstMonsterEncounterTriggered = true;

                if (s_firstMonsterEncounterTriggered) {
                    if (!enemyInRange) {
                        s_firstMonsterEncounterCompleted = true;
                    }
                    else if (!text.empty()) {
                        out.show = true;
                        out.text = text;
                        out.font = s_firstMonsterEncounter.font;
                        out.sizePx = s_firstMonsterEncounter.sizePx;
                        out.rgba = s_firstMonsterEncounter.rgba;
                        return out;
                    }
                }
            }
        }

        {
            if (!s_firstKeyDropCompleted && s_hasFirstKeyDrop) {
                const std::string text = !s_firstKeyDrop.text.empty() ? s_firstKeyDrop.text : s_firstKeyDrop.firstText;
                const float range = (s_firstKeyDrop.range > 0.0f) ? s_firstKeyDrop.range : baseRange;
                const float r2 = range * range;

                if (!s_firstKeyDropTriggered) {
                    for (const auto& kv : app.GetAllColliders()) {
                        const Entity e = kv.first;
                        const std::string tag = app.GetPrefabTag(e);
                        if (tag != "Key_Projectile") continue;
                        s_firstKeyDropTriggered = true;
                        s_firstKeyEntity = e;
                        if (Transform* kt = app.GetTransform(e)) {
                            Vector2 kPos = kt->GetPosition();
                            Vector2 kSize = kt->GetScale();
                            if (Collider* kc = app.GetCollider(e)) kSize = kc->size;
                            s_firstKeyDropOrigin = { kPos.x + kSize.x * 0.5f, kPos.y + kSize.y * 0.5f };
                            s_firstKeyDropOriginSet = true;
                        }
                        break;
                    }
                }

                if (s_firstKeyDropTriggered) {
                    if (s_firstKeyDropOriginSet) {
                        const float dx = pCenter.x - s_firstKeyDropOrigin.x;
                        const float dy = pCenter.y - s_firstKeyDropOrigin.y;
                        const float d2 = dx * dx + dy * dy;

                        if (d2 <= r2) {
                            s_firstKeyDropSeen = true;
                            if (!text.empty()) {
                                out.show = true;
                                out.text = text;
                                out.font = s_firstKeyDrop.font;
                                out.sizePx = s_firstKeyDrop.sizePx;
                                out.rgba = s_firstKeyDrop.rgba;
                                return out;
                            }
                        }
                        else if (s_firstKeyDropSeen) {
                            s_firstKeyDropCompleted = true;
                        }
                    }
                }
            }
        }

        {
            if (!s_secondKeyDropCompleted && s_hasSecondKeyDrop) {
                const std::string text = !s_secondKeyDrop.text.empty() ? s_secondKeyDrop.text : s_secondKeyDrop.firstText;
                const float range = (s_secondKeyDrop.range > 0.0f) ? s_secondKeyDrop.range : baseRange;
                const float r2 = range * range;

                if (!s_secondKeyDropTriggered) {
                    for (const auto& kv : app.GetAllColliders()) {
                        const Entity e = kv.first;
                        const std::string tag = app.GetPrefabTag(e);
                        if (tag != "Key_Burrow") continue;
                        s_secondKeyDropTriggered = true;
                        s_secondKeyEntity = e;
                        if (Transform* kt = app.GetTransform(e)) {
                            Vector2 kPos = kt->GetPosition();
                            Vector2 kSize = kt->GetScale();
                            if (Collider* kc = app.GetCollider(e)) kSize = kc->size;
                            s_secondKeyDropOrigin = { kPos.x + kSize.x * 0.5f, kPos.y + kSize.y * 0.5f };
                            s_secondKeyDropOriginSet = true;
                        }
                        break;
                    }
                }

                if (s_secondKeyDropTriggered) {
                    if (s_secondKeyDropOriginSet) {
                        const float dx = pCenter.x - s_secondKeyDropOrigin.x;
                        const float dy = pCenter.y - s_secondKeyDropOrigin.y;
                        const float d2 = dx * dx + dy * dy;

                        if (d2 <= r2) {
                            s_secondKeyDropSeen = true;
                            if (!text.empty()) {
                                out.show = true;
                                out.text = text;
                                out.font = s_secondKeyDrop.font;
                                out.sizePx = s_secondKeyDrop.sizePx;
                                out.rgba = s_secondKeyDrop.rgba;
                                return out;
                            }
                        }
                        else if (s_secondKeyDropSeen) {
                            s_secondKeyDropCompleted = true;
                        }
                    }
                }
            }
        }

        {
            if (!s_thirdKeyDropCompleted && s_hasThirdKeyDrop) {
                const std::string text = !s_thirdKeyDrop.text.empty() ? s_thirdKeyDrop.text : s_thirdKeyDrop.firstText;
                const float range = (s_thirdKeyDrop.range > 0.0f) ? s_thirdKeyDrop.range : baseRange;
                const float r2 = range * range;

                if (!s_thirdKeyDropTriggered) {
                    for (const auto& kv : app.GetAllColliders()) {
                        const Entity e = kv.first;
                        const std::string tag = app.GetPrefabTag(e);
                        if (tag != "Key_Heal") continue;
                        s_thirdKeyDropTriggered = true;
                        s_thirdKeyEntity = e;
                        if (Transform* kt = app.GetTransform(e)) {
                            Vector2 kPos = kt->GetPosition();
                            Vector2 kSize = kt->GetScale();
                            if (Collider* kc = app.GetCollider(e)) kSize = kc->size;
                            s_thirdKeyDropOrigin = { kPos.x + kSize.x * 0.5f, kPos.y + kSize.y * 0.5f };
                            s_thirdKeyDropOriginSet = true;
                        }
                        break;
                    }
                }

                if (s_thirdKeyDropTriggered) {
                    if (s_thirdKeyDropOriginSet) {
                        const float dx = pCenter.x - s_thirdKeyDropOrigin.x;
                        const float dy = pCenter.y - s_thirdKeyDropOrigin.y;
                        const float d2 = dx * dx + dy * dy;

                        if (d2 <= r2) {
                            s_thirdKeyDropSeen = true;
                            if (!text.empty()) {
                                out.show = true;
                                out.text = text;
                                out.font = s_thirdKeyDrop.font;
                                out.sizePx = s_thirdKeyDrop.sizePx;
                                out.rgba = s_thirdKeyDrop.rgba;
                                return out;
                            }
                        }
                        else if (s_thirdKeyDropSeen) {
                            s_thirdKeyDropCompleted = true;
                        }
                    }
                }
            }
        }

        {
            if (!s_generatorRoom1Completed && s_hasGeneratorRoom1) {
                const float range = (s_generatorRoom1.range > 0.0f) ? s_generatorRoom1.range : baseRange;
                const float r2 = range * range;

                const GLuint genOffTex = ResourceManager::GetTexture("PuzzleGenerator");
                GLuint genOnTex = ResourceManager::GetTexture("PuzzleGenerator_On");
                if (genOnTex == 0) genOnTex = ResourceManager::GetTexture("PuzzleGenerator_On");

                if (!s_generatorRoom1Triggered) {
                    Entity chosen = INVALID_ENTITY;
                    Vector2 chosenCenter{};
                    float bestD2 = 1e12f;

                    for (const auto& kv : app.GetPuzzleObjects()) {
                        const Entity e = kv.first;
                        const PuzzleObject& po = kv.second;
                        if (po.kind != PuzzleKind::ShootTarget) continue;

                        const std::string bakedName = app.GetEntityNameByEntity(e);
                        if (bakedName.find("entities_Level1") == std::string::npos) continue;

                        MeshRenderer* mr = app.GetRenderer(e);
                        if (!mr) continue;
                        const GLuint tex = mr->GetTexture();
                        if (tex == 0) continue;
                        if (tex != genOffTex && tex != genOnTex) continue;

                        Transform* t = app.GetTransform(e);
                        if (!t) continue;
                        Vector2 gPos = t->GetPosition();
                        Vector2 gSize = t->GetScale();
                        if (Collider* c = app.GetCollider(e)) gSize = c->size;

                        const Vector2 gCenter{ gPos.x + gSize.x * 0.5f, gPos.y + gSize.y * 0.5f };
                        const float dxp = pCenter.x - gCenter.x;
                        const float dyp = pCenter.y - gCenter.y;
                        const float d2p = dxp * dxp + dyp * dyp;
                        if (d2p > r2) continue;

                        bool enemiesRemain = false;
                        const float eR2 = s_generatorRoom1EnemyRadius * s_generatorRoom1EnemyRadius;
                        for (const auto& ck : app.GetAllColliders()) {
                            const Entity en = ck.first;
                            if (!app.GetEnemyController(en)) continue;
                            Transform* et = app.GetTransform(en);
                            if (!et) continue;

                            Vector2 ePos = et->GetPosition();
                            Vector2 eSize = et->GetScale();
                            if (Collider* ec = app.GetCollider(en)) eSize = ec->size;
                            const Vector2 eCenter{ ePos.x + eSize.x * 0.5f, ePos.y + eSize.y * 0.5f };
                            const float dx = eCenter.x - gCenter.x;
                            const float dy = eCenter.y - gCenter.y;
                            const float d2 = dx * dx + dy * dy;
                            if (d2 <= eR2) { enemiesRemain = true; break; }
                        }
                        if (enemiesRemain) continue;

                        if (d2p < bestD2) {
                            bestD2 = d2p;
                            chosen = e;
                            chosenCenter = gCenter;
                        }
                    }

                    if (chosen != INVALID_ENTITY) {
                        s_generatorRoom1Triggered = true;
                        s_generatorRoom1Timer = 0.0f;
                        s_generatorRoom1SecondSeen = false;
                        s_generatorRoom1Origin = chosenCenter;
                        s_generatorRoom1OriginSet = true;
                    }
                }

                if (s_generatorRoom1Triggered && s_generatorRoom1OriginSet) {
                    s_generatorRoom1Timer += (float)eng::deltaTime();

                    const float dx = pCenter.x - s_generatorRoom1Origin.x;
                    const float dy = pCenter.y - s_generatorRoom1Origin.y;
                    const float d2 = dx * dx + dy * dy;

                    const bool inRange = (d2 <= r2);
                    const bool secondPhase = (s_generatorRoom1Timer >= s_generatorRoom1DelaySec);

                    if (inRange) {
                        const std::string text = secondPhase ? s_generatorRoom1.repeatText : s_generatorRoom1.firstText;
                        if (secondPhase) s_generatorRoom1SecondSeen = true;
                        if (!text.empty()) {
                            out.show = true;
                            out.text = text;
                            out.font = s_generatorRoom1.font;
                            out.sizePx = s_generatorRoom1.sizePx;
                            out.rgba = s_generatorRoom1.rgba;
                            return out;
                        }
                    }
                    else if (s_generatorRoom1SecondSeen) {
                        s_generatorRoom1Completed = true;
                    }
                }
            }
        }

        {
            if (s_hasRoom5Sequence && s_room5SequenceIndex < (int)s_room5Sequence.size()) {
                bool inRoom5 = false;
                for (const auto& kv : app.GetAllColliders()) {
                    const Entity e = kv.first;
                    const std::string name = app.GetEntityNameByEntity(e);
                    if (name.find("entities_Level5") != std::string::npos) { inRoom5 = true; break; }
                }
                if (inRoom5) {
                    const Room5SequenceStep& step = s_room5Sequence[s_room5SequenceIndex];
                    const bool isLastRoom5Step = (s_room5SequenceIndex == (int)s_room5Sequence.size() - 1);

                    Vector2 stepCenter{};
                    if (step.hasWorld) {
                        stepCenter = { step.wx, step.wy };
                    }
                    else {
                        const float tileSize = (s_room5SequenceTileSize > 0.0f) ? s_room5SequenceTileSize : 100.0f;
                        stepCenter = { (step.x + 0.5f) * tileSize, (step.y + 0.5f) * tileSize };
                    }
                    float range = step.range;
                    if (range <= 0.0f) range = (s_room5SequenceStyle.range > 0.0f) ? s_room5SequenceStyle.range : baseRange;
                    const float r2 = range * range;
                    const float dx = pCenter.x - stepCenter.x;
                    const float dy = pCenter.y - stepCenter.y;
                    const float d2 = dx * dx + dy * dy;

                    if (d2 <= r2) {
                        s_room5SequenceSeen = true;
                        if (isLastRoom5Step && !s_room5EndCutsceneRequested) {
                            const double now = glfwGetTime();
                            if (s_room5EndCutsceneStartTime < 0.0) s_room5EndCutsceneStartTime = now;
                            if (now - s_room5EndCutsceneStartTime >= 3.0) s_room5EndCutsceneRequested = true;
                        }
                        if (!step.text.empty()) {
                            out.show = true;
                            out.text = step.text;
                            out.font = s_room5SequenceStyle.font;
                            out.sizePx = s_room5SequenceStyle.sizePx;
                            out.rgba = s_room5SequenceStyle.rgba;
                            return out;
                        }
                    }
                    else if (s_room5SequenceSeen) {
                        s_room5SequenceIndex += 1;
                        s_room5SequenceSeen = false;
                        if (isLastRoom5Step && !s_room5EndCutsceneRequested) s_room5EndCutsceneStartTime = -1.0;
                    }
                }
            }
        }

        Entity corpse = Interaction::GetFirstAbsorbCorpse();
        if (corpse != INVALID_ENTITY) {
            Transform* ct = app.GetTransform(corpse);
            if (!ct) {
                Interaction::ClearFirstAbsorbCorpseIfMatch(corpse);
            }
            else {
                Vector2 cPos = ct->GetPosition();
                Vector2 cSize = ct->GetScale();
                if (Collider* cc = app.GetCollider(corpse)) cSize = cc->size;
                const Vector2 cCenter{ cPos.x + cSize.x * 0.5f, cPos.y + cSize.y * 0.5f };
                const float dx = pCenter.x - cCenter.x;
                const float dy = pCenter.y - cCenter.y;
                const float d2 = dx * dx + dy * dy;

                ProximityPromptSpec spec = GetPromptForFirstAbsorb();
                const float range = (spec.range > 0.0f) ? spec.range : baseRange;
                const float r2 = range * range;
                const std::string text = !spec.text.empty() ? spec.text : spec.firstText;

                if (d2 <= r2 && !text.empty()) {
                    out.show = true;
                    out.text = text;
                    out.font = spec.font;
                    out.sizePx = spec.sizePx;
                    out.rgba = spec.rgba;
                    return out;
                }
            }
        }

        Entity best = INVALID_ENTITY;
        PuzzleKind bestKind = PuzzleKind::None;
        float bestDist2 = 1e12f;

        std::unordered_set<Entity> promptableNow;
        for (const auto& kv : app.GetPuzzleObjects()) {
            Entity e = kv.first;
            const PuzzleObject& po = kv.second;

            ProximityPromptSpec spec = GetPromptForEntitiesKind(po.kind);
            if (spec.text.empty() && spec.firstText.empty() && spec.repeatText.empty()) continue;
            promptableNow.insert(e);
            if (po.kind == PuzzleKind::Lever || po.kind == PuzzleKind::Switch || po.kind == PuzzleKind::HealingMemory) {
                if (po.active) s_dismissedByActive.insert(e);
                if (s_dismissedByActive.find(e) != s_dismissedByActive.end()) continue;
            }

            Transform* t = app.GetTransform(e);
            if (!t) continue;
            Vector2 ePos = t->GetPosition();
            Vector2 eSize = t->GetScale();
            if (Collider* c = app.GetCollider(e)) eSize = c->size;

            const Vector2 eCenter{ ePos.x + eSize.x * 0.5f, ePos.y + eSize.y * 0.5f };
            const float dx = pCenter.x - eCenter.x;
            const float dy = pCenter.y - eCenter.y;
            const float d2 = dx * dx + dy * dy;

            const float range = (spec.range > 0.0f) ? spec.range : baseRange;
            const float r2 = range * range;

            if (d2 <= r2 && d2 < bestDist2) {
                best = e;
                bestKind = po.kind;
                bestDist2 = d2;
            }
        }

        for (auto it = s_completedFirstEncounter.begin(); it != s_completedFirstEncounter.end(); ) {
            if (promptableNow.find(*it) == promptableNow.end()) it = s_completedFirstEncounter.erase(it);
            else ++it;
        }
        for (auto it = s_dismissedByActive.begin(); it != s_dismissedByActive.end(); ) {
            if (promptableNow.find(*it) == promptableNow.end()) it = s_dismissedByActive.erase(it);
            else ++it;
        }

        if (best == INVALID_ENTITY) {
            if (s_lastProximityEntity != INVALID_ENTITY) {
                s_completedFirstEncounter.insert(s_lastProximityEntity);
                s_lastProximityEntity = INVALID_ENTITY;
            }
            return out;
        }

        if (s_lastProximityEntity != INVALID_ENTITY && s_lastProximityEntity != best) {
            s_completedFirstEncounter.insert(s_lastProximityEntity);
        }
        s_lastProximityEntity = best;

        ProximityPromptSpec spec = GetPromptForEntitiesKind(bestKind);
        const bool isRepeat = (s_completedFirstEncounter.find(best) != s_completedFirstEncounter.end());
        std::string text = isRepeat ? spec.repeatText : spec.firstText;
        if (text.empty()) text = spec.text;
        if (text.empty()) return out;

        if (bestKind == PuzzleKind::BurrowFloor || bestKind == PuzzleKind::BurrowWall) {
            if (PlayerController* pc = app.GetController(player)) {
                if (pc->IsBurrowAnimating()) return out;
            }
        }

        out.show = true;
        out.text = text;
        out.font = spec.font;
        out.sizePx = spec.sizePx;
        out.rgba = spec.rgba;
        return out;
    }

    bool ConsumeEndCutsceneRequest()
    {
        const bool v = s_room5EndCutsceneRequested;
        if (v) {
            s_room5EndCutsceneRequested = false;
            s_room5EndCutsceneStartTime = -1.0;
        }
        return v;
    }
}

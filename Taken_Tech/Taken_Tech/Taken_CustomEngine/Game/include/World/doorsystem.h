/*
 * @file      doorsystem.h
 * @author    Jethro Sung
 * @co-author Woh Kye Le
 * @email     sung.h, w.kyele
 * @brief Declares the DoorSystem class for inter-scene door transitions.
 *
 * Each door is represented by a DoorLink struct that stores its entity,
 * target scene, target door, and arrival direction. DoorSystem handles
 * detection, scene loading, and repositioning of the player between scenes.
*/
#pragma once
#include "Core/entitymanager.h"
#include "Math/vect2.h"
#include <string>
#include <vector>
#include <functional> 
#include <unordered_set> 


class GameApp; // forward declare to avoid circular dependency

/**
 * @brief Possible facing direction when the player arrives through a door.
 */
enum class DoorArrivalDir { None, Top, Bottom, Left, Right };

/**
 * @brief Represents a door's link and teleport metadata.
 */
struct DoorLink {
	Entity entity = 0;              ///< Door entity ID
	std::string name{};             ///< Door name identifier
	std::string targetScene{};      ///< Target scene file path
	std::string targetDoor{};       ///< Target door name in the destination scene
	DoorArrivalDir arrivalDir = DoorArrivalDir::None; ///< Direction player appears when entering
};

/** 
 * @class DoorSystem
 * @brief Manages door registration, collision checks, and scene transitions.
 * 
 * Handles the logic for detecting when a player enters a door, loading the
 * destination scene, and positioning the player at the correct spawn point.
 */
class DoorSystem {
public:
	using RequestSceneFn = std::function<void(const std::string& sceneFile, const std::string& spawnDoorName)>;

    /**
     * @brief Sets the callback function for requesting a scene load.
     * @param fn The callback function.
     */
	void SetRequestSceneFn(RequestSceneFn fn) { requestScene = std::move(fn);}

    /**
     * @brief Queue a door spawn for later processing.
     * 
     * Sets up a pending door spawn that will be processed after a delay.
     * 
     * @param doorName Name of the door to spawn the player at.
     * @param delayFrames Number of frames to wait before spawning.
     */
	void SetPendingSpawnDoor(const std::string& doorName, int delayFrames = 2);

	/**
     * @brief Registers a new door link in the system.
     * @param link The door link data.
     */
    void RegisterDoor(const DoorLink& link);

    /**
     * @brief Removes door links associated with specific entities.
     * @param doomed Set of entity IDs that are being destroyed.
     */
	void UnregisterDoorsForEntities(const std::unordered_set<Entity>& doomed);

	/**
     * @brief Clears all registered doors.
     */
    void ClearDoors();

    /**
     * @brief Clears the list of entered doors.
     */
    void ClearEnteredDoors() { enteredDoors.clear(); }

	/**
     * @brief Checks for player-door collisions and initiates transitions.
     * @param app Reference to the GameApp.
     */
    void CheckDoorTransitions(GameApp& app);

	/**
     * @brief Updates door textures based on unlocked status.
     * 
     * Checks player keys and changes door visuals (e.g., locked vs unlocked).
     * @param app Reference to the GameApp.
     */
	void UpdateDoorTextures(GameApp& app);

	/**
	 * @brief Renders action hints beside unlocked, unentered doors.
	 * @param renderer Reference to the Renderer.
	 * @param app Reference to the GameApp.
	 */
	void DrawActionHints(class Renderer& renderer, GameApp& app);

	/**
     * @brief Handles pending player spawn after scene load.
     * 
     * Teleports player to the linked door position.
     * @param app Reference to the GameApp.
     */
    void HandlePendingSpawn(GameApp& app);

	/**
     * @brief Updates cooldown timer to prevent rapid re-triggering.
     */
    void UpdateCooldown();

	/**
     * @brief Immediately teleports player to specified door.
     * 
     * Used for Editor switching and testing.
     * 
     * @param app Reference to the GameApp.
     * @param doorName Name of the target door.
     * @param playSound Whether to play the door sound.
     */
	void SnapPlayerToDoor(GameApp& app, const std::string& doorName, bool playSound = false);
	/**
	 * @brief Gets a read-only list of all registered door links.
	 * @return A constant reference to the vector of DoorLink structures.
	 */
	const std::vector<DoorLink>& GetDoorLinks() const { return doorLinks; }

	/**
     * @brief Clears all non-persistent runtime doors.
     * 
     * Used by editor rebuilds to reset state.
     */
	void ClearNonPersistentDoors();

private:
	std::vector<DoorLink> doorLinks; ///< Registered doors
	std::unordered_set<std::string> enteredDoors; ///< Set of doors entered by player
	bool isTransitioning = false;    ///< True if a transition is in progress

	/**
     * @brief Pending spawn data after scene load.
     */
    struct PendingSpawn {
		bool active = false;          ///< True if a spawn is pending
		int delayFrames = 2;          ///< Frames to wait before spawning
		std::string doorName;         ///< Target door name to spawn at
		std::string scene;            ///< Target scene file path
    } pendingSpawn;

	float cooldown = 0.0f;           ///< Cooldown timer to prevent rapid re-triggering
	float totalTime = 0.0f;          ///< Total time for animation

	RequestSceneFn requestScene;     ///< Callback to load a scene
};

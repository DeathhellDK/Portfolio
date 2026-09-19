/**
* @file IPhysicsProxy.hpp
* @author Tan Wei Liang Terril
* @email t.weiliangterril
* @date 2025-10-21
*
* @brief Declaration of the IPhysicsProxy interface, which defines
* a combie access layer for retrieving and interacting with 
* physics-related components in the ECS
*
* @version 1.0
* - Declared an abstract interface with virtual methods for
* accessing SpeedComponent, Transform and entity collections.
*
* @copyright Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/

#ifndef IPHYSICSPROXY
#define IPHYSICSPROXY

#include "Core/component.h"
#include "PhysicsSpeedComponent.hpp"

class Transform; /// Forward declaration to Transform class

/**
* @class IphysicsProxy
* 
* @brief Abstract interface providing access to physics-related entity data
*/
class IPhysicsProxy {
public:

    /**
    * @brief Virtual destructor for safe cleanup in derived proxy classes
    */
    virtual ~IPhysicsProxy() = default;
    
    /**
    * @brief Checks whether the specificed entity has a SpeedComponent
    * 
    * @param e Entity identifier 
    */
    virtual bool hasSpeed(Entity e) const = 0;

    /**
     * @brief Checks whether the specificed entity has a Transform component
     *
     * @param e Entity identifier
     */
    virtual bool hasTransform(Entity e) const = 0;

    /**
    * @brief Get the SpeedComponent of the specified entity
    *
    * @param e Entity identifier
    */
    virtual SpeedComponent* getSpeed(Entity e) = 0;

    /**
    * @brief Get the Transform component of the specified entity
    * 
    * @param e Entity identified
    */
    virtual Transform* getTransform(Entity e) = 0;

    /**
    * @brief Get a constant reference to the list of all entities
    */
    virtual const std::vector<Entity>& getEntities() const = 0;
}

#endif
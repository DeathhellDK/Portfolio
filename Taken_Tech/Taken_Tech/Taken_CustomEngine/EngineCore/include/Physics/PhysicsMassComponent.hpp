/**
* @file PhysicsMassComponent.hpp
* @author Tan Wei Liang Terril
* @email t.weiliangterril
* @date 2025-10-30
*
* @brief Header file for physics masss component that will be used to
* store mass information for the entity,
*
* @version 1.0
*
* @copyright Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/

#ifndef PHYSICS_MASS_COMPONENT_HPP
#define PHYSICS_MASS_COMPONENT_HPP

#include "Math/vect2.h"

/**
* @struct MassComponent
* @brief A component that holds mass information
* This component contains a float representing the mass and a boolean flag indicating whether the entity is dynamic.
*/
struct MassComponent : public Component {
	float mass{ 1.0f };
	bool isDynamic;

	/**
	* @brief This is a custom default contructor that initialises the component
	* with an invalid entity (empty component).
	*
	* This is needed as it does not work with the base component default constructor.
	*/
	MassComponent() : 
		Component(INVALID_ENTITY), mass(1.0f), isDynamic(true) {}
	
	/**
	* @brief Constructs a MassComponent with the specified owner entity, initial mass, and dynamic flag.
	*/
	explicit MassComponent(Entity ownerId, float initialMass = 1.0f,
						   bool dynamic = true)
		: Component(ownerId), mass(initialMass), isDynamic(dynamic) {
	}
};

#endif // !PHYSICS_MASS_COMPONENT_HPP
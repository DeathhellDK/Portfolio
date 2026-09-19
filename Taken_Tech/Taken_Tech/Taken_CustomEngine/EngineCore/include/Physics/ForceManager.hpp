/**
* @file ForceManager.hpp
* @author Tan Wei Liang Terril
* @email t.weiliangterril
* @date 2025-10-27
*
* @brief Declaration of ForceManager class which is responsible for updating
* and managing physics forces through a ForceProxy interface
*
* @version 1.0
* - Created a class ForceManager class and its fuctions that will acts
* as an intermediary between the physics system and the indiviual entities 
* force components.
*
* @copyright Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/

#ifndef FORCE_MANAGER_HPP
#define FORCE_MANAGER_HPP

#include "Physics/ForceProxy.hpp"

/**
* @class ForceManager
* 
* @brief Handles the management and updating of forces
* in the physics system.
*/
class ForceManager {
	public:
		/**
		* @breif Constructs a ForceManager with a reference to a ForceProxy
		* 
		* @param proxy Reference to the ForceProxy that handles physics force data.
		*/
		explicit ForceManager(ForceProxy& proxy)
			: forceProxy(proxy) {
		}

		/**
		* @brief Updates all active forces using the given delta time
		* 
		* @note This function will runs once per frame, ensuring that all
		* forces are recalulated and applied correctly based on elapsed time.
		* 
		* @param dt Used for physics calulations.
		*/
		void Update(float dt);
	
	private:
		/// Reference to the ForceProxy that managing physics forces.
		ForceProxy& forceProxy;
};
#endif // !FORCE_MANAGER_HPP

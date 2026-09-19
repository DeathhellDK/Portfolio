/**
* @file     message_system.h
* @author   Lim Zhi Jie
* @email    zhijie.lim
* @co-author
* @email
* @date     2025-09-29
*
* @brief Header file for Messaging namespace with classes: IMessage,
* Observer, Observable. EnemyAttackMessage, PlayerAttackMessage and
* PlayerDamageMessage are child class of IMessage
* 
* This system allows different parts of the engine (Observers) to subscribe to
* messages published by other parts (Observables). When an Observable receives
* a message, it forwards it to all registered Observers, which can then handle
* it via their attached callback functions.
*
* @version 1.0
* @copyright Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/

#pragma once
#include <string>
#include <map>
#include <functional>
#include "Math/vect2.h"

namespace Messaging {
	class IMessage {
	public:	
		IMessage(const std::string);
		virtual ~IMessage();
		std::string GetId() const noexcept;

	private:
		std::string m_id;
	};

	typedef void(*MESSAGE_HANDLER)(IMessage*);

	class Observer {
	public:
		Observer(const std::string& id);
		~Observer();

		void AttachHandler(const std::string& messageId, MESSAGE_HANDLER handler);
		MESSAGE_HANDLER GetHandler(const std::string& messageId) const noexcept;

		std::string GetId() const noexcept;

	private:
		std::string m_id;
		std::multimap<std::string, MESSAGE_HANDLER> m_handlers;
	};

	class Observable
	{
	public:
		Observable() : m_id("Observable") {}
		Observable(const std::string& id);
		~Observable();

		void Register(const std::string& messageId, Observer* observer);
		void ProcessMessage(IMessage* message);

		std::string GetId() const noexcept;

	private:
		std::string m_id;
		std::multimap<std::string, Observer*> m_observers;
	};

	// Game Specific Messages
	// Enemy attacking player
	class EnemyAttackMessage : public IMessage {
	public:
		EnemyAttackMessage(int enemyId, int damage)
			: IMessage("ENEMY_ATTACK"), enemyId(enemyId), damage(damage) {
		}

		int enemyId;
		int damage;
	};

	// Player attacking enemy
	class PlayerAttackMessage : public IMessage {
	public:
		PlayerAttackMessage(int playerId, int damage)
			: IMessage("PLAYER_ATTACK"), playerId(playerId), damage(damage) {
		}

		int playerId;
		int damage;
	};

	// Player taking damage
	class PlayerDamageMessage : public IMessage {
	public:
		PlayerDamageMessage(int playerId, int damageTaken)
			: IMessage("PLAYER_DAMAGE"), playerId(playerId), damageTaken(damageTaken) {
		}

		int playerId;
		int damageTaken;
	};

	class EnemySpottedPlayerMessage : public Messaging::IMessage {
	public:
		int entityId;
		Vector2 enemyPos;
		Vector2 playerPos;
		int mobType;

		EnemySpottedPlayerMessage(int id, Vector2 ePos, Vector2 pPos, int type)
			: IMessage("EnemySpottedPlayer"),
			entityId(id), enemyPos(ePos), playerPos(pPos), mobType(type) {
		}
	};
}
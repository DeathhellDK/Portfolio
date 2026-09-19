/**
* @file     message_system.cpp
* @author   Lim Zhi Jie
* @email    zhijie.lim,t.weiliangterril
* @co-author Tan Wei Liang Terril
* @date     2025-09-29
*
* @brief Defines the behavior for sending, receiving, and handling messages between
* Observables and Observers. This allows systems to communicate without being
* tightly coupled, following the Observer design pattern.
*
* @version 1.0
* @copyright Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/

#include "Input/message_system.h"
#include "Input/DebugConsole.hpp"
#include <iostream>

namespace Messaging {

    // IMessage
    
    // Constructor
    /**
        * @brief Constructs a message with the specified identifier.
        *
        * The identifier is used by Observables and Observers to determine
        * routing and handler selection.
        *
        * @param id String identifying the message type.
    */
    IMessage::IMessage(const std::string id) : m_id(id){}
    
    // Destructor
    IMessage::~IMessage() = default;

    // Gets the id of message
    std::string IMessage::GetId() const noexcept {
        return m_id;
    }
    
    // Observer
    
    // Constructor
    Observer::Observer(const std::string& id) : m_id(id) {}

    // Destructor
    Observer::~Observer() = default;

    // Attach handler function to a specific message id
    void Observer::AttachHandler(const std::string& messageId, MESSAGE_HANDLER handler) {
        m_handlers.insert({ messageId, handler });
    }

    // Gets the handler for a specific message id
    MESSAGE_HANDLER Observer::GetHandler(const std::string& messageId) const noexcept {
        auto it = m_handlers.find(messageId);
        if (it != m_handlers.end()) {
            return it->second;
        }
        return nullptr; // no handler found
    }

    // Gets the id of the observer
    std::string Observer::GetId() const noexcept {
        return m_id;
    }


    // Observable

    // Constructor
    Observable::Observable(const std::string& id) : m_id(id) {}


    // Destructor
    Observable::~Observable() = default;

    /**
        * @brief Registers an observer to listen for a specific message type.
        *
        * Multiple observers may register for the same message ID.
        *
        * @param messageId ID of the message to listen for.
        * @param observer Pointer to the observing listener.
    */
    void Observable::Register(const std::string& messageId, Observer* observer) {
        m_observers.insert({ messageId, observer });
    }

    /**
        * @brief Dispatches a message to all observers registered for its ID.
        *
        * Workflow:
        *  - Logs receipt of the message.
        *  - Retrieves all observers associated with message->GetId().
        *  - Invokes each observer’s handler if registered.
        *  - Logs handler execution or missing handlers.
        *
        * @param message Pointer to the message instance being processed.
    */
    void Observable::ProcessMessage(IMessage* message) {
        DebugConsole::Get().AddFormattedMessage(LogLevel::Info, "[MESSAGE] Observable: " + m_id + " processing message: " + message->GetId(), "\n");

        auto range = m_observers.equal_range(message->GetId());
        for (auto it = range.first; it != range.second; ++it) {
            Observer* obs = it->second;
            auto handler = obs->GetHandler(message->GetId());
            if (handler) {
                DebugConsole::Get().AddFormattedMessage(LogLevel::Info, "[MESSAGE] Handled by Observer: " + obs->GetId(), "\n");
                handler(message);
            }
            else {
                DebugConsole::Get().AddFormattedMessage(LogLevel::Warning, "[MESSAGE] Observer " + obs->GetId() + " has no handler for " + message->GetId(), "\n");
            }
        }
    }

    // Get id of observerable
    std::string Observable::GetId() const noexcept {
        return m_id;
    }

}

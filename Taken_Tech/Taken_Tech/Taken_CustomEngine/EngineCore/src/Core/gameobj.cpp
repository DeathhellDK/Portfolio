//#include "Core/gameobj.h"
//#include <iostream>
//#include <Core/transform.h>
//#include "Input/DebugConsole.hpp"
//
//void GameObject::Update(float dt) {
//    for (auto& c : components) {
//        c->Update(dt);
//    }
//}
//
//void GameObject::Draw(Renderer& renderer) {
//    // Here we expect a Transform component to provide the model matrix.
//    // For now, we just warn if it's missing.
//    auto transform = GetComponent<class Transform>();
//    if (!transform) {
//        DebugConsole::Get().AddFormattedMessage(LogLevel::Warning,
//            "GameObject '", name,
//			"' has no Transform component; skipping Draw().\n");    
//        return;
//    }
//
//    Matrix3x3 model = transform->GetMatrix();
//    for (auto& c : components) {
//        c->Draw(renderer, model);
//    }
//}
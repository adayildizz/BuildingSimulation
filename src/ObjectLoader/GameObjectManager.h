#ifndef GAME_OBJECT_MANAGER_H
#define GAME_OBJECT_MANAGER_H

#include <vector>
#include <iostream>
#include "GameObject.h"
#include "../Core/Shader.h" // Include Shader for RenderAll signature

class GameObjectManager{
public:
    GameObjectManager();
    ~GameObjectManager();
    int CreateNewObject(ObjectLoader &objectLoader);
    GameObject* GetGameObject(int index);
    void RenderAll(Shader& shader); // Modified to accept a shader

private:
    std::vector<GameObject*> gameObjects;
};


#endif // GAME_OBJECT_MANAGER_H

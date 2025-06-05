
#ifndef GAME_OBJECT_MANAGER_H
#define GAME_OBJECT_MANAGER_H
#include "GameObject.h"
class GameObjectManager{

    public:
        GameObjectManager();
        ~GameObjectManager();
        int CreateNewObject(ObjectLoader &objectLoader);
        GameObject* GetGameObject(int index);
        void RenderAll();
        void RenderAllForDepthPass(Shader& depthShader);

    private:
        std::vector<GameObject*> gameObjects;
};


#endif GAME_OBJECT_MANAGER_H

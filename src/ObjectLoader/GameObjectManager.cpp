
#include "GameObjectManager.h"
#include "GameObject.h"
#include "ObjectLoader.h"
GameObjectManager::GameObjectManager(){

}


int GameObjectManager::CreateNewObject(ObjectLoader &objectLoader){
    GameObject* gameObject = new GameObject(objectLoader);
    gameObjects.push_back(gameObject);
    return gameObjects.size()-1;
}


GameObject* GameObjectManager::GetGameObject(int index){
    if (index >= 0 && static_cast<size_t>(index) < gameObjects.size()) {
        return gameObjects[index];
    } else {
        std::cout << "Index Out of Bounds for index " << index << std::endl;
        return nullptr; // Return nullptr if index is invalid
    }
}

void GameObjectManager::RenderAll(){
    for(int i = 0; i<gameObjects.size();i++){
        gameObjects[i]->Render();
    }
}

void GameObjectManager::RenderAllForDepthPass(Shader& depthShader) {
    for(GameObject* obj : gameObjects) {
        if (obj) { // Check if object is valid
            obj->RenderForDepthPass(depthShader);
        }
    }
}

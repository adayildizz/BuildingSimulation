#include "GameObjectManager.h"
#include "GameObject.h"
#include "ObjectLoader.h"

GameObjectManager::GameObjectManager(){

}

GameObjectManager::~GameObjectManager(){
    // Proper cleanup of dynamically allocated GameObjects
    for(GameObject* go : gameObjects){
        delete go;
    }
    gameObjects.clear();
}

int GameObjectManager::CreateNewObject(ObjectLoader &objectLoader){
    GameObject* gameObject = new GameObject(objectLoader);
    gameObjects.push_back(gameObject);
    return gameObjects.size()-1;
}

GameObject* GameObjectManager::GetGameObject(int index){
    if (index >= 0 && index < gameObjects.size()) {
        return gameObjects[index];
    }
    
    std::cout << "Index Out of Bounds for index " << index << std::endl;
    return nullptr;
}

void GameObjectManager::RenderAll(Shader& shader){
    for(int i = 0; i < gameObjects.size(); i++){
        gameObjects[i]->Render(shader);
    }
}

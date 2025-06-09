
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
    try{
        return gameObjects[index];
    }catch(std::exception& ex){
        std::cout << "Index Out of Bounds for index " << index << std::endl;
    }
}

void GameObjectManager::RenderAll(){
    for(int i = 0; i<gameObjects.size();i++){
        gameObjects[i]->Render();
    }
}
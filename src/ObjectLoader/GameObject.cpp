
#include "GameObject.h"
#include "Angel.h"

GameObject::GameObject(ObjectLoader& objectLoader){
    this->objectLoader = &objectLoader;
    position = vec4(0,0,0,1);
    angle = 0;
    scale = 1.0f;
    UpdateModelMatrix();
    isInPlacement = true;
}

GameObject::~GameObject(){
    // Destructor - objectLoader is not owned by GameObject, so don't delete it
}

vec4 GameObject::GetPosition(){
    return position;
}

void GameObject::SetPosition(vec4 newPosition){
    this->position = newPosition;
    UpdateModelMatrix();
}

void GameObject::Move(vec4 deltaPosition){
    // Relative movement - add to current position
    this->position += deltaPosition;
    UpdateModelMatrix();
}

void GameObject::Rotate(float deltaAngle){
    // Relative rotation - add to current angle
    this->angle += deltaAngle;
    UpdateModelMatrix();
}

void GameObject::Scale(float scaleMultiplier){
    // Relative scaling - multiply current scale
    this->scale *= scaleMultiplier;
    UpdateModelMatrix();
}

void GameObject::UpdateModelMatrix(){
    // Build transformation matrix in proper order: Translation * Rotation * Scale
    mat4 translation = Translate(position.x, position.y, position.z);
    mat4 rotation = RotateY(angle);
    mat4 scaleMatrix = Angel::Scale(scale, scale, scale);
    
    objectModelMatrix = translation * rotation * scaleMatrix;
}

void GameObject::Render(){
    objectLoader->program.setUniform("gModelMatrix", objectModelMatrix);
    objectLoader->render();
}
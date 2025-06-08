#include "GameObject.h"
#include "Angel.h"

GameObject::GameObject(ObjectLoader& objectLoader){
    this->objectLoader = &objectLoader;
    position = vec4(0,0,0,1);
    angleY = 0;
    angleZ = 0;
    angleX = 0;
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

void GameObject::RotateY(float deltaAngle){
    // Relative rotation - add to current angle
    this->angleY += deltaAngle;
    UpdateModelMatrix();
}

void GameObject::RotateZ(float deltaAngle){
    // Relative rotation - add to current angle
    this->angleZ += deltaAngle;
    UpdateModelMatrix();
}
void GameObject::RotateX(float deltaAngle){
    // Relative rotation - add to current angle
    this->angleX += deltaAngle;
    UpdateModelMatrix();
}

void GameObject::Rotate(float deltaAngle){
    // Default rotation around Y axis (most common for object placement)
    RotateY(deltaAngle);
}

void GameObject::Scale(float scaleMultiplier){
    // This is applying a scale on top of the existing scale.
    // To set an absolute scale, you would use `this->scale = scaleValue;`
    this->scale = scaleMultiplier;
    UpdateModelMatrix();
}

void GameObject::UpdateModelMatrix(){
    mat4 translation = Translate(position.x, position.y, position.z);
    mat4 rotationX = Angel::RotateX(angleX);
    mat4 rotationY = Angel::RotateY(angleY);
    mat4 rotationZ = Angel::RotateZ(angleZ);
    mat4 scaleMatrix = Angel::Scale(scale, scale, scale);
    
    mat4 rotation = rotationZ * rotationY * rotationX;
    
    objectModelMatrix = translation * rotation * scaleMatrix;
}

void GameObject::Render(Shader& shader){
    shader.setUniform("gModelMatrix", objectModelMatrix);
    objectLoader->render(shader);
}

vec3 GameObject::GetBoundingBoxSize() const {
    return objectLoader->GetBoundingBoxSize() * scale;
}

float GameObject::GetWidth() const {
    return objectLoader->GetBoundingBoxSize().x * scale;
}

float GameObject::GetDepth() const {
    return objectLoader->GetBoundingBoxSize().z * scale;
}

float GameObject::GetHeight() const {
    return objectLoader->GetBoundingBoxSize().y * scale;
}

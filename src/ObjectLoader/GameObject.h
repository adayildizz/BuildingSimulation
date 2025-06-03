#ifndef GAME_OBJECT_H
#define GAME_OBJECT_H

#include "Angel.h"
#include "ObjectLoader.h"
class GameObject{
    public:
        GameObject(ObjectLoader &objectLoader);
        ~GameObject();
        void Move(vec4 pos);
        void SetPosition(vec4 pos);
        void Rotate(float angle);
        void Scale(float amount);
        vec4 GetPosition();
        void Render();
        bool isInPlacement;
    private:
        vec4 position;
        float angle;
        float scale;
        ObjectLoader* objectLoader;
        mat4 objectModelMatrix;
        void UpdateModelMatrix();
        
};

#endif // GAME_OBJECT_H
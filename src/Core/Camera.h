#pragma once

#include "../include/Angel.h"
#include "../Core/Shader.h"

struct PersProjInfo {
    float FOV;
    float Width;
    float Height;
    float zNear;
    float zFar;
};

class Camera {
public:
    Camera() {}

    Camera(const PersProjInfo& persPositionInfo, const vec3& Pos, const vec3& Target, const vec3& Up);
    void InitCamera(const PersProjInfo& persPositionInfo, const vec3& Pos, const vec3& Target, const vec3& Up);
    void OnRender();

    // Camera control
    void OnKeyboard(int key);
    void OnMouse(int x, int y);
    void UpdateMousePos(int x, int y);
    
    // Rotation control
    void StartRotation() { m_isRotating = true; }
    void StopRotation() { m_isRotating = false; }

    // Setters
    void SetPosition(float x, float y, float z);
    void SetPosition(const vec3& pos);
    void SetTarget(float x, float y, float z);
    void SetTarget(const vec3& target);
    void SetSpeed(float speed);

    // Getters
    const PersProjInfo& GetPersProjInfo() const { return m_persProjInfo; }
    const mat4& GetProjMatrix() const { return m_projMatrix; }
    const vec3& GetPos() const { return m_pos; }
    const vec3& GetTarget() const { return m_target; }
    const vec3& GetUp() const { return m_up; }
    const float& GetSpeed() const { return m_speed; }

    mat4 GetViewMatrix() const;
    mat4 GetViewProjMatrix() const;
    mat4 GetViewPortMatrix() const;

    void Print() const { std::cout << "Camera[pos = " << m_pos << ", target = " << m_target << ", up = " << m_up << "]" << std::endl; }

private:
    void InitInternal();
    void Update();

    // Camera parameters
    PersProjInfo m_persProjInfo;
    mat4 m_projMatrix;

    vec3 m_pos;
    vec3 m_target;
    vec3 m_up;

    float m_speed = 5.0f;
    float m_windowWidth = 0.0f;
    float m_windowHeight = 0.0f;

    float m_angleH = 0.0f;
    float m_angleV = 0.0f;

    bool m_onUpperEdge = false;
    bool m_onLowerEdge = false;
    bool m_onLeftEdge = false;
    bool m_onRightEdge = false;

    vec2 m_mousePos = vec2(0.0f, 0.0f);
    bool m_isRotating = false;
};
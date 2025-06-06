#pragma once

#include "Angel.h"
#include "Shader.h"

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
    vec3 GetPosition() const;
    void invertPitch();

    // Raycasting methods
    vec3 ScreenToWorldRay(float screenX, float screenY) const;
    bool RayTerrainIntersection(const vec3& rayOrigin, const vec3& rayDirection, 
                               float minY, float maxY, vec3& intersectionPoint) const;
    
    // Enhanced terrain intersection with grid sampling
    template<typename TerrainGrid>
    bool GetTerrainIntersection(float mouseX, float mouseY, TerrainGrid* grid, vec3& intersectionPoint) const {
        if (!grid) return false;
        
        // Get ray direction from camera through mouse cursor
        vec3 rayDirection = ScreenToWorldRay(mouseX, mouseY);
        vec3 rayOrigin = GetPosition();
        
        // Use smaller step size for more precision
        float stepSize = 0.5f;
        float maxDistance = 2000.0f; // Maximum raycast distance
        
        for (float distance = 0; distance < maxDistance; distance += stepSize) {
            vec3 currentPoint = rayOrigin + rayDirection * distance;
            
            // Get terrain height at this point
            float terrainHeight = grid->GetHeightAtWorldPos(currentPoint.x, currentPoint.z);
            
            // Check if we've hit the terrain (ray point is below terrain)
            if (currentPoint.y <= terrainHeight) {
                // Backtrack for more precision
                vec3 prevPoint = rayOrigin + rayDirection * (distance - stepSize);
                
                // Linear interpolation between the two points for better accuracy
                float ratio = (terrainHeight - prevPoint.y) / (currentPoint.y - prevPoint.y);
                if (ratio >= 0.0f && ratio <= 1.0f) {
                    vec3 precisePoint = prevPoint + ratio * (currentPoint - prevPoint);
                    intersectionPoint = vec3(precisePoint.x, terrainHeight, precisePoint.z);
                } else {
                    intersectionPoint = vec3(currentPoint.x, terrainHeight, currentPoint.z);
                }
                return true;
            }
        }
        
        return false; // No intersection found
    }

    // Camera control
    void invertPitch() { m_angleV = -m_angleV; Update(); }

    void Print() const { std::cout << "Camera[pos = " << m_pos << ", target = " << m_target << ", up = " << m_up << "]" << std::endl; }

private:
    void InitInternal();
    void Update();
    mat4 InvertMatrix(const mat4& m) const;  // Helper function for matrix inversion

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

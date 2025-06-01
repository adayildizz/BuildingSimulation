#include "Camera.h"

static int MARGIN = 40;
static float EDGE_STEP = 0.5f;

Camera::Camera(const PersProjInfo& persPositionInfo, const vec3& Pos, const vec3& Target, const vec3& Up)
{
    InitCamera(persPositionInfo, Pos, Target, Up);
}


void Camera::InitCamera(const PersProjInfo& persPositionInfo, const vec3& Pos, const vec3& Target, const vec3& Up)
{
    m_persProjInfo = persPositionInfo;
    m_projMatrix = Perspective(m_persProjInfo.FOV, m_persProjInfo.Width/ m_persProjInfo.Height, m_persProjInfo.zNear, m_persProjInfo.zFar);

    m_windowHeight = (int)m_persProjInfo.Height;
    m_windowWidth = (int)m_persProjInfo.Width;

    m_pos = Pos;
    m_target = normalize(Target);
    m_up = normalize(Up);

    InitInternal();
}

void Camera::InitInternal()
{
    vec3 hTarget = vec3(m_target.x, 0.0f, m_target.z);
    hTarget = normalize(hTarget);

    // Calculate horizontal angle (in degrees)
    m_angleH = -atan2(m_target.z, m_target.x) * (180.0f / M_PI);
    
    // Calculate vertical angle (in degrees)
    m_angleV = -asin(m_target.y) * (180.0f / M_PI);
    
    // Initialize edge flags for mouse control
    m_onUpperEdge = false;
    m_onLowerEdge = false;
    m_onLeftEdge = false;
    m_onRightEdge = false;
    
    // Center mouse position
    m_mousePos.x = m_windowWidth / 2;
    m_mousePos.y = m_windowHeight / 2;
}

void Camera::OnRender()
{
    bool ShouldUpdate = false;

    if (m_onLeftEdge) {
        m_angleH -= EDGE_STEP;
        ShouldUpdate = true;
    } else if (m_onRightEdge) {
        m_angleH += EDGE_STEP;
        ShouldUpdate = true;
    }

    if (m_onUpperEdge) {
        if (m_angleV > -90.0f) {
            m_angleV -= EDGE_STEP;
            ShouldUpdate = true;
        }
    } else if (m_onLowerEdge) {
        if (m_angleV < 90.0f) {
            m_angleV += EDGE_STEP;
            ShouldUpdate = true;
        }
    }

    if (ShouldUpdate) {
        Update();
    }
}

void Camera::OnKeyboard(int key)
{
    bool CameraChangedPos = false;

    switch (key) {
    case GLFW_KEY_W:
        m_pos += (m_target * m_speed);
        break;

    case GLFW_KEY_S:
        m_pos -= (m_target * m_speed);
        break;

    case GLFW_KEY_A:
        {
            vec3 Right = cross(m_up, m_target);
            Right = normalize(Right);
            m_pos += (Right * m_speed);
        }
        break;

    case GLFW_KEY_D:
        {
            vec3 Left = cross(m_target, m_up);
            Left = normalize(Left);
            m_pos += (Left * m_speed);
        }
        break;

    case GLFW_KEY_DOWN:
        m_angleV += m_speed;
        CameraChangedPos = true;
        break;

    case GLFW_KEY_UP:
        m_angleV -= m_speed;
        CameraChangedPos = true;
        break;

    case GLFW_KEY_LEFT:
        m_angleH += m_speed;
        CameraChangedPos = true;
        break;

    case GLFW_KEY_RIGHT:
        m_angleH -= m_speed;
        CameraChangedPos = true;
        break;

    case GLFW_KEY_N:
        m_pos.y += m_speed;
        break;

    case GLFW_KEY_M:
        m_pos.y -= m_speed;
        break;

    case GLFW_KEY_I:
        m_speed += 1.0f;
        std::cout << "Speed changed to " << m_speed << std::endl;
        break;

    case GLFW_KEY_O:
        m_speed -= 1.0f;
        if (m_speed < 1.0f) {
            m_speed = 1.0f;
        }
        std::cout << "Speed changed to " << m_speed << std::endl;
        break;
    }

    // Need to update camera vectors if rotation angles have changed
    if (CameraChangedPos) {
        Update();
    }
}

void Camera::OnMouse(int x, int y)
{
    int DeltaX = x - m_mousePos.x;
    int DeltaY = y - m_mousePos.y;

    m_mousePos.x = x;
    m_mousePos.y = y;

    // Only update angles and camera if rotating
    if (m_isRotating) {
        m_angleH += (float)DeltaX / 20.0f;
        m_angleV += (float)DeltaY / 20.0f;

        // Clamp vertical angle to prevent flipping
        if (m_angleV > 89.0f) m_angleV = 89.0f;
        if (m_angleV < -89.0f) m_angleV = -89.0f;
        
        Update();
    }

    // Edge panning logic (optional, can be removed if only click-drag is desired)
    if (!m_isRotating) { // Only do edge panning if not click-dragging
        if (x <= MARGIN) {
            m_onLeftEdge = true;
            m_onRightEdge = false;
        } else if (x >= (m_windowWidth - MARGIN)) {
            m_onRightEdge = true;
            m_onLeftEdge = false;
        } else {
            m_onLeftEdge = false;
            m_onRightEdge = false;
        }

        if (y <= MARGIN) {
            m_onUpperEdge = true;
            m_onLowerEdge = false;
        } else if (y >= (m_windowHeight - MARGIN)) {
            m_onLowerEdge = true;
            m_onUpperEdge = false;
        } else {
            m_onUpperEdge = false;
            m_onLowerEdge = false;
        }
    }
    else {
        // Ensure edge flags are off when rotating
        m_onLeftEdge = false;
        m_onRightEdge = false;
        m_onUpperEdge = false;
        m_onLowerEdge = false;
    }
}

void Camera::UpdateMousePos(int x, int y)
{
    m_mousePos.x = x;
    m_mousePos.y = y;
}

void Camera::Update()
{
    // Set up a y-axis vector
    const vec3 Yaxis(0.0f, 1.0f, 0.0f);

    // Start with forward vector (1,0,0) and rotate it
    // First rotate around Y axis by horizontal angle
    vec3 view(1.0f, 0.0f, 0.0f);
    
    // Apply horizontal rotation using rotation matrix
    mat4 rotH = RotateY(m_angleH);
    vec4 rotatedView = rotH * vec4(view, 0.0f);
    view = vec3(rotatedView.x, rotatedView.y, rotatedView.z);
    view = normalize(view);

    // Compute the camera right vector (horizontal axis perpendicular to view and up)
    vec3 u = cross(Yaxis, view);
    u = normalize(u);

    // For vertical rotation, we'll use quaternion principles but with our available functions
    // Convert to radians for the trig functions
    float angleVRad = m_angleV * DegreesToRadians;
    
    // Create rotation matrix around axis u by angle m_angleV
    // We can use a Rodriguez rotation formula implementation
    float cosV = cos(angleVRad);
    float sinV = sin(angleVRad);
    
    // Apply Rodriguez rotation formula: v' = v*cos(θ) + (u×v)*sin(θ) + u*(u·v)*(1-cos(θ))
    vec3 crossResult = cross(u, view);
    float dotResult = dot(u, view);
    
    vec3 rotatedVertically = view * cosV + crossResult * sinV + u * dotResult * (1.0f - cosV);
    rotatedVertically = normalize(rotatedVertically);
    
    // Set the new target direction
    m_target = rotatedVertically;
    
    // Update up vector to be perpendicular to view and u
    m_up = cross(m_target, u);
    m_up = normalize(m_up);
}

void Camera::SetPosition(float x, float y, float z)
{
    m_pos = vec3(x, y, z);
}


void Camera::SetPosition(const vec3& pos)
{
    SetPosition(pos.x, pos.y, pos.z);
}

void Camera::SetTarget(float x, float y, float z)
{
    m_target = vec3(x, y, z);
    InitInternal();
}

void Camera::SetTarget(const vec3& target)
{
    SetTarget(target.x, target.y, target.z);
}

void Camera::SetSpeed(float speed)
{
    if (speed < 0.1f) {
        speed = 0.1f;
    }
    m_speed = speed;
}

mat4 Camera::GetViewMatrix() const
{
    vec3 targetPos = m_pos + m_target;  
    return LookAt(m_pos, targetPos, m_up);
}


mat4 Camera::GetViewProjMatrix() const
{
    mat4 viewMatrix = GetViewMatrix();
    return m_projMatrix * viewMatrix;
}

vec3 Camera::GetPosition() const { return m_pos; }

mat4 Camera::GetViewPortMatrix() const
{
    // Calculate half window dimensions
    float halfW = m_windowWidth / 2.0f;
    float halfH = m_windowHeight / 2.0f;
    
    mat4 viewport(
        halfW, 0.0f, 0.0f, 0.0f,
        0.0f, halfH, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        halfW, halfH, 0.0f, 1.0f
    );
    
    return viewport;
}


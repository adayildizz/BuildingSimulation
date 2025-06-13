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
    // This method now only handles non-movement keys
    // Movement keys are handled by UpdateMovement() for smooth movement
    switch (key) {
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
}

void Camera::OnKeyboardStateChange(int key, int action)
{
    // Track key press/release states for smooth movement
    if (key >= 0 && key < 512) {
        m_keyStates[key] = (action == GLFW_PRESS || action == GLFW_REPEAT);
    }
}

void Camera::UpdateMovement(float deltaTime)
{
    bool CameraChangedPos = false;
    vec3 moveDirection = vec3(0.0f);
    
    // Calculate movement based on currently pressed keys
    float frameSpeed = m_speed * deltaTime * 15.0f; // Even slower movement for better control
    
    // Forward/Backward movement (W/S)
    if (m_keyStates[GLFW_KEY_W]) {
        moveDirection += m_target * frameSpeed;
    }
    if (m_keyStates[GLFW_KEY_S]) {
        moveDirection -= m_target * frameSpeed;
    }
    
    // Strafe movement (A/D)
    if (m_keyStates[GLFW_KEY_A]) {
        vec3 Right = cross(m_up, m_target);
        Right = normalize(Right);
        moveDirection += Right * frameSpeed;
    }
    if (m_keyStates[GLFW_KEY_D]) {
        vec3 Left = cross(m_target, m_up);
        Left = normalize(Left);
        moveDirection += Left * frameSpeed;
    }
    
    // Vertical movement (N/M)
    if (m_keyStates[GLFW_KEY_N]) {
        moveDirection.y += frameSpeed;
    }
    if (m_keyStates[GLFW_KEY_M]) {
        moveDirection.y -= frameSpeed;
    }
    
    // Apply movement
    if (length(moveDirection) > 0.0f) {
        m_pos += moveDirection;
    }
    
    // Rotation with arrow keys
    if (m_keyStates[GLFW_KEY_UP]) {
        m_angleV -= frameSpeed;
        CameraChangedPos = true;
    }
    if (m_keyStates[GLFW_KEY_DOWN]) {
        m_angleV += frameSpeed;
        CameraChangedPos = true;
    }
    if (m_keyStates[GLFW_KEY_LEFT]) {
        m_angleH += frameSpeed;
        CameraChangedPos = true;
    }
    if (m_keyStates[GLFW_KEY_RIGHT]) {
        m_angleH -= frameSpeed;
        CameraChangedPos = true;
    }
    
    // Update camera vectors if rotation angles have changed
    if (CameraChangedPos) {
        Update();  // This calls the existing Update() method that updates camera vectors
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

// Helper function to invert a 4x4 matrix
mat4 Camera::InvertMatrix(const mat4& m) const
{
    mat4 inv;
    float det;
    
    inv[0][0] = m[1][1] * m[2][2] * m[3][3] - 
                m[1][1] * m[2][3] * m[3][2] - 
                m[2][1] * m[1][2] * m[3][3] + 
                m[2][1] * m[1][3] * m[3][2] +
                m[3][1] * m[1][2] * m[2][3] - 
                m[3][1] * m[1][3] * m[2][2];

    inv[1][0] = -m[1][0] * m[2][2] * m[3][3] + 
                 m[1][0] * m[2][3] * m[3][2] + 
                 m[2][0] * m[1][2] * m[3][3] - 
                 m[2][0] * m[1][3] * m[3][2] - 
                 m[3][0] * m[1][2] * m[2][3] + 
                 m[3][0] * m[1][3] * m[2][2];

    inv[2][0] = m[1][0] * m[2][1] * m[3][3] - 
                m[1][0] * m[2][3] * m[3][1] - 
                m[2][0] * m[1][1] * m[3][3] + 
                m[2][0] * m[1][3] * m[3][1] + 
                m[3][0] * m[1][1] * m[2][3] - 
                m[3][0] * m[1][3] * m[2][1];

    inv[3][0] = -m[1][0] * m[2][1] * m[3][2] + 
                 m[1][0] * m[2][2] * m[3][1] +
                 m[2][0] * m[1][1] * m[3][2] - 
                 m[2][0] * m[1][2] * m[3][1] - 
                 m[3][0] * m[1][1] * m[2][2] + 
                 m[3][0] * m[1][2] * m[2][1];

    inv[0][1] = -m[0][1] * m[2][2] * m[3][3] + 
                 m[0][1] * m[2][3] * m[3][2] + 
                 m[2][1] * m[0][2] * m[3][3] - 
                 m[2][1] * m[0][3] * m[3][2] - 
                 m[3][1] * m[0][2] * m[2][3] + 
                 m[3][1] * m[0][3] * m[2][2];

    inv[1][1] = m[0][0] * m[2][2] * m[3][3] - 
                m[0][0] * m[2][3] * m[3][2] - 
                m[2][0] * m[0][2] * m[3][3] + 
                m[2][0] * m[0][3] * m[3][2] + 
                m[3][0] * m[0][2] * m[2][3] - 
                m[3][0] * m[0][3] * m[2][2];

    inv[2][1] = -m[0][0] * m[2][1] * m[3][3] + 
                 m[0][0] * m[2][3] * m[3][1] + 
                 m[2][0] * m[0][1] * m[3][3] - 
                 m[2][0] * m[0][3] * m[3][1] - 
                 m[3][0] * m[0][1] * m[2][3] + 
                 m[3][0] * m[0][3] * m[2][1];

    inv[3][1] = m[0][0] * m[2][1] * m[3][2] - 
                m[0][0] * m[2][2] * m[3][1] - 
                m[2][0] * m[0][1] * m[3][2] + 
                m[2][0] * m[0][2] * m[3][1] + 
                m[3][0] * m[0][1] * m[2][2] - 
                m[3][0] * m[0][2] * m[2][1];

    inv[0][2] = m[0][1] * m[1][2] * m[3][3] - 
                m[0][1] * m[1][3] * m[3][2] - 
                m[1][1] * m[0][2] * m[3][3] + 
                m[1][1] * m[0][3] * m[3][2] + 
                m[3][1] * m[0][2] * m[1][3] - 
                m[3][1] * m[0][3] * m[1][2];

    inv[1][2] = -m[0][0] * m[1][2] * m[3][3] + 
                 m[0][0] * m[1][3] * m[3][2] + 
                 m[1][0] * m[0][2] * m[3][3] - 
                 m[1][0] * m[0][3] * m[3][2] - 
                 m[3][0] * m[0][2] * m[1][3] + 
                 m[3][0] * m[0][3] * m[1][2];

    inv[2][2] = m[0][0] * m[1][1] * m[3][3] - 
                m[0][0] * m[1][3] * m[3][1] - 
                m[1][0] * m[0][1] * m[3][3] + 
                m[1][0] * m[0][3] * m[3][1] + 
                m[3][0] * m[0][1] * m[1][3] - 
                m[3][0] * m[0][3] * m[1][1];

    inv[3][2] = -m[0][0] * m[1][1] * m[3][2] + 
                 m[0][0] * m[1][2] * m[3][1] + 
                 m[1][0] * m[0][1] * m[3][2] - 
                 m[1][0] * m[0][2] * m[3][1] - 
                 m[3][0] * m[0][1] * m[1][2] + 
                 m[3][0] * m[0][2] * m[1][1];

    inv[0][3] = -m[0][1] * m[1][2] * m[2][3] + 
                 m[0][1] * m[1][3] * m[2][2] + 
                 m[1][1] * m[0][2] * m[2][3] - 
                 m[1][1] * m[0][3] * m[2][2] - 
                 m[2][1] * m[0][2] * m[1][3] + 
                 m[2][1] * m[0][3] * m[1][2];

    inv[1][3] = m[0][0] * m[1][2] * m[2][3] - 
                m[0][0] * m[1][3] * m[2][2] - 
                m[1][0] * m[0][2] * m[2][3] + 
                m[1][0] * m[0][3] * m[2][2] + 
                m[2][0] * m[0][2] * m[1][3] - 
                m[2][0] * m[0][3] * m[1][2];

    inv[2][3] = -m[0][0] * m[1][1] * m[2][3] + 
                 m[0][0] * m[1][3] * m[2][1] + 
                 m[1][0] * m[0][1] * m[2][3] - 
                 m[1][0] * m[0][3] * m[2][1] - 
                 m[2][0] * m[0][1] * m[1][3] + 
                 m[2][0] * m[0][3] * m[1][1];

    inv[3][3] = m[0][0] * m[1][1] * m[2][2] - 
                m[0][0] * m[1][2] * m[2][1] - 
                m[1][0] * m[0][1] * m[2][2] + 
                m[1][0] * m[0][2] * m[2][1] + 
                m[2][0] * m[0][1] * m[1][2] - 
                m[2][0] * m[0][2] * m[1][1];

    det = m[0][0] * inv[0][0] + m[0][1] * inv[1][0] + m[0][2] * inv[2][0] + m[0][3] * inv[3][0];

    if (det == 0)
        return mat4(); // Return identity matrix if not invertible

    det = 1.0 / det;

    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            inv[i][j] = inv[i][j] * det;

    return inv;
}

vec3 Camera::ScreenToWorldRay(float screenX, float screenY) const
{
    // Convert screen coordinates to normalized device coordinates (-1 to 1)
    // Note: GLFW uses top-left origin, OpenGL uses bottom-left
    float x = (2.0f * screenX) / m_persProjInfo.Width - 1.0f;
    float y = 1.0f - (2.0f * screenY) / m_persProjInfo.Height; // Flip Y coordinate
    
    // Create ray endpoints in NDC space
    vec4 rayStart_NDC = vec4(x, y, -1.0f, 1.0f); // Near plane
    vec4 rayEnd_NDC = vec4(x, y, 1.0f, 1.0f);    // Far plane
    
    // Convert to world space
    mat4 invViewProjMatrix = InvertMatrix(GetViewProjMatrix());
    
    vec4 rayStart_world = invViewProjMatrix * rayStart_NDC;
    vec4 rayEnd_world = invViewProjMatrix * rayEnd_NDC;
    
    // Perspective divide
    rayStart_world /= rayStart_world.w;
    rayEnd_world /= rayEnd_world.w;
    
    // Calculate ray direction - fix the vec4 to vec3 conversion
    vec3 rayDirection = normalize(vec3(rayEnd_world.x, rayEnd_world.y, rayEnd_world.z) - 
                                 vec3(rayStart_world.x, rayStart_world.y, rayStart_world.z));
    
    return rayDirection;
}

bool Camera::RayTerrainIntersection(const vec3& rayOrigin, const vec3& rayDirection, 
                                   float minY, float maxY, vec3& intersectionPoint) const
{
  
    if (abs(rayDirection.y) < 0.001f) {
        return false;
    }
    
    float averageTerrainHeight = (minY + maxY) / 2.0f;
    
    float t = (averageTerrainHeight - rayOrigin.y) / rayDirection.y;
    
    // If t is negative, intersection is behind the camera
    if (t < 0) {
        return false;
    }
    
    // Calculate intersection point
    intersectionPoint = rayOrigin + t * rayDirection;
    
    return true;
}




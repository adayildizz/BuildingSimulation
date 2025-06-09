#include "CelestialLightManager.h"
#include <cmath> // For fmod, cos, sin, fmax, abs. Ensure M_PI is available.
                 // If M_PI is not found, define it: const double M_PI = acos(-1.0);

CelestialLightManager::CelestialLightManager()
    : m_timeOfDay(0.0f),
      m_dayCycleSpeed(0.3f), // You can make this configurable
      m_daySkyColor(0.529f, 0.808f, 0.922f),
      m_nightSkyColor(0.0f, 0.0f, 0.0f),
      m_sunDayColor(1.0f, 1.0f, 0.95f),
      m_sunDayAmbientIntensity(0.3f),
      m_sunDayDiffuseIntensity(0.85f),
      m_moonNightColor(0.4f, 0.45f, 0.6f),
      m_moonNightAmbientIntensity(0.6f),
      m_moonNightDiffuseIntensity(0.05f),
      // Initialize calculated values to a default (e.g., daytime start)
      m_currentSkyColor(m_daySkyColor),
      m_actualLightDirectionForShader(normalize(vec3(0.4f, 0.8f, 0.3f))), // Initial sun direction
      m_activeLightColor(m_sunDayColor),
      m_activeAmbientIntensity(m_sunDayAmbientIntensity),
      m_activeDiffuseIntensity(m_sunDayDiffuseIntensity)
{
    CalculateCelestialProperties(); // Perform initial calculation
}

void CelestialLightManager::Update(float deltaTime) {
    m_timeOfDay += deltaTime * m_dayCycleSpeed;
    // Wrap m_timeOfDay around 2*PI to keep the angle in a manageable range
    m_timeOfDay = fmod(m_timeOfDay, 2.0f * static_cast<float>(M_PI));
    
    CalculateCelestialProperties();
}

void CelestialLightManager::CalculateCelestialProperties() {
    vec3 calculatedCelestialDirection;
    float celestialAngleRadians = m_timeOfDay;

    // Calculate the basic direction of the celestial body (sun/moon)
    calculatedCelestialDirection.x = cos(celestialAngleRadians);
    calculatedCelestialDirection.y = sin(celestialAngleRadians);
    calculatedCelestialDirection.z = 0.15f; // A small constant tilt is enough.


    float dayFactor = fmax(0.0f, calculatedCelestialDirection.y);
    
    // Interpolate sky color based on dayFactor
    m_currentSkyColor = m_nightSkyColor * (1.0f - dayFactor) + m_daySkyColor * dayFactor;

    // Normalize the direction vector for lighting calculations
    vec3 normalizedCelestialDirection = normalize(calculatedCelestialDirection);

    // Determine if it's daytime or nighttime for light properties
    if (normalizedCelestialDirection.y > 0.01f) { // Sun is considered "up"
        m_activeLightColor = m_sunDayColor;
        m_activeAmbientIntensity = m_sunDayAmbientIntensity;
        m_activeDiffuseIntensity = m_sunDayDiffuseIntensity;
        m_actualLightDirectionForShader = normalizedCelestialDirection;
    } else { // Moon is considered "up", or it's night
        m_activeLightColor = m_moonNightColor;
        m_activeAmbientIntensity = m_moonNightAmbientIntensity;
        m_activeDiffuseIntensity = m_moonNightDiffuseIntensity;
        
        // For moonlight, we might want to ensure it always comes from "above"
        // even if the calculated position is technically below the horizon.
        // This makes the moon's light direction more intuitive for "night".
        m_actualLightDirectionForShader.x = normalizedCelestialDirection.x;
        m_actualLightDirectionForShader.y = abs(normalizedCelestialDirection.y); // Force y to be positive
        if (m_actualLightDirectionForShader.y < 0.1f) { // Ensure a minimum elevation
            m_actualLightDirectionForShader.y = 0.1f;
        }
        m_actualLightDirectionForShader.z = normalizedCelestialDirection.z;
        m_actualLightDirectionForShader = normalize(m_actualLightDirectionForShader);
    }
}

vec3 CelestialLightManager::GetCurrentSkyColor() const {
    return m_currentSkyColor;
}

vec3 CelestialLightManager::GetActiveLightDirection() const {
    return m_actualLightDirectionForShader;
}

vec3 CelestialLightManager::GetActiveLightColor() const {
    return m_activeLightColor;
}

GLfloat CelestialLightManager::GetActiveAmbientIntensity() const {
    return m_activeAmbientIntensity;
}

GLfloat CelestialLightManager::GetActiveDiffuseIntensity() const {
    return m_activeDiffuseIntensity;
}

void CelestialLightManager::ConfigureLight(Light* lightObject) const {
    if (lightObject) {
        lightObject->SetDirection(m_actualLightDirectionForShader);
        lightObject->SetColor(m_activeLightColor);
        lightObject->SetAmbientIntensity(m_activeAmbientIntensity);
        lightObject->SetDiffuseIntensity(m_activeDiffuseIntensity);
    }
}

bool CelestialLightManager::IsSunAtZenith() const {
    // The world's 'up' vector
    vec3 upVector = vec3(0.0f, 1.0f, 0.0f);
    
    // Get the current light direction
    vec3 lightDir = GetActiveLightDirection();
    
    // If the dot product of the light direction and the world up vector is
    // very close to 1.0, it means the light is almost directly overhead.
    // A threshold of 0.99 gives a small window around the peak.
    if (dot(lightDir, upVector) > 0.80f) {
        return true;
    }
    
    return false;
}

#include "CelestialLightManager.h"
#include <cmath> // For fmod, cos, sin, fmax, abs. Ensure M_PI is available.
                 // If M_PI is not found, define it: const double M_PI = acos(-1.0);

CelestialLightManager::CelestialLightManager()
    : m_timeOfDay(0.0f),
      m_dayCycleSpeed(0.2f), // You can make this configurable
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
    // This path makes it rise in +Y, move across X, and set. Z is a slight tilt.
    calculatedCelestialDirection.x = cos(celestialAngleRadians);
    calculatedCelestialDirection.y = sin(celestialAngleRadians);
    calculatedCelestialDirection.z = 0.2f; // Adjust for desired celestial path tilt

    // Determine dayFactor: 0 for night (celestial body below horizon), 1 for full day (celestial body at zenith)
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

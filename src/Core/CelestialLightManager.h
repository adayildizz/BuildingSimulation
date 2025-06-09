#ifndef CELESTIAL_LIGHT_MANAGER_H
#define CELESTIAL_LIGHT_MANAGER_H

#include "Angel.h" // For vec3, M_PI, cos, sin, fmod, fmax, normalize
#include "light.h" // For the Light class
#include <memory>  // For std::unique_ptr (if it were to own Light, not in this design)

// Forward declaration for the Light class if its definition isn't fully needed in the header.
// However, since ConfigureLight takes a Light*, including "light.h" is cleaner.
// class Light;

class CelestialLightManager {
public:
    CelestialLightManager();

    // Call this once per frame to update time and recalculate light properties
    void Update(float deltaTime);

    // Getters for GridDemo to use
    vec3 GetCurrentSkyColor() const;
    vec3 GetActiveLightDirection() const; // Primarily for debugging or direct use if needed
    vec3 GetActiveLightColor() const;     // Primarily for debugging or direct use if needed
    GLfloat GetActiveAmbientIntensity() const; // Primarily for debugging or direct use if needed
    GLfloat GetActiveDiffuseIntensity() const; // Primarily for debugging or direct use if needed

    // Method to directly configure an existing Light object
    void ConfigureLight(Light* lightObject) const;
    
    // A function that tells if the sun is at the top.
    bool IsSunAtZenith() const;

private:
    // Time of day and cycle speed
    float m_timeOfDay;
    float m_dayCycleSpeed;

    // Sky color properties
    vec3 m_daySkyColor;
    vec3 m_nightSkyColor;

    // Sunlight (Daytime) properties
    vec3 m_sunDayColor;
    GLfloat m_sunDayAmbientIntensity;
    GLfloat m_sunDayDiffuseIntensity;

    // Moonlight (Nighttime) properties
    vec3 m_moonNightColor;
    GLfloat m_moonNightAmbientIntensity;
    GLfloat m_moonNightDiffuseIntensity;

    // ---- Calculated values (updated in Update method) ----
    vec3 m_currentSkyColor;                 // The interpolated sky color for glClearColor
    vec3 m_actualLightDirectionForShader;   // The direction vector for the current light source
    vec3 m_activeLightColor;                // The color of the current light source
    GLfloat m_activeAmbientIntensity;       // The ambient intensity of the current light source
    GLfloat m_activeDiffuseIntensity;       // The diffuse intensity of the current light source

    // Internal helper method to perform the calculations
    void CalculateCelestialProperties();
};

#endif // CELESTIAL_LIGHT_MANAGER_H

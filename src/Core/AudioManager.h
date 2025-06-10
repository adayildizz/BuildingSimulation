#pragma once

#include <SFML/Audio.hpp>
#include <string>

class AudioManager {
public:
    // Get the singleton instance of the AudioManager
    static AudioManager& getInstance();

    // Load and play a looping music file.
    // It will stop any previously playing music.
    void playMusic(const std::string& filePath);

    // Stop the currently playing music
    void stopMusic();

private:
    // Private constructor and destructor for singleton pattern
    AudioManager();
    ~AudioManager();

    // Delete copy and assignment operators
    AudioManager(const AudioManager&) = delete;
    AudioManager& operator=(const AudioManager&) = delete;

    // The SFML Music object for streaming background music
    sf::Music m_music;
};

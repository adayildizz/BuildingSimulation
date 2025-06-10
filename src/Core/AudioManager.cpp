#include "AudioManager.h"
#include <iostream>

AudioManager& AudioManager::getInstance() {
    static AudioManager instance; // This is created only once
    return instance;
}

// Constructor can be empty
AudioManager::AudioManager() {}

// Destructor can be empty, sf::Music handles its own cleanup
AudioManager::~AudioManager() {}

void AudioManager::playMusic(const std::string& filePath) {
    // openFromFile streams the music from the file, which is memory-efficient.
    if (!m_music.openFromFile(filePath)) {
        std::cerr << "Error: Could not open music file: " << filePath << std::endl;
        return; 
    }
    m_music.setLooping(true); // Always loop background music
    m_music.play();
}

void AudioManager::stopMusic() {
    m_music.stop();
}

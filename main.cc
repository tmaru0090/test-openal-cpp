#include <iostream>
#include <AL/alc.h>
#include <AL/al.h>
#include <AL/alext.h>
#include <fstream>
#include <cstdint>
#include <thread>
#include <chrono>

void checkOpenALError(const std::string& message) {
    ALenum error = alGetError();
    if (error != AL_NO_ERROR) {
        std::cerr << "OpenAL Error (" << message << "): " << alGetString(error) << std::endl;
    }
}

int main(int argc,char**argv) {
    std::string filePath;
    if(argc < 2){
        std::cerr << "引数は少なくとも1つ必要です" << std::endl;
        std::cerr << "デフォルトのパスを使用します" << std::endl;
	filePath = "/home/tanukimaru/th-music/麓の神社.wav";
    }else{
	filePath = argv[1];	
    }
    ALCdevice* device = alcOpenDevice(NULL);
    if (!device) {
        std::cerr << "Failed to open audio device." << std::endl;
        return -1;
    }
    ALCcontext* context = alcCreateContext(device, NULL);
    if (!context) {
        std::cerr << "Failed to create audio context." << std::endl;
        alcCloseDevice(device);
        return -1;
    }
    alcMakeContextCurrent(context);

    ALuint buffer, source;
    alGenBuffers(1, &buffer);
    checkOpenALError("alGenBuffers");
    alGenSources(1, &source);
    checkOpenALError("alGenSources");

    std::ifstream file(filePath, std::ios::binary);
    if (!file) {
        std::cerr << "Failed to open sound file." << std::endl;
        alDeleteSources(1, &source);
        alDeleteBuffers(1, &buffer);
        alcMakeContextCurrent(NULL);
        alcDestroyContext(context);
        alcCloseDevice(device);
        return -1;
    }

    // Read header
    char header[44];
    file.read(header, 44);
    if (!file) {
        std::cerr << "Failed to read WAV header." << std::endl;
        return -1;
    }

    // Extract format information
    int sampleRate = *reinterpret_cast<int*>(&header[24]);
    int bitsPerSample = *reinterpret_cast<short*>(&header[34]);
    int numChannels = *reinterpret_cast<short*>(&header[22]);
    int dataSize = *reinterpret_cast<int*>(&header[40]);

    std::cout << "Sample Rate: " << sampleRate << std::endl;
    std::cout << "Bits Per Sample: " << bitsPerSample << std::endl;
    std::cout << "Number of Channels: " << numChannels << std::endl;
    std::cout << "Data Size: " << dataSize << std::endl;

    ALenum format;
    if (numChannels == 1) {
        format = (bitsPerSample == 8) ? AL_FORMAT_MONO8 : AL_FORMAT_MONO16;
    } else if (numChannels == 2) {
        format = (bitsPerSample == 8) ? AL_FORMAT_STEREO8 : AL_FORMAT_STEREO16;
    } else {
        std::cerr << "Unsupported number of channels: " << numChannels << std::endl;
        return -1;
    }

    // Read audio data
    file.seekg(0, std::ios::end);
    std::streampos fileSize = file.tellg();
    file.seekg(44, std::ios::beg); // Skip header

    dataSize = static_cast<int>(fileSize) - 44;
    char* data = new char[dataSize];
    file.read(data, dataSize);
    if (!file) {
        std::cerr << "Failed to read WAV data." << std::endl;
        delete[] data;
        return -1;
    }
    file.close();

    // Load data into OpenAL
    alBufferData(buffer, format, data, dataSize, sampleRate);
    checkOpenALError("alBufferData");
    delete[] data;

    alSourcei(source, AL_BUFFER, buffer);
    checkOpenALError("alSourcei");
    alSourcePlay(source);
    checkOpenALError("alSourcePlay");

    ALint state;
    alGetSourcei(source, AL_SOURCE_STATE, &state);
    std::cout << "Initial state: " << state << std::endl;

    do {
        alGetSourcei(source, AL_SOURCE_STATE, &state);
        std::cout << "Current state: " << state << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Wait for 100 milliseconds
    } while (state == AL_PLAYING);

    alDeleteSources(1, &source);
    alDeleteBuffers(1, &buffer);
    alcMakeContextCurrent(NULL);
    alcDestroyContext(context);
    alcCloseDevice(device);

    return 0;
}

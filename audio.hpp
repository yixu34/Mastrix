#ifndef AUDIO_HPP
#define AUDIO_HPP

#include "fmod.h"

class Audio;
class Sound;
class Music;

extern Audio audio;

//wrapper for a sound effect file
class Sound
{
public:
	Sound() 
    {
        soundFlag = FSOUND_HW2D;
        soundData = 0;
    };
	~Sound(){};

    typedef FSOUND_SAMPLE* SoundData;

	void load(const char *fileName, bool is3D = true)
	{
		if (is3D)
			soundFlag = FSOUND_HW3D;

		soundData = FSOUND_Sample_Load(
						FSOUND_FREE, 
						fileName, 
						soundFlag, 
						0, 
						0);
	}

	inline SoundData getSoundData() const
	{
		return soundData;
	}

    inline bool is3D() const
    {
        return soundFlag == FSOUND_HW3D;
    }

private:
	SoundData soundData;
    int soundFlag;
};

//wrapper for a music file
class Music
{
public:
	Music() 
    {
        musicData = 0;
    };
	~Music(){};

    typedef FSOUND_STREAM* MusicData;
	
	void load(const char *fileName, bool repeat = true)
	{
        int musicFlags = FSOUND_2D;

        if (repeat)
            musicFlags |= FSOUND_LOOP_NORMAL;

		musicData = FSOUND_Stream_Open(
			fileName, 
			musicFlags, 
			0, 
			0);
	}

	inline MusicData getMusicData() const
	{
		return musicData;
	}

private:
	MusicData musicData;
};

//manages all sound/music playback
class Audio
{
public:
	Audio();
	~Audio();

	void initialize();
	void shutdown();
	void update();

	void playSound(const Sound &sound);
	void playSoundEx(
		const Sound &sound,
		float x, 
		float y, 
		float z = 0.0f,  
		bool  repeat = false);
	void stopAllSounds();

	void playMusic(const Music &music, bool repeat = false);
	void stopMusic(const Music &music);

    void setVolume(int volume);
    void setMusicVolume(int volume);

	const int getMusicChannel() const
	{
		return musicChannel;
	}

    enum speaker_mode
    {
        speaker_headphones = FSOUND_SPEAKERMODE_HEADPHONES, 
        speaker_mono       = FSOUND_SPEAKERMODE_MONO, 
        speaker_quad       = FSOUND_SPEAKERMODE_QUAD, 
        speaker_stereo     = FSOUND_SPEAKERMODE_STEREO
    };
    void setSpeakerMode(speaker_mode speakerMode);
    const speaker_mode getSpeakerModeByName(
        const std::string &speakerModeName) const;

    static const int maxVolume;
    static const int minVolume;

private:
    int currentVolume;
    int currentMusicVolume;
    int musicChannel;
    speaker_mode currentSpeakerMode;
};

void setSoundtrack(const char *filename, bool repeat);
void stopMusic(void);
void resumeMusic(bool repeat);

#endif	//AUDIO_HPP


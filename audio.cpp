#include "mastrix.hpp"

Audio audio;	//external declaration

Audio::Audio()
{
    currentVolume      = 255;
    currentMusicVolume = 128;
    musicChannel       = 0;
    currentSpeakerMode = speaker_stereo;
}

Audio::~Audio()
{}

void Audio::initialize()
{
	FSOUND_Init(44100, 32, 0);
    setSpeakerMode(currentSpeakerMode);
    currentVolume = FSOUND_GetSFXMasterVolume();
}


void Audio::update()
{
	FSOUND_Update();
}

void Audio::shutdown()
{
	FSOUND_Close();
}

//play a 2D (non-positional) sound.  Make sure that the sound was loaded
//with 2D parameters, not 3D!
void Audio::playSound(const Sound &sound)
{
	FSOUND_PlaySound(FSOUND_FREE, sound.getSoundData());
}

//play a 3D (positional) sound.  Make sure that thes ound was loaded with 
//3D parameters, not 2D!
void Audio::playSoundEx(
	const Sound &sound, 
	float x, 
	float y, 
	float z, 
	bool repeat)
{
	float soundOrientation[3] = {x, y, z};

	int channel = FSOUND_PlaySoundEx(
						FSOUND_FREE, 
						sound.getSoundData(), 
						0, 
						repeat);

	FSOUND_3D_SetAttributes(
		channel, 
		soundOrientation, 
		0);
}

//music should be loaded by this point!
void Audio::playMusic(const Music &music, bool repeat)
{
	musicChannel = FSOUND_Stream_PlayEx(
		               FSOUND_FREE,
		               music.getMusicData(), 
                       0, 
                       false);

    setMusicVolume(currentMusicVolume);
}

void Audio::stopMusic(const Music &music)
{
	FSOUND_Stream_Close(music.getMusicData());
}

const int Audio::maxVolume = 255;
const int Audio::minVolume = 0;

//change the master volume
void Audio::setVolume(int volume)
{
    FSOUND_SetSFXMasterVolume(volume);
}

//change the music volume only
void Audio::setMusicVolume(int volume)
{
    currentMusicVolume = volume;
    FSOUND_SetVolume(musicChannel, currentMusicVolume);
}

//change the speaker settings to match your speaker setup
void Audio::setSpeakerMode(speaker_mode speakerMode)
{
    currentSpeakerMode = speakerMode;
    FSOUND_SetSpeakerMode(currentSpeakerMode);
}

const Audio::speaker_mode Audio::getSpeakerModeByName(
    const std::string &speakerModeName) const
{
    if(speakerModeName == "headphones")
        return speaker_headphones;
    else if(speakerModeName == "mono")
        return speaker_mono;
    else if(speakerModeName == "quad")
        return speaker_quad;
    else
        return speaker_stereo;  
}

static std::string currentSoundtrack("");
static Music *currentMusic = NULL;

void setVolume(int volume);
void setMusicVolume(int volume);

ClientConsoleVar<int> volume("volume", 255, setVolume);
ClientConsoleVar<int> musicVolume("music_volume", 128, setMusicVolume);

void setSoundtrack(const char *filename, bool repeat)
{
	if(filename)
		currentSoundtrack = filename;
	
    int oldVolume = musicVolume;

	stopMusic();
	resumeMusic(repeat);
    setMusicVolume(musicVolume);
}
void stopMusic(void)
{
	if(currentMusic) {
		audio.stopMusic(*currentMusic);
		delete currentMusic;
		currentMusic = NULL;
	}
}
void resumeMusic(bool repeat)
{
	if(currentMusic) return;
	currentMusic = new Music();
	currentMusic->load(currentSoundtrack.c_str(), repeat);
	audio.playMusic(*currentMusic);
}

void setVolume(int volume)
{
	FSOUND_SetSFXMasterVolume(volume);
}

void setMusicVolume(int volume)
{
	FSOUND_SetVolume(audio.getMusicChannel(), volume);
}



CLIENT_CONSOLE_COMMAND(set_speaker_mode)
{
    if (argc < 1)
    {
        printfToClient(who, "Not enough arguments to set_speaker_mode (should be 1)");
        printfToClient(who, "Usage:  set_speaker_mode mode");
        return;
    }

    audio.setSpeakerMode(audio.getSpeakerModeByName(argv[0]));
}

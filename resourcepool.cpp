#include "mastrix.hpp"

ImagePool images;
SoundPool sounds;

ImagePool::ImagePool()
{ }

ImagePool::~ImagePool()
{
	for (ImageMap::iterator ii = imageMap.begin(); ii != imageMap.end(); ++ii)
	{
		delete ii->second;
		ii->second = 0;
	}

	imageMap.clear();
}

void ImagePool::addImage(const char *imgName)
{
	imageMap[imgName] = new Image();
	imageMap[imgName]->load(imgName);
}

Image *ImagePool::getImage(const char *imgName)
{
	if (imageMap.find(imgName) == imageMap.end())
		addImage(imgName);

	return imageMap[imgName];
}

void ImagePool::refreshTextures(void)
{
	for(ImageMap::iterator ii=imageMap.begin(); ii!=imageMap.end(); ii++)
		ii->second->load(ii->first.c_str());
}



//soundpool methods
SoundPool::SoundPool()
{
}

SoundPool::~SoundPool()
{
	for (SoundMap::iterator ii = soundMap.begin(); ii != soundMap.end(); ++ii)
	{
		//audio.stopAllSounds();
		delete ii->second;
		ii->second = 0;
	}
	soundMap.clear();
}

void SoundPool::addSound(const char *sndName, bool is3D)
{
	soundMap[sndName] = new Sound();
    soundMap[sndName]->load(sndName, is3D);
}

Sound *SoundPool::getSound(const char *sndName, bool is3D)
{
	if (soundMap.find(sndName) == soundMap.end())
        addSound(sndName, is3D);

	return soundMap[sndName];
}

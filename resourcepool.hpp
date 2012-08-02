//Container classes for game media (images and sounds)
//-Yi
#ifndef RESOURCEPOOL_HPP
#define RESOURCEPOOL_HPP

#include <map>
#include <string>


class Image;
class Sound;
class Music;

class ImagePool;
class SoundPool;

extern ImagePool images;
extern SoundPool sounds;

class ImagePool
{
public:
	ImagePool();
	~ImagePool();

	void   addImage(const char *imgName);
	Image *getImage(const char *imgName);
	void refreshTextures(void);

private:
	typedef std::map<std::string, Image*> ImageMap;
	ImageMap imageMap;
};

class SoundPool
{
public:
	SoundPool();
	~SoundPool();

    void   addSound(const char *sndName, bool is3D);
	Sound *getSound(const char *sndName, bool is3D);

private:
	typedef std::map<std::string, Sound*> SoundMap;
	SoundMap soundMap;
};

#endif	//RESOURCEPOOL_HPP

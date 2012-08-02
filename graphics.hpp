#ifndef GRAPHICS_HPP
#define GRAPHICS_HPP

class Image;
class Color;
class Graphics;

extern Graphics graphics;

class Image
{
public:
	void load(const char *fileName);

	inline int getWidth() const
		{ return width; }
	inline int getHeight() const
		{ return height; }
	inline GLuint getTexID() const
		{ return textureID; }

private:
	GLuint textureID;
	int	   width;
	int	   height;
};

class Color
{
public:
	typedef unsigned char uchar;

	Color()
	{ setComponents(0, 0, 0, 0); }

	Color(uchar r, uchar g, uchar b, uchar a = 255)
	{ setComponents(r, g, b, a); }

	void setComponents(uchar r, uchar g, uchar b, uchar a = 255)
	{
		red   = r;
		green = g;
		blue  = b;
		alpha = a;
	}

	uchar red;
	uchar green;
	uchar blue;
	uchar alpha;
};

class Graphics
{
public:
	Graphics();
	~Graphics();

	void initialize(int width, int height);
	void shutDown();

	void setFullScreen(bool isFullScreen);
	void changeResolution(int width, int height);

	void drawImage(
		const Image *image, 
		float x, 
		float y, 
		float angleRadians = 0.0f, 
		float scale = 1.0f);

	void drawColoredImage(
		const Image *image, 
		float x, 
		float y, 
		float angleRadians = 0.0f, 
		float scale = 1.0f);

	void drawPoint(
		const Color &color, 
		float x, 
		float y, 
		float size = 1.0f);

	void drawVertex(
		float x, 
		float y);

	void drawLine(
		const Color &color, 
		float x1, 
		float y1, 
		float x2, 
		float y2, 
		float width = 1.0f);

	void drawRect(
		const Color &color, 
		float xLeft, 
		float yTop, 
		float xRight, 
		float yBottom);

	void setColor(const Color &color);

	void drawText(const char *text, float x, float y, void *font=GLUT_BITMAP_9_BY_15);
	void drawTextN(const char *text, int maxlen, float x, float y, void *font=GLUT_BITMAP_9_BY_15);
	void drawTextCentered(const char *text, float x, float y, void *font=GLUT_BITMAP_9_BY_15);
	int  drawTextWidth(const char *text, void *font=GLUT_BITMAP_9_BY_15);
	int  drawCharWidth(char c, void *font=GLUT_BITMAP_9_BY_15);

	void swapBuffers();

	void clearScreen();

	inline int getScreenWidth() const
		{ return screenWidth; }
	inline int getScreenHeight() const
		{ return screenHeight; }
	SDL_Surface *getScreenBuffer()
		{ return screen; }

private:
	void initSDL(int width, int height);
	void initOpenGL();
	
	bool currentlyFullscreen;
	int screenWidth;
	int screenHeight;

	SDL_Surface *screen;
};

#endif	//GRAPHICS_HPP


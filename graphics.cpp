#include "mastrix.hpp"

Graphics graphics;

void Image::load(const char *fileName)
{
	if(!strcmp(fileName, "") || !strcmp(fileName, " ")) {
		width = height = 0;
		textureID = 0;
		return;
	}
	SDL_Surface *image = 0;
	image = IMG_Load(fileName);
	
	if (image == 0)
	{
		//put some error message here?
		console.printf("Could not load image '%s': %s", fileName, SDL_GetError());
	//	assert(false);
		return;
	}
	
	//keep track of dimensions
	width  = image->w;
	height = image->h;

	if(image->format->BitsPerPixel != 32) {
		textureID = 0;
		console.printf("Could not load image %s: wrong color depth.", fileName);
		return;
	}

	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_2D, textureID);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
/*	glTexImage2D(
		GL_TEXTURE_2D,
		0, 
		4,
		image->w, 
		image->h, 
		0, 
		GL_RGBA, 
		GL_UNSIGNED_BYTE, 
		image->pixels);*/
	gluBuild2DMipmaps(GL_TEXTURE_2D, 4, image->w, image->h, GL_RGBA, GL_UNSIGNED_BYTE, image->pixels);

	SDL_FreeSurface(image);
	glBindTexture(GL_TEXTURE_2D, 0);
}

Graphics::Graphics()
{
	screen	   = 0;

	//some default values for the screen dimensions
	screenWidth  = 1024;
	screenHeight = 768;
}

Graphics::~Graphics()
{
	screen = 0;
}

void Graphics::initialize(int width, int height)
{
	initSDL(width, height);
	initOpenGL();
}

void Graphics::setFullScreen(bool isFullScreen)
{
	int fullScreenFlags;
	if(currentlyFullscreen == isFullScreen) return;
	if (isFullScreen)
		fullScreenFlags = SDL_OPENGL | SDL_FULLSCREEN;
	else
		fullScreenFlags = SDL_OPENGL;
	currentlyFullscreen = isFullScreen;

	screen = SDL_SetVideoMode(
		screenWidth, 
		screenHeight, 
		0, 
		fullScreenFlags);

	initOpenGL();
	images.refreshTextures();
}

void Graphics::changeResolution(int width, int height)
{
	//ignore the command if the new dimensions are the safe
	if (width == screenWidth && height == screenHeight)
		return;
	
	int fullScreenFlags;
	if (fullScreen)
		fullScreenFlags = SDL_OPENGL | SDL_FULLSCREEN;
	else
		fullScreenFlags = SDL_OPENGL;

	screen = SDL_SetVideoMode(
		width, 
		height, 
		0, 
		fullScreenFlags);

	//if the new resolution failed, fall back to the old one!
	if (screen == 0)
	{
		screen = SDL_SetVideoMode(
			screenWidth, 
			screenHeight, 
			0, 
			fullScreenFlags);
	}
	else	//success!  update the screen dimensions
	{
		screenWidth  = width;
		screenHeight = height;
	}

	initOpenGL();
	images.refreshTextures();
}

static void cleanup_sdl(void) {
	SDL_Quit();
}

void Graphics::initSDL(int width, int height)
{
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_NOPARACHUTE) < 0)
	{
		printf("Couldn't initialize SDL!");
		exit(1);
	}

	atexit(cleanup_sdl);

	SDL_GL_SetAttribute(SDL_GL_RED_SIZE,	 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE,	 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE,	 8);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE,	 0);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	
	int flags = SDL_OPENGL;
	if(fullScreen) flags |= SDL_FULLSCREEN;
	
	screen = SDL_SetVideoMode(
		width, 
		height, 
		0, 
		flags);

	if (screen == 0)
	{
//		assert(false);
		exit(1);
	}
}

void Graphics::initOpenGL()
{
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glShadeModel(GL_SMOOTH);
	glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);	

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClearDepth(1.0f);	
	
	//the screen surface should be loaded by here!
	glViewport(
		0, 
		0, 
		screenWidth,
		screenHeight
		);

	glMatrixMode(GL_PROJECTION);
    glOrtho(0, screenWidth, screenHeight, 0, -1, 1 );
    glMatrixMode( GL_MODELVIEW );
    glLoadIdentity();
}

//(leave SDL_Quit() to the main program)
void Graphics::shutDown()
{
}

void Graphics::drawImage(
	const Image *image, 
	float x, 
	float y, 
	float angleRadians, 
	float scale)
{
	//TODO:  make these static constants
	static const Color colorWhite = Color(255, 255, 255);
	setColor(colorWhite);
	drawColoredImage(
		image, 
		x, 
		y, 
		angleRadians, 
		scale);
}

//this does not make any call to glColor(), so you must use setColor()
//to set the appropriate shading color before calling this function
void Graphics::drawColoredImage(
	const Image *image, 
	float x, 
	float y, 
	float angleRadians, 
	float scale)
{
	glBindTexture(GL_TEXTURE_2D, image->getTexID());

	float angleInDegrees = (angleRadians * 180) / M_PI;

	//apply some transformations first
	glPushMatrix();
	glTranslatef(x, y, 0.0f);
	glScalef(scale/2, scale/2, 1);
	glRotatef(angleInDegrees, 0.0f, 0.0f, -1.0f);
	
	float width = image->getWidth(),
	      height = image->getHeight();

	//start in top right vertex and go counter-clockwise
	glBegin(GL_QUADS);
		glTexCoord2f(0.0f, 0.0f); glVertex2f(-width, -height);
		glTexCoord2f(1.0f, 0.0f); glVertex2f( width, -height);
		glTexCoord2f(1.0f, 1.0f); glVertex2f( width,  height);
		glTexCoord2f(0.0f, 1.0f); glVertex2f(-width,  height);
	glEnd();

	glPopMatrix();

	//unbind the texture
	glBindTexture(GL_TEXTURE_2D, 0);	
}

void Graphics::drawPoint(
	const Color &color, 
	float x, 
	float y, 
	float size)
{
	setColor(color);

	glPointSize(size);
	glBegin(GL_POINTS);
		glVertex2f(x, y);
	glEnd();
}

//you have to manually call glBegin(GL_POINTS) and glEnd() for this one!
//(useful for drawing the starfield)
void Graphics::drawVertex(
	float x, 
	float y)
{
	glVertex2f(x, y);
}
	

void Graphics::drawLine(
	const Color &color, 
	float x1, 
	float y1, 
	float x2,
	float y2, 
	float width)
{
	setColor(color);

	glLineWidth(width);
	glBegin(GL_LINES);
		glVertex2f(x1, y1);
		glVertex2f(x2, y2);
	glEnd();
}

void Graphics::drawRect(
	const Color &color, 
	float xLeft, 
	float yTop, 
	float xRight, 
	float yBottom)
{
	setColor(color);

	//start with the top right corner and go counter-clockwise
	glBegin(GL_QUADS);
		glVertex2f(xRight, yTop);
		glVertex2f(xRight, yBottom);
		glVertex2f(xLeft,  yBottom);
		glVertex2f(xLeft,  yTop);
	glEnd();
}

void Graphics::drawText(const char *text, float x, float y, void *font)
{
	glColor3ub(255,255,255);
	glRasterPos2f(x, y);
	int length = (int) strlen(text);
	for (int i = 0; i < length; i++)
		glutBitmapCharacter(font, text[i]);
}

void Graphics::drawTextN(const char *text, int maxlen, float x, float y, void *font)
{
	glColor3ub(255,255,255);
	glRasterPos2f(x, y);
	int length = (int) strlen(text);
	if(length>maxlen) length = maxlen;
	for (int i = 0; i < length; i++)
		glutBitmapCharacter(font, text[i]);
}

void Graphics::drawTextCentered(const char *text, float x, float y, void *font)
{
	drawText(text, x - drawTextWidth(text,font)/2, y, font);
}

int Graphics::drawTextWidth(const char *text, void *font)
{
	int ret=0;
	for(int ii=0; text[ii]; ii++)
		ret += glutBitmapWidth(font, text[ii]);
	return ret;
}

int Graphics::drawCharWidth(char c, void *font)
{
	return glutBitmapWidth(font, c);
}


//tells OpenGL to change the color (for drawing a primitive)
//saves some typing at least
void Graphics::setColor(const Color &color)
{
	glColor4ub(color.red, color.green, color.blue, color.alpha);
}

void Graphics::clearScreen()
{
	//since we only clear to black, and because I'd rather not do these 
	//divisions, let's just clear the screen to black by default...
	//glClearColor(
	//	color.red / 255, 
	//	color.green / 255, 
	//	color.blue / 255, 
	//	color.alpha / 255);
	glClear(GL_COLOR_BUFFER_BIT);
	glLoadIdentity();
}

void Graphics::swapBuffers()
{
	SDL_GL_SwapBuffers();
}


#include "mastrix.hpp"
#include "hawkthreads.h"
#include <time.h>

// Ugly hack: Include GameX's inline timer functions, because GameX won't
// export usable timing data.
//#include "gamex-win-dx-timer.inl"

// TODO: Smooth out frame-to-frame variations in dt by using a 10-frame or so rolling average

static double currentTime = 0;
static double realTime = 0;
static float  dt = 0;
static double currentFramerate  = 0;
static double returnedFramerate = 0;
static double currentFrametime  = 0;

typedef std::deque<float> ServerTimeOffsetList;
ServerTimeOffsetList serverTimeOffsets;

ClientConsoleVar<bool> showDts("show_dts", false);

void updateTime(void)
{
	double prevTime = realTime;
	realTime = (double)SDL_GetTicks() / 1000;

	if (!gameFrozen) {
		currentTime = realTime - FrozenTime;
	}
	
	if(prevTime != 0) { // If this isn't the first frame (which has some rediculous dt)
		if(realTime >= prevTime) // And the time increased (like it always should)
			dt = realTime - prevTime; // Set dt
		else 
			dt = 0; // Else fall back to dt=0 (something like clock-wraparound happened.)
		
		if(dt > 0.2) dt = 0.2;
	}
	
	//update the framerate counter
	if ((double)SDL_GetTicks() - currentFrametime >= 1000)
	{
		returnedFramerate = currentFramerate;
		currentFramerate  = 0;
		currentFrametime  = SDL_GetTicks();

	}
	else
		currentFramerate++;
	
	float offset = 0;
	if(serverTimeOffsets.size())
	{
		for(ServerTimeOffsetList::iterator ii=serverTimeOffsets.begin(); ii!=serverTimeOffsets.end(); ii++)
			offset += *ii;
		offset /= serverTimeOffsets.size();
		currentTime += offset;
	}
}

CLIENT_CONSOLE_COMMAND(showtime)
{
	console.printf("Time is %f/%f", (float)currentTime, realTime);
}

void syncTime(float time)
{
	serverTimeOffsets.push_back(time - realTime);
	if(serverTimeOffsets.size() > 30)
		serverTimeOffsets.pop_front();
}

double getFrameRate()
{
	return returnedFramerate;
}

/// Get the current time, in seconds since some arbitrary period.
double getTime(void)
	{ return currentTime; }

/// Get the time since the last frame, in seconds.
float getDt(void)
{ 
	if (gameFrozen)
		return 0;
	else return dt;
}

/// Return dt, even if the game is frozen.
float getRealDt(void)
{
	return dt;
}
double getRealTime(void)
{
	return realTime;
}

/// Get the clamped angle (between -2pi and 2pi radians)
float getClampedAngleRad(float angleInRadians)
{
	bool wasOriginalAngleNegative = false;
	if (angleInRadians < 0)
		wasOriginalAngleNegative = true;

	float absAngle = (float)fabs(angleInRadians);

	const float angleWraparound = (M_PI * 2);
	while (absAngle > (angleWraparound))
	{
		absAngle -= (angleWraparound);
	}

	if (wasOriginalAngleNegative)
		absAngle *= -1;

	return absAngle;
}

/// Get the squared distance between two positions
float distSquaredBetween(
	const Position &point1, 
	const Position &point2)
{
	return distSquaredBetween(
		point1.getX(), 
		point1.getY(), 
		point2.getX(), 
		point2.getY());
}

/// Get the squared distance between two pairs of points
float distSquaredBetween(
	float x1, 
	float y1, 
	float x2, 
	float y2)
{
	float xDifference = x2 - x1;
	float yDifference = y2 - y1;

	return ((xDifference * xDifference) + (yDifference * yDifference));
}

/// Get the angle between two vectors
float getAngleBetween(
	const Vector2D &vector1, 
	const Vector2D &vector2)
{
	float vector1Mag = vector1.getMagnitude();
	float vector2Mag = vector2.getMagnitude();

	//prevent divide by zero (this is ok because an angle bisector with 
	//components (0, 0) means you shouldn't turn anyway)
	if (vector1Mag == 0.0f || vector2Mag == 0.0f)
		return 0.0f;

	float angleBetween = (float)acos((vector1.dotProduct(vector2)) / 
									 (vector1.getMagnitude() * 
									  vector2.getMagnitude()));

	return angleBetween;
}

std::string retprintf(const char *fmt, ...)
{
	char buf[512] = "";
	va_list args;
	va_start(args, fmt);
		vsnprintf(buf, 512, fmt, args);
	va_end(args);
	return std::string(buf);
}

const char *getNlError(void)
{
#ifndef DISABLE_NETWORKING
	int errCode = nlGetError();
	
	if(errCode == NL_SYSTEM_ERROR)
		return nlGetSystemErrorStr( nlGetSystemError() );
	else
		return nlGetErrorStr(errCode);
#endif
}

float getHeading(const Position& source, const Position& dest)
{
	float dx = dest.getX() - source.getX(),
	      dy = dest.getY() - source.getY();
	return atan2(-dy, dx);
}


void errorDialog(const char *msg)
{
	fprintf(stderr, "%s", msg);
}


std::string quotify(std::string str)
{
	std::string ret;
	
	for(unsigned ii=0; ii<str.length(); ii++)
	{
		switch(str[ii])
		{
			case '\"':
			case '\'':
			case '\n':
			case '{':
			case '}':
				ret += '\\';
				/* FALLTHROUGH */
			default:
				ret += str[ii];
				break;
		}
	}
	return ret;
}


const char *buttonName(int button)
{
	switch(button)
	{
		default:
		case SDL_BUTTON_LEFT: return "mouse1";
		case SDL_BUTTON_RIGHT: return "mouse2";
		case SDL_BUTTON_MIDDLE: return "mouse3";
		case SDL_BUTTON_WHEELUP: return "mwheelup";
		case SDL_BUTTON_WHEELDOWN: return "mwheeldown";
	}
}



/*
 * Screenshot function by Ray Kelm  sdl@lokigames.com.
 * Posted 31 Aug 2000 on SDL mailing list.
 * http://www.libsdl.org/pipermail/sdl/2000-September/030013.html
 */
int Screenshot(const char *filename)
{
	SDL_Surface *screen = graphics.getScreenBuffer();
	SDL_Surface *temp;
	unsigned char *pixels;
	int i;
	
	temp = SDL_CreateRGBSurface(SDL_SWSURFACE, screen->w, screen->h, 24,
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
	0x000000FF, 0x0000FF00, 0x00FF0000, 0
#else
	0x00FF0000, 0x0000FF00, 0x000000FF, 0
#endif
	);
	if (temp == NULL)
		return -1;

	pixels = (unsigned char*)malloc(3 * screen->w * screen->h);
	if (pixels == NULL)
	{
		SDL_FreeSurface(temp);
		return -1;
	}

	glReadPixels(0, 0, screen->w, screen->h, GL_RGB, GL_UNSIGNED_BYTE, pixels);

	for (i=0; i<screen->h; i++)
		memcpy(((char *) temp->pixels) + temp->pitch * i, pixels + 3*screen->w * (screen->h-i-1), screen->w*3);
	free(pixels);

	SDL_SaveBMP(temp, filename);
	SDL_FreeSurface(temp);
	return 0;
}

CLIENT_CONSOLE_COMMAND(screenshot)
{
	static int screenshotNumber = 1;
	Screenshot( retprintf("screen%i.bmp", screenshotNumber++).c_str() );
}
#ifndef MASTRIX_HPP
#define MASTRIX_HPP

#ifdef MACOS
#include "GLUT/glut.h"
#else
#include "GL/glut.h"
#endif
#include "SDL.h"
#include "SDL_image.h"
#ifndef DISBALE_NETWORK
#include "nl.h"
#endif
//#include "gamex.hpp"

#include "msvcfix.hpp"
#include <cstdlib>
#include <cmath>
#include <cassert>
#include <cstdarg>
#include <string>

#include "float.h"

#include "graphics.hpp"
#include "audio.hpp"
#include "resourcepool.hpp"
#include "command.hpp"
#include "cvar.hpp"
#include "message.hpp"
#include "console.hpp"
#include "position.hpp"
#include "cl_entity.hpp"
#include "client.hpp"
#include "UI.hpp"
#include "particlepool.hpp"
#include "sv_entity.hpp"
#include "shot.hpp"
#include "shiptype.hpp"
#include "player.hpp"
#include "humanplayer.hpp"
#include "aipathfinding.hpp"
#include "aisteering.hpp"
#include "aiplayer.hpp"
#include "mapmarker.hpp"
#include "server.hpp"
#include "protocol.hpp"
#include "keybuffer.hpp"
#include "keyboard.hpp"
#include "scripting.hpp"
#include "clientedge.hpp"
#include "events.hpp"
#include "weapons.hpp"
#include "mine.hpp"
#include "powerup.hpp"

//mouse functions
static inline int getMouseX()
{		
	static int x;
	SDL_GetMouseState(&x, 0);
	return x;	
}

static inline int getMouseY()
{
	static int y;
	SDL_GetMouseState(0, &y);
	return y;
}

//timer functions
void updateTime(void);
void syncTime(float time);
double getTime(void);
float getDt(void);
float getRealDt(void);
double getRealTime(void);
double getFrameRate();

extern bool needSinglePlayerInit;

extern bool gameFrozen;
extern double FrozenTime;
extern double unFrozenTime;

//utility functions
static inline float randFloat(float min, float max)
	{ return min+((float)rand()*(max-min)/(float)RAND_MAX); }
static inline int randInt(int min, int max)
{ 
	if(min>max)
	{
		int temp = max; 
		max=min; 
		min=temp;
	} 
	
	return min+(rand()%(max-min+1));
}

float getClampedAngleRad(float angleInRadians);

float distSquaredBetween(
	const Position &point1, 
	const Position &point2);

float distSquaredBetween(
	float x1, 
	float y1, 
	float x2, 
	float y2);

float getAngleBetween(
	const Vector2D &vector1, 
	const Vector2D &vector2);

float getHeading(const Position& source, const Position& dest);

std::string retprintf(const char *fmt, ...);
void errorDialog(const char *msg);
std::string quotify(std::string str);
const char *buttonName(int button);

//const int screenWidth  = 1024;
//const int screenHeight = 768;

//network functions
void initNetwork(void);
void initNetTarget(NetworkNode *node);
void connectToServer(void);
void acceptConnections(void);
const char *getNlError(void);

extern bool networkServer;
extern bool networkDedicatedServer;
extern bool networkClient;
extern const char *networkAddress;
extern int serverPort;
extern NLsocket serverSock;

#endif

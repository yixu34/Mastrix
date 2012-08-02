#ifndef CLIENT_HPP
#define CLIENT_HPP

#include "cvar.hpp"
#include "command.hpp"
#include "message.hpp"
#include "cl_entity.hpp"
#include "ui.hpp"
#include "particlepool.hpp"
#include "clientedge.hpp"
#include <map>
#include <vector>

//class MusicX;
//extern MusicX *music;

class GameNode
	: public NetworkNode
{
public:
	GameNode(VarPool *_cvars, ConsoleCommandPool *_commands)
		: cvars(_cvars), commands(_commands) { }
	virtual ~GameNode() { }
	VarPool *cvars;
	ConsoleCommandPool *commands;
	virtual const Position *getEntityPosition(int entid) = 0;
	virtual int getNodeId(void) = 0;
	virtual void timepass(void) = 0;
	
	virtual bool isConnected(void) { return true; }
};


struct ClientViewport
{
	ClientViewport(
		float left, 
		float right, 
		float top, 
		float bottom, 
		int   width, 
		int   height);
	void setClip(void) const;
	void reset(
		float left, 
		float right,
		float top, 
		float bottom, 
		int   width, 
		int   height);
	
	float left, right, top, bottom;
	float center_x, center_y;
};
extern const ClientViewport viewport_full;

struct Starpoint
{
	float x;
	float y;
	float depth;
	int bright;
};

class Starfield
{
public:
	Starfield();
	void draw(const Position &camera, const ClientViewport *viewport);
	void reinit(void);
	
private:
	int numSmallStars;
	int numLargeStars;
	
	typedef std::vector<Starpoint> StarPool;
	StarPool smallStars;
	StarPool largeStars;

};

void assignViewports(void);

class Client
	: public GameNode
{
public:
	Client(int index);
	~Client();
	void connect(void);
	void timepass(void);
	void redraw(void);
	void setViewport(const ClientViewport *view) { viewport=view; }
	const ClientViewport *getViewport(void) { return viewport; }
	Position getCamera(void) { return camera; }
	void setAvatar(int ID) { avatarID = ID; }
	
	const Position *getEntityPosition(int entid)
		{ if(entpool.find(entid)!=entpool.end()) return &entpool[entid]->getPosition(); else return NULL; }

	int getNodeId(void) { return index; };
	bool isConnected(void) { return connected; }
	
	void select(void);     ///< Select the entity under the cursor
	void select(int entID);///< Select the given entID
	
	inline void hideScoreboard(void) { showScoreboard = false; }

	void convertToDrawCoords(float &x, float &y);

	void resetViewports(int width, int height);
	
	float fadeSpeed, targetFade, fadeOut;
	
	// Menu
	bool showMenu;
	std::string menuText;
	std::vector<std::string> menuCommands;
	
	Starfield stars;

protected:
	int  index;
	bool connected;
	bool connectionComplete;
	bool areEdgesVisible;
	typedef std::map<int, CLEntity *> CLEntityPool;
	CLEntityPool entpool;
	ParticlePool particlePool;
	typedef std::list<ClientEdge> CLEdgePool;
	CLEdgePool edges;
	int avatarID;

	int selectedEntId;
	int shipEntId;

	Position cameraCenter, camera;
	float camera_bound_left, camera_bound_right, camera_bound_top, camera_bound_bottom;
	UI	  userInterface;
	const ClientViewport *viewport;
	
	// Scoreboard
	bool showScoreboard;
	std::string scoreText;
	
	// Rectangle display
	struct RegionRect {
		std::string name;
		float min_x, max_x, min_y, max_y;
	};
	std::vector<RegionRect> rectpool;
	
	// Misc functions
	void drawEnt(CLEntity *ent);
	void drawWaypointEdges();
	void drawRects();
	void drawMapBorders();
	void drawShield(int entID, float radius, float &time, int type);

	// Message handlers
	void serverDisconnect(RecvMessage &msg);
	void runCommand(RecvMessage &msg);
	void addEntity(RecvMessage &msg);
	void dropEntity(RecvMessage &msg);
	void updateEntity(RecvMessage &msg);
	void centerCamera(RecvMessage &msg);
	void print(RecvMessage &msg);
	void playSoundAt(RecvMessage &msg);
    void playSound(RecvMessage &msg);
	void setEngines(RecvMessage &msg);
	void explodeAt(RecvMessage &msg);
	void updateUI(RecvMessage &msg);
	void changeColor(RecvMessage &msg);
	void restoreColor(RecvMessage &msg);
	void updateScoreboard(RecvMessage &msg);
	void addEdge(RecvMessage &msg);
	void dropEdge(RecvMessage &msg);
	void showAllEdges();
	void hideAllEdges();
	void clearEdges();
	void setRectangles(RecvMessage &msg);
    void drawLightning(RecvMessage &msg);
	void setDrawShield(RecvMessage &msg);
    void markAsAlly(RecvMessage &msg);
	void updateDrawEffect(RecvMessage &msg);
};

typedef std::map<int, Client*> ClientPool;
extern ClientPool clients;
extern GameNode *currentNode;

extern ClientConsoleVar<bool> fullScreen;
extern float zoom;
extern ClientConsoleVar<float> targetZoom;

#endif

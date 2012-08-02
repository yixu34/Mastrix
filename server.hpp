#ifndef SERVER_HPP
#define SERVER_HPP

#include "sv_entity.hpp"

class Player;
class ShipType;


struct CollisionData {
	int entid;
	float radius;
	float x, y;
	bool rectangular;
	float width, height, rotation;
};


class MapCycle
{
public:
	MapCycle();
	std::string getNext(void);
	void addLevel(const char *lvl);
	void removeLevel(const char *lvl);
	void clear(void);
protected:
	std::vector<std::string> levels;
	int pos;
};


class Server
	: public GameNode
{
public:
	Server();

	int getNodeId(void) { return -1; };
	void timepass(void);
	int registerEntity(Entity *ent);
	void registerAvatar(int clientID, int entID);
	HumanPlayer *getAvatar(int clientID);
	void deleteAvatar(int clientID);
	void spawnPlayer(Player *player);
	void addDeath(int clientID, int killerID, int victimID);
	void silentDeath(int clientID);
	void startGame(void);
	void spawnPlayers(void);
	bool testSphereCollision(Entity *ent1, Entity *ent2);
	void inflict_gravity(void);
	void collide(Entity*, Entity*);
	void semiElasticBounce(Entity *a, Entity *b, float elasticity);
	const Position *getEntityPosition(int entid) { return &entities[entid]->getPosition(); }
	void checkEvents(void);
	void notifyClient(int clientID); ///< Send 'add' message for all entities to client (for late join)
	void keepEntityInBounds(Entity *ent);
	void regulateSpeed(Entity *ent);

    void markAsAlly(Entity *ent);

	void highlightEntity(Entity *ent);
	void unhighlightEntity(Entity *ent);

    void playSoundAt(const std::string &soundName, const Position &sourcePos);
    void playSound(const std::string &soundName);

	void unvisitAllWaypoints();
	WaypointMarker *getWaypointNearestTo(const Position &pos);

	bool isShipTypeDeclared(const std::string &shipTypeName);

	//containers
	typedef std::map<int, Entity*> Entpool;
	Entpool entities;
	
	struct ClientInfo
	{
		std::string name;
		int entID;
		int score;
		int color;
		team_type team;
	};

	typedef std::map<int, ClientInfo> ClientInfoPool;
	ClientInfoPool clientInfo;

	typedef std::vector<SpawnpointMarker*> SpawnPool;
	SpawnPool spawns;
	
	typedef std::vector<WaypointMarker *> WaypointPool;
	WaypointPool waypoints;

	typedef std::map<std::string, ShipType *> ShipTypePool;
	ShipTypePool shipTypes;

	bool areSpawnsMarked;
	bool areWaypointsMarked;
	
	// Trigger: Regions
	struct Region {
		std::string name;
		int min_x, max_x, min_y, max_y;
	};
	typedef std::vector<Region> RegionPool;
	RegionPool regions;
	void refreshRegionDisplay();
	Region findRegion(std::string name);
	
	// Trigger: Timers
	struct Timer {
		std::string name;
		float fireTime;
	};
	typedef std::vector<Timer> TimerPool;
	TimerPool timers;
	
	int selectedEntity;

	typedef std::pair<int, double>  RespawnData;	//clientID-time of death
	typedef std::deque<RespawnData> RespawnPool;
	RespawnPool respawns;
	
	void reassignTeams(void);
	void giveFlagTo(team_type team);
	
	std::map<team_type, int> teamScores;
	
	float levelRunTimer;
	bool levelDone;
	float levelEndTimer;
	void nextLevel(void);
	
	MapCycle cycle;
	std::string getGameResult(void);
	
protected:
	void checkCollisions(void);
	void getCollisionData(Entity *ent, CollisionData *dat);
	bool checkCollisionBetween(CollisionData *a, CollisionData *b);
	void findEntsWithLabel(std::string label, std::vector<int> &results);
	Position findFreeSpotWithLabel(std::string label);
	void checkLevelEnd(void);
	int newPlayerColor(void);
	int countPlayersWithColor(int color);
	
	team_type newPlayerTeam(int clientID);
	int countPlayersOnTeam(team_type team);
	
	int maxEntId;
};
extern Server *server;


enum {
	PRINT_CONSOLE = 1,
	PRINT_CHAT    = 2,
};
void printfToClient(int who, const char *fmt, ...);
void printToClient(int who, int where, const char *str);
void messageToClient(int who, const char *fmt, ...);

extern ServerConsoleVar<float> leftBorder;
extern ServerConsoleVar<float> rightBorder;
extern ServerConsoleVar<float> topBorder;
extern ServerConsoleVar<float> bottomBorder;
#endif	//SERVER_HPP

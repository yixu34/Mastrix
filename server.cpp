#include "mastrix.hpp"
#include <algorithm>
#include <float.h>

using namespace std;

ServerConsoleVar<float> respawnDelay("respawn_delay", 1.0f);

Server::Server()
	: GameNode(serverCvars, serverCommandPool)
{
	maxEntId           = 1;
	selectedEntity     = -1;
	areSpawnsMarked    = false;
	areWaypointsMarked = false;
	levelDone          = false;
}

void Server::startGame(void)
{
	checkEvents();
	
	for(ClientInfoPool::iterator ii=clientInfo.begin(); ii!=clientInfo.end(); ii++)
	{
		int clientID = ii->first;
		Player *currentPlayer = getAvatar(clientID);
		if(currentPlayer)
			spawnPlayer(currentPlayer);
	}
}

void Server::spawnPlayers(void)
{
	for(ClientInfoPool::iterator ii=clientInfo.begin(); ii!=clientInfo.end(); ii++)
	{
		int clientID = ii->first;
		Player *currentPlayer = getAvatar(clientID);
		respawns.clear();
		if(currentPlayer)
			spawnPlayer(currentPlayer);
		else
			spawnPlayer( new HumanPlayer(clientID, clientInfo[clientID].name) );
	}
}

void Server::notifyClient(int clientID)
{
	for (Entpool::iterator ii=entities.begin(); ii != entities.end(); ii++)
	{
		ii->second->sendAdd(clientID);
	}
}


int Server::registerEntity(Entity *ent)
{
	int entid = maxEntId++;
	entities[entid] = ent;
	return entid;
}

void Server::registerAvatar(int clientID, int entID)
{
	clientInfo[clientID].entID = entID;
}

HumanPlayer *Server::getAvatar(int clientID)
{
	if(clientInfo.find(clientID) != clientInfo.end())
	{
		int entID = clientInfo[clientID].entID;
		if(entID >= 0)
			return (HumanPlayer*)entities[ clientInfo[clientID].entID ];
		else
			return NULL;
	} else
		return NULL;
}

void Server::deleteAvatar(int clientID)
{
	clientInfo[clientID].entID = -1;
}

void Server::spawnPlayer(Player *player)
{
	std::vector<SpawnpointMarker*> validPositions;
	SpawnpointMarker *bestSpawn;
	
	for(SpawnPool::iterator ii = spawns.begin(); ii != spawns.end(); ++ii)
	{
		bool isSpawnOK = true;
		
		for (Entpool::iterator jj = entities.begin(); jj != entities.end(); ++jj)
		{
			if(jj->second->getEntId() == (*ii)->getEntId())
				continue;
			
			Vector2D displacement = (jj->second->getPosition().positionVector() - (*ii)->getPosition().positionVector());
			
			if (displacement.getMagnitude() < 40.0f) {
				isSpawnOK = false;
				break;
			}
		}
		if((*ii)->team != team_free && (*ii)->team != player->getTeam())
			isSpawnOK = false;
		
		if (isSpawnOK)
			validPositions.push_back(*ii);
	}
	if(spawns.size() == 0)
	{
		player->setPosition(Position(0,0));
	}
	else if(validPositions.size() == 0)  // All spawn points are filled, so pick
	{                               // one at random. - This causes ships to spawn on top of each other [eric]
		bestSpawn = spawns[ rand()%spawns.size() ];
		player->setPosition(bestSpawn->getPosition());
	}
	else
	{
		bestSpawn = validPositions[ rand()%validPositions.size() ];
		player->setPosition(bestSpawn->getPosition());
	}

}

void printfToClient(int who, const char *fmt, ...)
{
	char buf[512] = "";
	va_list args;
	va_start(args, fmt);
		vsnprintf(buf, 512, fmt, args);
	va_end(args);
	printToClient(who, PRINT_CONSOLE,  buf);
}
void printToClient(int who, int where, const char *str)
{
	SendMessage msg(message_print);
		msg << str;
		msg.putShort(where);
	if(who == -1)
		msg.sendToEveryone();
	else
		msg.sendToClient(who);
}
void messageToClient(int who, const char *fmt, ...)
{
	char buf[512] = "";
	va_list args;
	va_start(args, fmt);
		vsnprintf(buf, 512, fmt, args);
	va_end(args);
	printToClient(who, PRINT_CHAT,  buf);
}



void Server::checkEvents(void)
{
	MessageGroup *messages = getMessages();
	RecvMessage *message;
	
	while( (message=messages->next()) )
	{
		switch(message->getType())
		{
		case message_connect: {
			notifyClient(message->getSource());
			printfToClient(-1, "A new player has joined the game.");
			int clientID = message->getSource();
			clientInfo[clientID].name = "unnamed";
			clientInfo[clientID].score = 0;
			clientInfo[clientID].team = newPlayerTeam(clientID);
			clientInfo[clientID].color = -1;
			clientInfo[clientID].color = newPlayerColor();
			spawnPlayer( new HumanPlayer(clientID, clientInfo[clientID].name) );
			
			break;
			}
		case message_disconnect: {
			int clientID = message->getSource();
			if(getAvatar(message->getSource()))
				getAvatar(message->getSource())->die(-1);
			messageToClient(-1, "%s has left the game.", clientInfo[clientID].name.c_str());
			
			clientInfo.erase(clientID);
			break;
			}
		case message_name: {
			std::string newname = message->getString();
			int clientID = message->getSource();
			if(clientInfo[clientID].name != "unnamed")
				messageToClient(-1, "%s changed name to %s", clientInfo[clientID].name.c_str(), newname.c_str());
			clientInfo[clientID].name = newname;
			if(getAvatar(message->getSource()))
				getAvatar(message->getSource())->setName(newname);
			break;
			}
		case message_command:
			serverScripting.command(*message);
			break;
		default:
			break;
		}
	}
	
	delete messages;
}


void Server::timepass(void)
{
	checkEvents();

	Entpool::iterator temp;
	Entpool::iterator jj;
	
	if(!levelDone)
	{
		// Send time sync
		SendMessage timeSync(message_sync_time);
			timeSync.putFloat(getTime());
		timeSync.sendToEveryone();
		
		for(Entpool::iterator ii=entities.begin(); ii!=entities.end(); ii++)
		{
			ii->second->timepass();
			regulateSpeed(ii->second);
		}
		
		// Check timers
		float currentTime = getTime();
		for(unsigned ii=0; ii<timers.size(); ii++)
		{
			if(currentTime > timers[ii].fireTime)
			{
				std::string triggerName = timers[ii].name;
				timers[ii--] = timers[timers.size()-1];
				timers.resize(timers.size()-1);
				serverScripting.defineVariable(triggerName, "", tag_map_temporary);
			}
		}
		
		for (RespawnPool::iterator ii = respawns.begin(); ii != respawns.end(); ++ii)
		{
			int    clientID	   = ii->first;
			double timeOfDeath = ii->second;

			if (getTime() - timeOfDeath >= respawnDelay &&
				clientInfo.find(ii->first) != clientInfo.end())
			{
				spawnPlayer(new HumanPlayer(clientID, clientInfo[clientID].name));
				ii = respawns.erase(ii);
				ii--;
			}
		}
		
		// Check for collisions and apply gravity
		checkCollisions();
		if(gravConst != 0.0) inflict_gravity();
		
		// Delete entities marked as 'should delete'
		Entpool::iterator next;
		for(Entpool::iterator ii=entities.begin(); ii!=entities.end(); ii=next) {
			if(ii->second->shouldDelete()) {
				if(ii->first == selectedEntity)
					selectedEntity = -1;
				next = ii; next++;
				delete ii->second;
				entities.erase(ii);
			} else
				next = ++ii;
		}
		
		if (!gameFrozen) for(Entpool::iterator ii=entities.begin(); ii!=entities.end(); ii++)
		{
			keepEntityInBounds(ii->second);
		}
		
		if(serverScripting.shouldWalkTriggers)
			serverScripting.walkTriggers();
		
		// Clear temporary variables
		serverScripting.clearTaggedVariables(tag_frame_temporary);
		
		levelRunTimer += getDt();
		checkLevelEnd();
	}
	else
	{
		levelEndTimer -= getDt();
		if(levelEndTimer <= 0)
			nextLevel();
	}
}

void Server::playSoundAt(
	const std::string &soundName, 
	const Position &sourcePos)
{
	SendMessage soundMsg(message_sound_3D);
	    soundMsg.putString(soundName.c_str());
	    soundMsg.putFloat(sourcePos.getX());
	    soundMsg.putFloat(sourcePos.getY());
	soundMsg.sendToEveryone();
}

void Server::playSound(const std::string &soundName)
{
    SendMessage soundMsg(message_sound);
        soundMsg.putString(soundName.c_str());
    soundMsg.sendToEveryone();
}

void Server::keepEntityInBounds(Entity *ent)
{
	if(!ent->staysInBounds())
		return;
	
	bool boundaryHit = false;
	Position &entPos	  = ent->getPosition();
	Position entVel		  = Position(entPos.getX_vel(), entPos.getY_vel());
	Position nextFramePos = entPos;
	nextFramePos.setX(nextFramePos.getX() + (entVel.getX() * getDt()));
	nextFramePos.setY(nextFramePos.getY() + (entVel.getY() * getDt()));

	float entRadius		  = ent->getRadius();

	if (entPos.getX()		- entRadius <= leftBorder || 
		nextFramePos.getX() - entRadius <= leftBorder)
	{
		entPos.setX_vel(entPos.getX_vel() * -0.1);
		entPos.setX(leftBorder + entRadius);
		boundaryHit = true;
	}

	else if (entPos.getX()		 + entRadius >= rightBorder ||
			 nextFramePos.getX() + entRadius >= rightBorder)
	{
		entPos.setX_vel(entPos.getX_vel() * -0.1);
		entPos.setX(rightBorder - entRadius);
		boundaryHit = true;
	}
	
	if (entPos.getY()		 - entRadius <= topBorder || 
		nextFramePos.getY()  - entRadius <= topBorder)
	{
		entPos.setY_vel(entPos.getY_vel() * -0.1);
		entPos.setY(topBorder + entRadius);
		boundaryHit = true;
	}

	else if (entPos.getY()		 + entRadius >= bottomBorder || 
			 nextFramePos.getY() + entRadius >= bottomBorder)
	{
		entPos.setY_vel(entPos.getY_vel() * -0.1);
		entPos.setY(bottomBorder - entRadius);
		boundaryHit = true;
	}
	
	if(ent->isHuman() && boundaryHit)
	{
		serverScripting.defineVariable("edge_hit", retprintf("%i", ent->getEntId()), tag_frame_temporary);
	}
}

void Server::regulateSpeed(Entity *ent)
{
	float entVelX		  = ent->getPosition().getX_vel();
	float entVelY         = ent->getPosition().getY_vel();
	float entSpeedSquared = ent->getSpeedSquared();

	float entMaxSpeed     = ent->getMaxSpeed();

	//moving too fast; shorten the velocity vector to maxSpeed
	if (entSpeedSquared > entMaxSpeed * entMaxSpeed)
	{
		Vector2D oldVel = Vector2D(entVelX, entVelY);
		oldVel.normalize();
		oldVel *= entMaxSpeed;
		ent->getPosition().setX_vel(oldVel.x);
		ent->getPosition().setY_vel(oldVel.y);
	}
}

//find the closest waypoint to a given position
WaypointMarker *Server::getWaypointNearestTo(const Position &pos)
{
	WaypointMarker *nearestWaypoint = 0;

	float minDistSquared = FLT_MAX;

	WaypointPool::iterator ii;
	for (ii = server->waypoints.begin(); ii != server->waypoints.end(); ++ii)
	{
		float distSquared = distSquaredBetween(pos, ((*ii)->getPosition()));

		if (distSquared <= minDistSquared)
		{
			minDistSquared  = distSquared;
			nearestWaypoint = (*ii);
		}
	}

	return nearestWaypoint;
}

SERVER_CONSOLE_COMMAND(say)
{
	std::string output("");
	
	if(who>=0) {
		output = server->clientInfo[who].name + ": ";
	}
	
	for(int ii=0; ii<argc; ii++) {
		if(ii>0) output += " ";
		output += argv[ii];
	}
	messageToClient(-1, "%s", output.c_str());
}

SERVER_CONSOLE_COMMAND(team_say)
{
	std::string output("");
	
	if(who>=0) {
		output = std::string("(team) ") + server->clientInfo[who].name + ": ";
	}
	
	for(int ii=0; ii<argc; ii++) {
		if(ii>0) output += " ";
		output += argv[ii];
	}
	
	team_type myTeam = server->clientInfo[who].team;
	
	for(Server::ClientInfoPool::iterator ii=server->clientInfo.begin(); ii!=server->clientInfo.end(); ii++)
	{
		if(ii->second.team == myTeam)
			messageToClient(ii->first, "%s", output.c_str());
	}
}

#if 0
SERVER_CONSOLE_COMMAND(radio_say)
{
	if (argc < 1)
	{
		printfToClient(who, "Not enough arguments to radio say (usage:  radio_say command)");
		return;
	}

	//radioSoundFile is the name of the file w/out the extension, and
	//radioSoundFilename is the name of the file WITH the extension
	std::string radioText("");
	std::string radioSoundFile     = argv[0];
	std::string radioSoundFilename = "sounds/";
	radioSoundFilename += radioSoundFile + ".wav";

	if (who >= 0)
	{
		radioText  = server->clientInfo[who].name + " (radio): ";
		radioText += radioSoundFile;
		
		//find and play the corresponding sound, if possible
		//server->playSoundAt(radioSoundFilename, Position());
		server->playSoundAt(radioSoundFilename, Position());
	}	

    messageToClient(-1, "%s", radioText.c_str());
}
#endif

void clearLevel(void)
{
	for(Server::Entpool::iterator ii=server->entities.begin(); ii!=server->entities.end(); ii++)
	{
		Entity *ent = ii->second;
		if(!ent->isHuman())
			ent->deleteMe();
	}
	server->spawns.clear();
	server->waypoints.clear();
	server->regions.clear();
	server->refreshRegionDisplay();
	server->timers.clear();
	server->teamScores.clear();
	server->levelDone = false;
	server->levelRunTimer = 0;
	serverCvars->restoreVars();

	for(Server::ClientInfoPool::iterator ii=server->clientInfo.begin(); ii!=server->clientInfo.end(); ii++)
		ii->second.score = 0;

	//hide map markers upon starting
	server->areSpawnsMarked	   = false;
	server->areWaypointsMarked = false;
	
	serverScripting.clearTaggedVariables(tag_map_temporary);
	serverScripting.clearTriggers();

	//tell clients to clear their edges (since not handled like normal entities)
	SendMessage clearEdgesMsg(message_clear_edges);
	clearEdgesMsg.sendToEveryone();
}

void Server::markAsAlly(Entity *ent)
{
    SendMessage allyMarkerMsg(message_mark_ally);
        allyMarkerMsg.putInt(ent->getEntId());
    allyMarkerMsg.sendToEveryone();
}

void Server::highlightEntity(Entity *ent)
{
	SendMessage highlightMsg(message_change_color);
		highlightMsg.putInt(ent->getEntId());
		highlightMsg.putInt(255);
		highlightMsg.putInt(0);
		highlightMsg.putInt(0);
	highlightMsg.sendToEveryone();
}

void Server::unhighlightEntity(Entity *ent)
{
	SendMessage unhighlightMsg(message_change_color);
		unhighlightMsg.putInt(ent->getEntId());
		unhighlightMsg.putInt(255);
		unhighlightMsg.putInt(255);
		unhighlightMsg.putInt(255);
	unhighlightMsg.sendToEveryone();
}

void Server::unvisitAllWaypoints()
{
	WaypointPool::iterator ii;
	for (ii = waypoints.begin(); ii != waypoints.end(); ++ii)
	{
		WaypointMarker *currentWaypoint = *ii;
		currentWaypoint->setVisitStatus(status_unvisited);
		currentWaypoint->setParent(0);
		currentWaypoint->setWeight(FLT_MAX);
	}
}

bool Server::isShipTypeDeclared(const std::string &shipTypeName)
{
	return server->shipTypes.find(shipTypeName) != server->shipTypes.end();
}

SERVER_CONSOLE_COMMAND(map)
{
	if(argc != 1)
	{
		printfToClient(who, "Usage: map [level name]");
		return;
	}
	
	clearLevel();
	
	std::string filename = retprintf("maps/%s.map", argv[0]);
	serverScripting.setTag(tag_map_temporary);
	loadingMap = true;
	serverScripting.exec(filename.c_str());
	serverScripting.setTag(tag_normal);
	loadingMap = false;
}

SERVER_CONSOLE_COMMAND(clear_map)
{
	clearLevel();
}

SERVER_CONSOLE_COMMAND(spawn_players)
{
	server->spawnPlayers();
}


void refreshRects(bool show) { server->refreshRegionDisplay(); }
ServerConsoleVar<bool> showRects("show_rects", false, refreshRects);

void Server::refreshRegionDisplay()
{
	SendMessage msg(message_set_rects);
		if(showRects) {
			msg.putInt(server->regions.size());
			for(Server::RegionPool::iterator ii=server->regions.begin(); ii!=server->regions.end(); ii++) {
				msg.putString(ii->name.c_str());
				msg.putFloat(ii->min_x);
				msg.putFloat(ii->max_x);
				msg.putFloat(ii->min_y);
				msg.putFloat(ii->max_y);
			}
		} else {
			msg.putInt(0);
		}
	msg.sendToEveryone();
}

Server::Region Server::findRegion(std::string name)
{
	for(Server::RegionPool::iterator ii=server->regions.begin(); ii!=server->regions.end(); ii++)
	{
		if(ii->name == name)
			return *ii;
	}
	Region ret = {name, leftBorder, rightBorder, topBorder, bottomBorder};
	return ret;
}

SERVER_CONSOLE_COMMAND(timer)
{
	Server::Timer timer;
	
	if(argc < 2) {
		printfToClient(who, "Usage: timer [name] [delay]  (in seconds)");
		return;
	}
	float delay = atof(argv[1]);
	
	timer.fireTime = getTime() + delay;
	timer.name = argv[0];
	server->timers.push_back(timer);
}

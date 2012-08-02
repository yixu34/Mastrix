#include "mastrix.hpp"

static void reassignTeams(bool ignored);
ServerConsoleVar<bool> isDeathmatch("deathmatch", false, reassignTeams);
ServerConsoleVar<bool> isRedVsBlue("red_vs_blue", false, reassignTeams);
ServerConsoleVar<int> fragLimit("fraglimit", 30);
ServerConsoleVar<int> teamScoreLimit("team_fraglimit", 100);
ServerConsoleVar<int> timeLimit("timelimit", 300);

static std::string getScoreText(void);

static void reassignTeams(bool ignored)
{
	server->reassignTeams();
}

void Server::reassignTeams(void)
{
	// First, set everyone to team_free (so that when assigning people to red+
	// blue, they won't be counted). Then assign everyone normally.
	
	for(ClientInfoPool::iterator ii=clientInfo.begin(); ii!=clientInfo.end(); ii++)
		ii->second.team = team_free;
	
	// Assign everyone to a team
	for(ClientInfoPool::iterator ii=clientInfo.begin(); ii!=clientInfo.end(); ii++)
	{
		ii->second.team = newPlayerTeam(ii->first);
		
		// Kill live players to they respawn on the right team
		HumanPlayer *av = getAvatar(ii->first);
		if(av) av->dieSilently();
	}
}


team_type Server::newPlayerTeam(int clientID)
{
	if(isRedVsBlue)
	{
		team_type team;
		int reds = countPlayersOnTeam(team_red);
		int blues = countPlayersOnTeam(team_blue);
		
		// Assign to the team with less players, or if they're equal, then a
		// 50/50 random chance
		if     (reds>blues)   team = team_blue;
		else if(blues>reds)   team = team_red;
		else if(randInt(0,1)) team = team_blue;
		else                  team = team_red;
		
		// Tell the player which team he's on
		if(team==team_red) messageToClient(clientID, "You're on the red team.");
		else               messageToClient(clientID, "You're on the blue team.");
		
		return team;
	}
	else if(isDeathmatch)
		return team_free;
	else
		return team_human;
}



int Server::countPlayersOnTeam(team_type team)
{
	int ret = 0;
	for(ClientInfoPool::iterator ii=clientInfo.begin(); ii!=clientInfo.end(); ii++)
		if( ii->second.team == team )
			ret++;
	return ret;
}

const int numColors = 6;
int Server::newPlayerColor(void)
{
	int bestColorNum = -1;
	int bestColor = 0;
	for(int ii=0; ii<numColors; ii++)
	{
		int num = countPlayersWithColor(ii);
		if(bestColorNum==-1 || num<bestColorNum) {
			bestColorNum = num;
			bestColor = ii;
		}
	}
	return bestColor;
}
int Server::countPlayersWithColor(int color)
{
	int ret = 0;
	for(ClientInfoPool::iterator ii=clientInfo.begin(); ii!=clientInfo.end(); ii++)
		if( ii->second.color == color )
			ret++;
	return ret;
}


std::string Server::getGameResult(void)
{
	if(isRedVsBlue)
	{
		if(teamScores[team_red] > teamScores[team_blue])
			return "Red wins!";
		else if(teamScores[team_blue] > teamScores[team_red])
			return "Blue wins!";
		else
			return "Tie game!";
	}
	else
	{
		int maxScore = clientInfo.begin()->second.score;
		std::string bestPlayer = clientInfo.begin()->second.name;
		for(ClientInfoPool::iterator ii=clientInfo.begin(); ii!=clientInfo.end(); ii++)
		{
			if(ii->second.score >= maxScore) {
				maxScore = ii->second.score;
				bestPlayer = ii->second.name;
			}
		}
		return bestPlayer+" wins!";
	}
}


void Server::checkLevelEnd(void)
{
	bool done = false;
	
	if(levelRunTimer >= timeLimit)
		done = true;
	
	if(isRedVsBlue)
	{
		if(teamScores[team_red] >= teamScoreLimit ||
		   teamScores[team_blue] >= teamScoreLimit)
			done = true;
	}
	else if(isDeathmatch)
	{
		int maxScore = 0;
		for(ClientInfoPool::iterator ii=clientInfo.begin(); ii!=clientInfo.end(); ii++)
		{
			if(ii->second.score > maxScore) {
				maxScore = ii->second.score;
			}
		}
		if(maxScore >= fragLimit)
			done = true;;
	}
	
	if(done && !levelDone)
	{
		levelDone = true;
		levelEndTimer = 5;
		
		for(Entpool::iterator ii=entities.begin(); ii!=entities.end(); ii++)
		{
			ii->second->getPosition().setX_vel(0);
			ii->second->getPosition().setY_vel(0);
			ii->second->timepass();
		}
		
		std::string result=getGameResult();
		std::string text = getScoreText();
		SendMessage scoreText(message_scoreboard);
			scoreText.putString((result+"\n\n\n"+text.c_str()).c_str());
		scoreText.sendToEveryone();
	}
}

void Server::nextLevel(void)
{
	levelDone = false;
	SendMessage scoreText(message_scoreboard);
		scoreText.putString("");
	scoreText.sendToEveryone();
	serverScripting.command( retprintf("map %s", cycle.getNext().c_str()).c_str() );
}


void Server::addDeath(int clientID, int killerID, int victimID)
{
	double timeOfDeath = getTime();
	RespawnData data(clientID, timeOfDeath);
	
	respawns.push_front(data);
	
	std::string name = clientInfo[clientID].name;
	if(killerID < 0 || killerID == victimID)
	{
		//penalize suicides
		messageToClient(-1, "%s died.", name.c_str());
		clientInfo[clientID].score--;
		if(isDeathmatch && clientInfo[clientID].team != team_free)
			teamScores[clientInfo[clientID].team]--;
	}
	else {
		if(clientInfo.find(killerID) != clientInfo.end()) {
			clientInfo[killerID].score ++;
			messageToClient(-1, "%s killed %s.", clientInfo[killerID].name.c_str(), name.c_str());
			if(isDeathmatch && clientInfo[killerID].team != team_free)
				teamScores[clientInfo[killerID].team]++;
		}
	}
}

void Server::silentDeath(int clientID)
{
	double timeOfDeath = getTime();
	RespawnData data(clientID, timeOfDeath);
	respawns.push_front(data);
}


SERVER_CONSOLE_COMMAND(change_team)
{
	if(!isRedVsBlue)
		return;
	if(server->clientInfo.find(who) == server->clientInfo.end())
		return;
	team_type oldTeam = server->clientInfo[who].team;
	team_type newTeam;
	if(oldTeam == team_red) newTeam = team_blue;
	else                    newTeam = team_red;
	
	server->clientInfo[who].team = newTeam;
	Player *av = server->getAvatar(who);
	if(av) av->die(-1);
	
	messageToClient(-1, "%s has switched to the %s team.",
		server->clientInfo[who].name.c_str(),
		(newTeam==team_red)?"red":"blue");
}

struct scoreInfo {
	team_type team;
	std::string line;
};
static std::string getScoreText(void)
{
	std::vector<scoreInfo> lines;
	std::string scoreText;
	
	for(Server::ClientInfoPool::iterator ii = server->clientInfo.begin(); ii != server->clientInfo.end(); ii++)
	{
		scoreInfo add = {ii->second.team, retprintf("%i  %s", ii->second.score, ii->second.name.c_str())};
		lines.push_back(add);
	}
	
	if(!isRedVsBlue)
	{
		for(int ii=0; ii<lines.size(); ii++)
			scoreText += retprintf("%s\n", lines[ii].line.c_str());
	}
	else
	{
		scoreText += retprintf("%i Red\n", server->teamScores[team_red]);
		for(int ii=0; ii<lines.size(); ii++)
			if(lines[ii].team == team_red)
				scoreText += retprintf("  %s\n", lines[ii].line.c_str());
		
		scoreText += retprintf("%i Blue\n", server->teamScores[team_blue]);
		for(int ii=0; ii<lines.size(); ii++)
			if(lines[ii].team == team_blue)
				scoreText += retprintf("  %s\n", lines[ii].line.c_str());
	}
	return scoreText;
}

/// Find all entities with label #label and put their entids in #results
void Server::findEntsWithLabel(std::string label, std::vector<int> &results)
{
	for(Entpool::iterator ii=entities.begin(); ii!=entities.end(); ii++)
	{
		if(ii->second->getLabel() == label && !ii->second->shouldDelete())
			results.push_back(ii->second->getEntId());
	}
}

Position Server::findFreeSpotWithLabel(std::string label)
{
	std::vector<int> markers;
	findEntsWithLabel(label, markers);
	
	std::vector<Position> positions;
	for(int ii=0; ii<markers.size(); ii++)
	{
		Entity *marker = entities[ markers[ii] ];
		positions.push_back(marker->getPosition());
	}
	
	std::vector<Position> validPositions;
	
	for(int ii=0; ii<positions.size(); ii++)
	{
		
		bool isSpotOK = true;
		
		for (Entpool::iterator jj = entities.begin(); jj != entities.end(); ++jj)
		{
			if(jj->second->getEntId() == markers[ii])
				continue;
			
			Vector2D displacement = (jj->second->getPosition().positionVector() - positions[ii].positionVector());
			
			if (displacement.getMagnitude() < 20.0f) {
				isSpotOK = false;
				break;
			}
		}
		if(isSpotOK)
			validPositions.push_back(positions[ii]);
	}
	
	if(validPositions.size() > 0) {
		return validPositions[ randInt(0, validPositions.size()-1) ];
	} else if(positions.size() > 0) {
		return positions[ randInt(0, positions.size()-1) ];
	} else {
		return Position(0,0);
	}
}

void Server::giveFlagTo(team_type team)
{
	Position pos;
	if(team==team_red)
		pos = findFreeSpotWithLabel("red_flag");
	else
		pos = findFreeSpotWithLabel("blue_flag");
	
	placeFlagAt(pos.getX(), pos.getY(), team, true);
}

SERVER_CONSOLE_COMMAND(give_flag_to)
{
	if(argc != 1) {
		printfToClient(who, "Usage: give_flag_to (red|blue)");
	}
	team_type team;
	if     (!strcasecmp(argv[0], "red" )) team = team_red;
	else if(!strcasecmp(argv[0], "blue")) team = team_blue;
	else {
		printfToClient(who, "%s is not a valid team", argv[0]);
		return;
	}
	server->teamScores[team]++;
	server->giveFlagTo(team);
}


SERVER_INVERTIBLE_CONSOLE_COMMAND(show_scores)
{
	std::string text = getScoreText();
	
	SendMessage scoreText(message_scoreboard);
		scoreText.putString(text.c_str());
	scoreText.sendToClient(who);
}


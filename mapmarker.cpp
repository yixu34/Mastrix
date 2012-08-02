#include "mastrix.hpp"

SpawnpointMarker::SpawnpointMarker(float x, float y, float r, team_type team)
{
	drawScale = 0.6;
	sprite	  = "images/spawnpoint.png";
	position  = Position(x, y, r);
	visible   = false;
	this->team = team;
	server->spawns.push_back(this);
	sendAdd();
}

SpawnpointMarker::~SpawnpointMarker()
{
	for(Server::SpawnPool::iterator ii=server->spawns.begin(); ii!=server->spawns.end(); ii++)
	{
		if((*ii) == this) {
			server->spawns.erase(ii);
			break;
		}
	}
}

SERVER_CONSOLE_COMMAND(addspawn)
{
	if(argc < 2)
	{
		printfToClient(who, "Not enough arguments to addspawn (should be 2)");
		printfToClient(who, "Usage: addspawn x y [r] [team]");
		return;
	}
	float x=atof(argv[0]), y=atof(argv[1]);
	float r = (argc>=3) ? atof(argv[2]) : 0;
	team_type team = (argc>=4) ? (team_type)atoi(argv[3]) : team_free;
	SpawnpointMarker *marker = new SpawnpointMarker(x, y, r, team);
	if(server->areSpawnsMarked) {
		marker->show();
	}
}

SERVER_CONSOLE_COMMAND(mark_spawnpoints)
{
	Server::SpawnPool::iterator ii;
	for(ii = server->spawns.begin(); ii!=server->spawns.end(); ii++)
		(*ii)->show();
	
	server->areSpawnsMarked = true;
}

SERVER_CONSOLE_COMMAND(unmark_spawnpoints)
{
	Server::SpawnPool::iterator ii;
	for (ii = server->spawns.begin(); ii != server->spawns.end(); ++ii)
		(*ii)->hide();

	server->areSpawnsMarked = false;
}

//
// Waypoint Marker methods
//

WaypointMarker::WaypointMarker(float x, float y, float r)
{
	drawScale   = 0.6;
	sprite		= "images/waypoint.png";
	position	= Position(x, y, r);
	visible		= false;
	visitStatus	= status_unvisited;
	weight		= FLT_MAX;
	parent		= 0;
	sendAdd();
}

//this will return a random neighboring waypoint
//if it is isolated, it will return itself
WaypointMarker *WaypointMarker::getRandomNeighbor()
{
	int maxSize = edgesOut.size();

	if (maxSize <= 0)	//handle empty edges gracefully...
		return this;

	int randomIndex = randInt(0, maxSize - 1);
	WaypointMarker *nearestWaypoint = edgesOut.at(randomIndex).target;

	return nearestWaypoint;
}

//symmetrically add an edge to a destination waypoint
void WaypointMarker::addEdgeTo(WaypointMarker *target)
{
	if (target == 0)
		return;

	float sharedDist = sqrtf(distSquaredBetween(
								position, 
								target->getPosition()));

	Edge targetEdge;
	targetEdge.target = target;
	targetEdge.length = sharedDist;

	edgesOut.push_back(targetEdge);

	//now take care of symmetry; have the target connect to this waypoint
	Edge thisEdge;
	thisEdge.target = this;
	thisEdge.length = sharedDist;

	target->edgesOut.push_back(thisEdge);

	//now inform the client visually
	SendMessage addEdgeMsg(message_add_edge);
		addEdgeMsg.putInt(getEntId());
		addEdgeMsg.putInt(target->getEntId());
	addEdgeMsg.sendToEveryone();
}

//removes edges between two waypoints, whether the edge was originally
//undirected or not
void WaypointMarker::removeEdgeTo(WaypointMarker *target)
{
	if (target == 0)
		return;

	//two waypoints are unconnected, ignore
	bool hasEdgeToTarget     = hasEdgeTo(target);
	bool targetHasEdgeToThis = target->hasEdgeTo(this);
	
	if (!(hasEdgeToTarget || targetHasEdgeToThis))
		return;

	//remove edge to target
	if (hasEdgeToTarget)
	{
		EdgePool::iterator sourceIter;
		for (sourceIter = edgesOut.begin(); sourceIter != edgesOut.end(); ++sourceIter)
		{
			if (sourceIter->target == target)
			{
				sourceIter = edgesOut.erase(sourceIter);
				--sourceIter;
				break;
			}
		}				
	}//if

	//remove target's edge to this
	if (targetHasEdgeToThis)
	{
		EdgePool &destEdges = target->getEdges();

		EdgePool::iterator destIter;
		for (destIter = destEdges.begin(); destIter != destEdges.end(); ++destIter)
		{
			if (destIter->target == this)
			{
				destIter = destEdges.erase(destIter);
				--destIter;
				break;
			}
		}		
	}//if

	//now inform the client visually
	//...client edge ID's are just one line, so we don't have to worry about
	//...two colinear lines sharing an edge ID
	SendMessage dropEdgeMsg(message_drop_edge);
		dropEdgeMsg.putInt(getEntId());
		dropEdgeMsg.putInt(target->getEntId());
	dropEdgeMsg.sendToEveryone();
}

//when a waypoint is updated (moved), update the edge weights
void WaypointMarker::calculateEdgeWeights()
{
	EdgePool::iterator thisIter;
	for (thisIter = edgesOut.begin(); thisIter != edgesOut.end(); ++thisIter)
	{
		//set the length for the edge going out...
		Edge &currentEdgeOut = (*thisIter);
		WaypointMarker *currentTarget = currentEdgeOut.target;
		const float sharedLength = sqrtf(distSquaredBetween(
												position, 
												currentTarget->getPosition()));

		currentEdgeOut.length = sharedLength;

		//...and set the length for the edge coming into this waypoint
		EdgePool::iterator targetIter;
		EdgePool &targetEdges = currentTarget->getEdges();
		for (targetIter = targetEdges.begin(); targetIter != targetEdges.end(); ++targetIter)
		{
			if (targetIter->target == this)
			{
				targetIter->length = sharedLength;
				break;
			}
		}
	}//for
}

//return true if this is connected to the target, false otherwise
//!!!THIS IS NOT SYMMETRIC LIKE THE OTHER METHODS!!!
bool WaypointMarker::hasEdgeTo(WaypointMarker *target) const
{
	EdgePool::const_iterator sourceIter;
	for (sourceIter = edgesOut.begin(); sourceIter != edgesOut.end(); ++sourceIter)
		if (sourceIter->target == target)
			return true;

	return false;
}

//break all connections with this node
void WaypointMarker::deleteMe()
{
	Server::WaypointPool::iterator ii;
	//Server::WaypointPool::iterator thisIter;
	for (ii = server->waypoints.begin(); ii != server->waypoints.end(); ++ii)
	{
		if ((*ii) != this && (*ii)->hasEdgeTo(this))
		{
			(*ii)->removeEdgeTo(this);			
		}
	}

	//after being disconnected, remove waypoint from server waypoint pool
	for (ii = server->waypoints.begin(); ii != server->waypoints.end(); ++ii)
	{
		if (*ii == this)
		{
			ii = server->waypoints.erase(ii);
			--ii;
			break;
		}
	}

	edgesOut.clear();

	Entity::deleteMe();	
}

SERVER_CONSOLE_COMMAND(mark_waypoints)
{
	Server::WaypointPool::iterator ii;
	for(ii = server->waypoints.begin(); ii!=server->waypoints.end(); ii++)
		(*ii)->show();

	SendMessage showEdgesMsg(message_show_all_edges);
	showEdgesMsg.sendToEveryone();

	server->areWaypointsMarked = true;
}

SERVER_CONSOLE_COMMAND(unmark_waypoints)
{
	Server::WaypointPool::iterator ii;
	for(ii = server->waypoints.begin(); ii!=server->waypoints.end(); ii++)
		(*ii)->hide();

	SendMessage hideEdgesMsg(message_hide_all_edges);
	hideEdgesMsg.sendToEveryone();

	server->areWaypointsMarked = false;
}


SERVER_CONSOLE_COMMAND(addwaypoint)
{
	if (argc < 2)
	{
		printfToClient(who, "Not enough arguments to addwaypoint (should be 2)");
		printfToClient(who, "Usage: addwaypoint x y [r]");
		return;
	}

	float x = atof(argv[0]);
	float y = atof(argv[1]);
	float r = (argc>=3) ? atof(argv[2]) : 0;

	WaypointMarker *marker = new WaypointMarker(x, y, r);
	server->waypoints.push_back(marker);

	if (server->areWaypointsMarked)
		marker->show();
}

//handles adding edges between A and B, with error checking
//I'm going to add an optional 3rd and 4th parameter that represents
//the source position (easier for saving)
SERVER_CONSOLE_COMMAND(addedge)
{
	if (argc < 2)
	{
		printfToClient(who, "Not enough arguments to addedge (should be 2)");
		printfToClient(who, "Usage: addedge destX destY [sourceX] [sourceY]");
		return;
	}

	//the 4 argument case; generally used for saving the level file
	WaypointMarker *source = 0;	
	if (argc >= 4)
	{
		float sourceX = atoi(argv[2]);
		float sourceY = atoi(argv[3]);
		Position sourcePos = Position(sourceX, sourceY);

		source = server->getWaypointNearestTo(sourcePos);
	}
	else	//2 argument case
	{
		if (server->entities.find(server->selectedEntity) == server->entities.end() || 
			!server->entities[server->selectedEntity]->isWaypointMarker())
		{
			printfToClient(who, "Couldn't find the source waypoint!");
			return;
		}
		source = (WaypointMarker *)(server->entities[server->selectedEntity]);
	}

	if (source == 0)
	{
		printfToClient(who, "Couldnt' find a suitable source waypoint");
		return;
	}

	float destX = atof(argv[0]);
	float destY = atof(argv[1]);

	Position sourcePos = source->getPosition();
	Position destPos   = Position(destX, destY);

	WaypointMarker *dest   = server->getWaypointNearestTo(destPos);

	//make sure that we could find suitable waypoints, 
	//and don't add an edge to yourself!
	if (source == 0 || dest == 0 || source == dest)
	{
		printfToClient(who, "Couldn't find suitable waypoints");
		return;
	}

	if (source->hasEdgeTo(dest))
	{
		printfToClient(who, "Source already has an edge to that destination!");
		return;
	}

	source->addEdgeTo(dest);
}

//handles removing edges between A and B, with error checking
SERVER_CONSOLE_COMMAND(removeedge)
{
	if (argc < 2)
	{
		printfToClient(who, "Not enough arguments to removeedge (should be 2)");
		printfToClient(who, "Usage: removeedge destX destY");
		return;
	}

	//make sure you have a source waypoint selected from which to remove an edge!
	if (server->selectedEntity < 0 ||
		server->entities.find(server->selectedEntity) == server->entities.end())
	{
		printfToClient(who, "No source entity selected!");
		return;
	}

	float destX = atof(argv[0]);
	float destY = atof(argv[1]);

	Entity *selectedEnt  = server->entities[server->selectedEntity];

	Position selectedPos = selectedEnt->getPosition();
	Position destPos     = Position(destX, destY);

	WaypointMarker *source = server->getWaypointNearestTo(selectedPos);
	WaypointMarker *dest   = server->getWaypointNearestTo(destPos);

	//make sure that waypoints exist and aren't the same
	if (source == 0 || dest == 0 || source == dest)
	{
		printfToClient(who, "Couldn't find suitable waypoints");
		return;
	}

	//debugging info; will still try to remove edges but will print a warning
	if (!source->hasEdgeTo(dest))
		printfToClient(who, "Source has no edge to dest");
	if (!dest->hasEdgeTo(source))
		printfToClient(who, "Dest has no edge to source");

	source->removeEdgeTo(dest);
}
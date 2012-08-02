#include "mastrix.hpp"
#include <algorithm>

static void writeLevel(FILE *fout);

ClientConsoleVar<bool> gridEnabled("grid_enabled", false);
ClientConsoleVar<float> gridSize("grid_size", 64);

// Get the mouse X-position and translate it into game coordinates.
float getEditMouseX()
{
	Client *cl = (Client*)currentNode;
	return (getMouseX() - cl->getViewport()->center_x)/zoom + cl->getCamera().getX();
}
// Get the mouse Y-position and translate it into game coordinates.
float getEditMouseY()
{
	Client *cl = (Client*)currentNode;
	return (getMouseY() - cl->getViewport()->center_y)/zoom + cl->getCamera().getY();
}

CLIENT_CONSOLE_COMMAND(select_click)
{
	((Client*)currentNode) -> select();
}

CLIENT_CONSOLE_COMMAND(cursor)
{
	if(argc<1) {
		console.printf("Usage: cursor [command] [arguments]");
		return;
	}
	std::string args;
	for(int ii=1; ii<argc; ii++)
		args += std::string(" {") + argv[ii] + "}";
	
	float x = getEditMouseX(),
	      y = getEditMouseY();
	if(gridEnabled) {
		x = gridSize * floor((x+gridSize/2)/gridSize);
		y = gridSize * floor((y+gridSize/2)/gridSize);
	}
	console.command( retprintf(
						"%s %f %f %s", 
						argv[0], 
						x, y,
						args.c_str()) );
}

SERVER_CONSOLE_COMMAND(selected)
{
	if(argc<1) {
		console.printf("Usage: selected [command] [arguments]");
		return;
	}
	std::string args;
	for(int ii=1; ii<argc; ii++)
		args += std::string(" {") + argv[ii] + "}";
	console.command( retprintf(
						"%s %i %s",
						argv[0],
						server->selectedEntity, 
						args.c_str()) );
}


void Client::select(void)
{
	for(CLEntityPool::iterator ii=entpool.begin(); ii!=entpool.end(); ii++)
	{
		CLEntity *ent = ii->second;
		Position &pos(ii->second->getPosition());
		float radius = ii->second->sprite->getWidth() / 2 * ii->second->scale;
		
		float click_x = getEditMouseX(), click_y = getEditMouseY();
		
		float dx = click_x - pos.getX(),
		      dy = click_y - pos.getY();
		float distSq = dx*dx + dy*dy;
		if(distSq < radius*radius) {
			console.command( retprintf("select_ent %i", ii->first) );
			break;
		}
	}
}

void Client::select(int entID)
{
	selectedEntId = entID;
}


SERVER_CONSOLE_COMMAND(addregion)
{
	if(argc != 5) {
		console.printf("Usage: addregion name min_x min_y max_x max_y");
		return;
	}
	float min_x = atof(argv[1]),
	      min_y = atof(argv[2]),
	      max_x = atof(argv[3]),
	      max_y = atof(argv[4]);
	if(min_x > max_x) std::swap(min_x, max_x);
	if(min_y > max_y) std::swap(min_y, max_y);
	Server::Region addition = {argv[0], min_x, max_x, min_y, max_y};
	server->regions.push_back(addition);
	server->refreshRegionDisplay();
}

/* Remove all ents inside the region */
SERVER_CONSOLE_COMMAND(clear_region)
{
	if(argc != 4) {
		console.printf("Usage: clear_region min_x min_y max_x max_y");
		return;
	}
	float min_x = atof(argv[0]),
	      min_y = atof(argv[1]),
	      max_x = atof(argv[2]),
	      max_y = atof(argv[3]);
	if(min_x > max_x) std::swap(min_x, max_x);
	if(min_y > max_y) std::swap(min_y, max_y);
	
	for(Server::Entpool::iterator ii=server->entities.begin(); ii!=server->entities.end(); ii++)
	{
		Entity *ent = ii->second;
		float x = ent->getPosition().getX(),
		      y = ent->getPosition().getY();
		if(x>min_x && x<max_x && y>min_y && y<max_y)
			ent->deleteMe();
	}
}

SERVER_CONSOLE_COMMAND(select_ent)
{
	if(argc != 1) {
		console.printf("Usage: select_ent [entID]");
		return;
	}
	int entID = atoi(argv[0]);

	
	if (!server->entities[entID]->isVisible())
		return;

	
	server->selectedEntity = entID;
	
	SendMessage msg(message_selection_box);
		msg.putInt(entID);
	msg.sendToEveryone();
}

SERVER_CONSOLE_COMMAND(move_ent_to)
{
	if(argc < 2) {
		console.printf("Usage: move_ent_to x y [entid]");
		return;
	}
	
	int entid = server->selectedEntity;
	if(argc == 3)
		entid = atoi(argv[2]);
	if(entid < 0) return;
	
	Entity *ent = server->entities[entid];
	Position newPos = Position(atof(argv[0]), atof(argv[1]), ent->getPosition().getR());
	ent->setPosition( newPos );

	//if the entity is a waypoint, recalculate all of its edge weights
	if (ent->isWaypointMarker())
		((WaypointMarker *)ent)->calculateEdgeWeights();
}

SERVER_CONSOLE_COMMAND(set_rvel)
{
	if(argc < 1) {
		console.printf("Usage: set_rvel rvel [entid]");
		return;
	}
	
	int entid = server->selectedEntity;
	if(argc == 2)
		entid = atoi(argv[1]);
	if(entid < 0) return;
	
	Entity *ent = server->entities[entid];
	Position pos = ent->getPosition();
	pos.setR_vel( atof(argv[0]) );
	ent->setPosition(pos);
}

SERVER_CONSOLE_COMMAND(rescale_ent)
{
	if(argc < 1) {
		console.printf("Usage: rescale size [entid]");
		return;
	}
	
	int entid = server->selectedEntity;
	if(argc == 2)
		entid = atoi(argv[1]);
	if(entid < 0) return;
	
	Entity *ent = server->entities[entid];
	ent->rescale( atof(argv[0]) );
}

SERVER_CONSOLE_COMMAND(rotate_ent_towards)
{
	if(argc < 2) {
		console.printf("Usage: rotate_ent_towards x y [entid]");
		return;
	}
	
	int entid = server->selectedEntity;
	if(argc == 3)
		entid = atoi(argv[2]);
	if(entid < 0) return;
	
	Entity *ent = server->entities[entid];
	float x=atof(argv[0]), y=atof(argv[1]);
	float angle = atan2( ent->getPosition().getY()-y, x-ent->getPosition().getX() );
	
	Position newPos = ent->getPosition();
	newPos.setR(angle);
	ent->setPosition(newPos);
}

SERVER_CONSOLE_COMMAND(delete_ent)
{
	int entid = server->selectedEntity;
	if(argc == 1)
		entid = atoi(argv[0]);
	if(entid < 0) return;
	
	Entity *ent = server->entities[server->selectedEntity];
	ent->die(-1);
}

SERVER_CONSOLE_COMMAND(entflag)
{
	if(argc < 1) {
		printfToClient(who, "Usage: entflag flag [entid]");
		return;
	}
	int entid = server->selectedEntity;
	if(argc == 2)
		entid = atoi(argv[1]);
	if(entid < 0) return;
	
	Entity *ent = server->entities[entid];
	EnvironmentEntity *cast = (EnvironmentEntity*)ent;
	if(!cast) return;
	
	int flag = EnvironmentEntity::flagFromString(argv[0]);
	if(!flag)
		printfToClient(who, "%s is not a valid flag.", argv[0]);
	else
		cast->addFlag(flag);
}


SERVER_CONSOLE_COMMAND(kick_ent_towards)
{
	if(argc < 3) {
		console.printf("Usage: kick_ent_towards x y strength [entid]");
		return;
	}
	int entid = server->selectedEntity;
	if(argc == 4)
		entid = atoi(argv[3]);
	if(entid < 0) return;
	
	float x=atof(argv[0]), y=atof(argv[1]);
	float strength = atof(argv[2]);
	Entity *ent = server->entities[entid];

	//ignore this command if it's a waypoint (too damn expensive to constantly
	//recalculate the edge weights for every step it moves)	
	if (ent->isWaypointMarker())
		return;
	
	Position newPos = ent->getPosition();
	float xvel = x-newPos.getX(),
	      yvel = y-newPos.getY();
	float norm = sqrt(xvel*xvel + yvel*yvel);
	xvel *= strength/norm;
	yvel *= strength/norm;
	
	newPos.setX_vel(xvel);
	newPos.setY_vel(yvel);
	ent->setPosition(newPos);
}


SERVER_CONSOLE_COMMAND(save_level)
{
	if(argc != 1) {
		printfToClient(who, "Usage: save_level name");
		return;
	}
	// Check for bad chars and generate filename
	for(const char *pos=argv[0]; *pos; pos++) {
		if(*pos == '.' || *pos == '/' || *pos == '\\') {
			printfToClient(who, "Cannot use special characters in filename. (The extension and path are handled automatically.");
		}
	}
	std::string filename = retprintf("maps/%s.map", argv[0]);
	
	// First, read the file that will be overwritten and extract all non-auto-generated parts.
	char buf[512];
	std::string prefix(""), suffix("");
	std::string *read_target = &prefix;
	int read_state = 0; // 0:prefix, 1:generated, 2:suffix
	FILE *fin = fopen(filename.c_str(), "r");
	if(fin)
	{
		while(!feof(fin))
		{
			if(!fgets(buf, 512, fin))
				break;
			if(!strcmp(buf, "# BEGIN GENERATED CODE\n") ) {
				read_state = 1;
				read_target = NULL;
			}
			else if(!strcmp(buf, "# END GENERATED CODE\n") ) {
				read_state = 2;
				read_target = &suffix;
			}
			else if(read_target)
				(*read_target) += buf;
		}
		fclose(fin);
	}
	
	// Open output file
	FILE *fout = fopen(filename.c_str(), "w");
	if(!fout) {
		printfToClient(who, "Couldn't open output file.");
		return;
	}
	fputs(prefix.c_str(), fout);
	fputs("# BEGIN GENERATED CODE\n", fout);
	writeLevel(fout);
	fputs("# END GENERATED CODE\n", fout);
	fputs(suffix.c_str(), fout);
	
	fclose(fout);
}

static void writeLevel(FILE *fout)
{
	// Write addent ... lines
	for(Server::Entpool::iterator ii=server->entities.begin(); ii!=server->entities.end(); ii++)
		fprintf(fout, "%s", ii->second->creationString().c_str());
	
	// Write addspawn ... lines
	for(Server::SpawnPool::iterator ii=server->spawns.begin(); ii!=server->spawns.end(); ii++)
	{
		fprintf(
			fout, 
			"addspawn %f %f %f %i\n", 
			(*ii)->getPosition().getX(), 
			(*ii)->getPosition().getY(), 
			(*ii)->getPosition().getR(),
			(*ii)->team);
	}
	
	// Write addregion ... lines
	for(Server::RegionPool::iterator ii=server->regions.begin(); ii!=server->regions.end(); ii++)
	{
		fprintf(fout,
			"addregion %s %f %f %f %f\n",
			ii->name.c_str(), (float)ii->min_x, (float)ii->min_y, (float)ii->max_x, (float)ii->max_y);
	}

	// Write addwaypoint ... lines
	for (Server::WaypointPool::iterator ii = server->waypoints.begin(); ii != server->waypoints.end(); ++ii)
	{
		WaypointMarker *currentWaypoint = (*ii);
		fprintf(
			fout, 
			"addwaypoint %f %f\n", 
			currentWaypoint->getPosition().getX(), 
			currentWaypoint->getPosition().getY());
	}

	// Write the waypoint edges (taking 2nd pass because not all waypoints might 
	// have already been read)
    server->unvisitAllWaypoints();
	for (Server::WaypointPool::iterator ii = server->waypoints.begin(); ii != server->waypoints.end(); ++ii)
	{
		WaypointMarker *currentWaypoint = (*ii);

        if (currentWaypoint->getVisitStatus() == status_visited)
            continue;

        currentWaypoint->setVisitStatus(status_visited);

		WaypointMarker::EdgePool::iterator edgeIter;
		WaypointMarker::EdgePool &currentEdges = currentWaypoint->getEdges();
		for (edgeIter = currentEdges.begin(); edgeIter != currentEdges.end(); ++edgeIter)
		{
            if (edgeIter->target->getVisitStatus() == status_visited)
                continue;

			float destX   = edgeIter->target->getPosition().getX();
			float destY   = edgeIter->target->getPosition().getY();
			float sourceX = (*ii)->getPosition().getX();
			float sourceY = (*ii)->getPosition().getY();

			fprintf(
				fout, 
				"addedge %f %f %f %f\n", 
				destX, 
				destY, 
				sourceX, 
				sourceY);
		}
	}

	fprintf(fout,
		"border_left %f\n"
		"border_right %f\n"
		"border_top %f\n"
		"border_bottom %f\n",
		(float)leftBorder, (float)rightBorder, (float)topBorder, (float)bottomBorder);
	fprintf(fout, "spawn_players\n");	
}

bool gameFrozen = false;double FrozenTime = 0;double TimeofFreeze = 0;double TimeofUnfreeze = 0;
SERVER_CONSOLE_COMMAND(freeze){	if(gameFrozen) return;	gameFrozen = true;	TimeofFreeze = (double)SDL_GetTicks() / 1000;}
SERVER_CONSOLE_COMMAND(unfreeze){	if(!gameFrozen) return;	gameFrozen = false;	TimeofUnfreeze = (double)SDL_GetTicks() / 1000;	FrozenTime +=TimeofUnfreeze-TimeofFreeze;}SERVER_CONSOLE_COMMAND(pause){	if(!gameFrozen) {		TimeofFreeze = (double)SDL_GetTicks() / 1000;	} else {		TimeofUnfreeze = (double)SDL_GetTicks() / 1000;		FrozenTime += TimeofUnfreeze-TimeofFreeze;	}	gameFrozen = !gameFrozen;}
SERVER_CONSOLE_COMMAND(display_time){	printfToClient(who, "Time is (%f), dt is (%f)",getTime(),getDt());}

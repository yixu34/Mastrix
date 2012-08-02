#include "mastrix.hpp"

ServerConsoleVar<float> soundRange("sound_range", 500.0f);
float zoom = 1.0;
ClientConsoleVar<float> targetZoom("zoom", 1.0);
static void changeMusicStatus(bool playing);
ClientConsoleVar<bool> playMusic("play_music", true, changeMusicStatus);
ClientConsoleVar<bool> showBorders("show_borders", false);

//MusicX *music = NULL;

CLIENT_CONSOLE_COMMAND(instant_zoom)
{
	zoom = targetZoom;
}

static void changeName(std::string newname);
ClientConsoleVar<std::string> name("name", "unnamed", changeName);

Client::Client(int index)
	 : GameNode(clientCvars, clientCommandPool)
{
	this->index  = index;
	showScoreboard = false;
	shipEntId = 0;

	areEdgesVisible = false;
	fadeSpeed = 0;
//	targetFade = fadeOut = 1.0;
	targetFade = fadeOut = 0.0;
	connectionComplete = false;
}

Client::~Client() {
}

// Notify the server of my existance.
void Client::connect(void)
{
	SendMessage connect(message_connect);
	connect.sendToServer();
	
	SendMessage myname(message_name);
		myname << name;
	myname.sendToServer();
	
	connected = true;
}

ClientConsoleVar<bool> trackMouse("track_mouse", false);

void Client::timepass(void)
{
	MessageGroup *messages = getMessages();
	RecvMessage  *message;
	
	if(zoom > targetZoom)
	{
		zoom *= pow(0.5f, getRealDt());
		if(zoom < targetZoom) zoom = targetZoom;
	}
	else if(zoom < targetZoom)
	{
		zoom *= pow(2.0f, getRealDt());
		if(zoom > targetZoom) zoom = targetZoom;
	}
	
	if(trackMouse) {
		const Position *myPos = getEntityPosition(shipEntId);
		if(myPos)
		{
			float ship_x = zoom*(myPos->getX() - getCamera().getX()) + viewport->center_x,
			      ship_y = zoom*(myPos->getY() - getCamera().getY()) + viewport->center_y;

			console.command(
				retprintf("mousepos %f %f",
					(getMouseX() - ship_x)/zoom,
					(getMouseY() - ship_y)/zoom)
				);
		}
	}
	
	if(!connectionComplete) {
		fadeSpeed = 3;
		targetFade = 0.0;
		connectionComplete = true;
	}
	
	while( (message=messages->next()) )
	{
		switch(message->getType())
		{
		case message_disconnect:		serverDisconnect(*message); break;
		case message_command:			runCommand(*message);       break;
		case message_add_entity:		addEntity(*message);        break;
		case message_update_entity:		updateEntity(*message);     break;
		case message_drop_entity:		dropEntity(*message);	    break;
		case message_camera:			centerCamera(*message);     break;
		case message_print:				print(*message);	        break;
		case message_sound_3D:			playSoundAt(*message);      break;
        case message_sound:             playSound(*message);        break;
		case message_engine:			setEngines(*message);       break;
		case message_explode:			explodeAt(*message);        break;
		case message_updatehealth:		updateUI(*message);         break;
		case message_change_color:		changeColor(*message);      break;
		case message_scoreboard:		updateScoreboard(*message); break;
		case message_selection_box:		select(message->getInt());  break;
		case message_ship_entid:		shipEntId = message->getInt(); break;
		case message_add_edge:			addEdge(*message);          break;
		case message_drop_edge:			dropEdge(*message);         break;
		case message_show_all_edges:	showAllEdges();             break;
		case message_hide_all_edges:	hideAllEdges();             break;
		case message_clear_edges:		clearEdges();               break;
		case message_set_rects:         setRectangles(*message);    break;
		case message_weapons:			userInterface.updateWeaponInfo(*message); break;
        case message_lightning:         drawLightning(*message);    break;
		case message_shield:			setDrawShield(*message);    break;
        case message_mark_ally:         markAsAlly(*message);       break;
        case message_update_engines:    userInterface.updateEngineDiamond(*message); break;
		case message_avatar:			setAvatar(message->getInt()); break;
		case message_update_effect:		updateDrawEffect(*message);	break;
		case message_sync_time:         syncTime(message->getFloat()); break;
		default: break;
		}
	}
	
	particlePool.timepass();
	
	delete messages;
}

ClientConsoleVar<bool> showPosition("show_position", false);
ClientConsoleVar<bool> showFPS("showfps", false);
ClientConsoleVar<bool> showNumParticles("show_particle_ct", false);
ClientConsoleVar<bool> boundCamera("bound_camera", true);
ClientConsoleVar<int> smoothStrength("smooth_strength", 5);
ClientConsoleVar<float> mouseDistance("mouse_distance", 0.6);

void Client::redraw(void)
{
	viewport->setClip();
	
	typedef std::deque<Position> PositionQueue;
	static std::deque<Position> prevMouse;
	
	float mouse_x = (getMouseX()-viewport->center_x)/zoom * mouseDistance;
	float mouse_y = (getMouseY()-viewport->center_y)/zoom * mouseDistance;
	prevMouse.push_front(Position(mouse_x, mouse_y));
	if(prevMouse.size() > smoothStrength)
		prevMouse.pop_back();
	float smooth_mouse_x = 0,
	      smooth_mouse_y = 0;
	for(PositionQueue::iterator ii = prevMouse.begin(); ii!=prevMouse.end(); ii++) {
		smooth_mouse_x += ii->getX();
		smooth_mouse_y += ii->getY();
	}
	smooth_mouse_x /= prevMouse.size();
	smooth_mouse_y /= prevMouse.size();
	float camera_x = smooth_mouse_x + cameraCenter.getX();
	float camera_y = smooth_mouse_y + cameraCenter.getY();
	camera = Position(camera_x, camera_y);
	
	if(boundCamera)
	{
		if(camera.getX() - (viewport->center_x-viewport->left)/zoom < camera_bound_left)
			camera.setX( camera_bound_left + (viewport->center_x-viewport->left)/zoom );
		if(camera.getX() + (viewport->right-viewport->center_x)/zoom > camera_bound_right)
			camera.setX( camera_bound_right - (viewport->right-viewport->center_x)/zoom );
		
		if(camera.getY() - (viewport->center_y-viewport->top)/zoom < camera_bound_top)
			camera.setY( camera_bound_top + (viewport->center_y-viewport->top)/zoom );
		if(camera.getY() + (viewport->bottom-viewport->center_y)/zoom > camera_bound_bottom)
			camera.setY( camera_bound_bottom - (viewport->bottom-viewport->center_y)/zoom );
	}
	
	stars.draw(getCamera(), viewport);
	particlePool.draw(&camera, viewport, true);
	
	for (CLEntityPool::iterator ii = entpool.begin(); ii != entpool.end(); ++ii) {
		drawEnt(ii->second);
		if ((ii->second)->shieldTime > 0 || (ii->second)->shieldTime < -300.0) {
			drawShield
				(ii->first, 
				(ii->second)->shieldRadius, 
				(ii->second)->shieldTime, 
				(ii->second)->shieldType);
		}
	}
	if(showPosition)
	{
		std::string text = retprintf("(%.1f,%.1f)", camera.getX(), camera.getY());
		//GameX.DrawText(viewport->left+5, viewport->top+5, (char*)text.c_str(), 230, 230, 255);
		graphics.drawText(text.c_str(), viewport->left + 100, viewport->top + 45);
	}
	
	if(showScoreboard)
	{
		int ii=0;
		char *tokScoreText = strdup(scoreText.c_str());
		char *pos = strtok(tokScoreText, "\n");
		do {
			if(pos)
				//GameX.DrawText(viewport->left+35, viewport->top+100 + 12*ii, pos, 230, 230, 255);
				graphics.drawText(
					pos, 
					viewport->left + 35, 
					(viewport->top + 100) + (12 * ii));
			ii++;
		} while(pos = strtok(NULL, "\n"));
		
		free(tokScoreText);
	}
	if(showMenu)
	{
		int ii=0;
		char *tokMenuText = strdup(menuText.c_str());
		char *pos = strtok(tokMenuText, "\n");
		do {
			if(pos)
				graphics.drawText(
					pos, 
					viewport->left + 40, 
					(viewport->top + 300) + (12 * ii));
			ii++;
		} while(pos = strtok(NULL, "\n"));
		
		free(tokMenuText);
	}
	
	if (showFPS)
	{
		std::string text = retprintf("%.1f fps", getFrameRate());
		graphics.drawText(text.c_str(), viewport->left + 100, viewport->top + 60);
	}

	if (showNumParticles)
	{
		std::string text = retprintf("%i particles", particlePool.size());
		graphics.drawText(text.c_str(), viewport->right - 88, viewport->top + 35);
	}

	particlePool.draw(&camera, viewport, false);
	userInterface.setRadarBounds(camera_bound_left, camera_bound_right, camera_bound_top, camera_bound_bottom);
	userInterface.setView(viewport->left, viewport->right, viewport->top, viewport->bottom);
	userInterface.draw();

	for (CLEntityPool::iterator ii = entpool.begin(); ii != entpool.end(); ++ii) {
		//bool sameTeam = false;
		//if (entpool.find(avatarID) != entpool.end()) {
		//	if ((ii->second->getTeam() == entpool[avatarID]->getTeam()) &&
		//		entpool[avatarID]->getTeam() != team_free) sameTeam = true;
		//}
		userInterface.radarDraw(
				ii->second->getPosition(),
				ii->second->getType(),
				ii->second->getTeam(),
				ii->second->getRadius(),
				ii->second->getID(),
				ii->second->effect,
				avatarID);
	}

	if (showBorders)
		drawMapBorders();

	//draw the edges between waypoints
	drawWaypointEdges();
	
	// draw rectangular regions
	drawRects();
	
	//GameX.ResetView();
	
	if(fadeOut > 0)
	{
		glColor4f(0, 0, 0, fadeOut);
		glBegin(GL_QUADS);
			glVertex2f(0,                         0);
			glVertex2f(graphics.getScreenWidth(), 0);
			glVertex2f(graphics.getScreenWidth(), graphics.getScreenHeight());
			glVertex2f(0,                         graphics.getScreenHeight());
		glEnd();
		
		if(connectionComplete == false && fadeOut >= 0.9) {
			graphics.drawTextCentered("Connecting...", graphics.getScreenWidth()/2, 350);
		}
	}
	if(fadeOut > targetFade) {
		fadeOut -= fadeSpeed * getRealDt();
		if(fadeOut <= targetFade) fadeOut = targetFade;
	} else if(fadeOut < targetFade) {
		fadeOut += fadeSpeed * getRealDt();
		if(fadeOut >= targetFade) fadeOut = targetFade;
	}
}

CLIENT_CONSOLE_COMMAND(fade_out)
{
	if(argc < 1) {
		console.printf("Usage: fade_out speed");
		return;
	}
	Client *client = (Client*)currentNode;
	client->fadeSpeed = atof(argv[0]);
	client->targetFade = 1.0;
}

CLIENT_CONSOLE_COMMAND(fade_in)
{
	if(argc < 1) {
		console.printf("Usage: fade_in speed");
		return;
	}
	Client *client = (Client*)currentNode;
	client->fadeSpeed = atof(argv[0]);
	client->targetFade = 0.0;
}

void Client::runCommand(RecvMessage &msg)
{
	clientScripting.command(msg);
}

void Client::addEntity(RecvMessage &msg)
{
	int		    entID	  = msg.getInt();
	std::string imageName = msg.getString();
	float		drawScale = msg.getFloat();	
	Position	pos;
	pos.readFromMessage(msg);
	int			type	  = msg.getInt();
	int			team	  = msg.getInt();
	std::string label     = msg.getString();
	float		radius	  = msg.getFloat();
	int         effect    = msg.getShort();

	
	if(entpool.find(entID) != entpool.end()) {
		// Received 'add' notification for an entity we already knew about.
		// In this case, the new definition supercedes the old one.
		delete entpool[entID];
	}
	entpool[entID] = new CLEntity(entID, imageName, drawScale, pos, type, team, label, effect);
	//console.printf("type is %i", type); 
	entpool[entID]->setRadius(radius);
}

void Client::updateEntity(RecvMessage &msg)
{
	int		    entID	  = msg.getInt();
	Position	pos;
	pos.readFromMessage(msg);
	
	if(entpool.find(entID) == entpool.end()) {
		// Update for a previously dropped entity. Well, not much to do with
		// that but ignore it.
		return;
	}
	entpool[entID]->setPosition(pos);
}

void Client::dropEntity(RecvMessage &msg)
{
	int	entID = msg.getInt();

	delete entpool[entID];
	entpool[entID] = 0;
	entpool.erase(entID);
}

void Client::centerCamera(RecvMessage &msg)
{
	Position cam;
	cam.readFromMessage(msg);
	cameraCenter = cam;
	camera_bound_left   = msg.getFloat();
	camera_bound_right  = msg.getFloat();
	camera_bound_top    = msg.getFloat();
	camera_bound_bottom = msg.getFloat();
}

void Client::print(RecvMessage &msg)
{
	std::string text  = msg.getString();
	short       where = msg.getShort();

	if(where & 1) {console.println(text.c_str());}
	if(where & 2) {userInterface.updateMessage(text);}
}

void Client::playSoundAt(RecvMessage &msg)
{
	std::string soundName       = msg.getString();
    const Position *listenerPos = getEntityPosition(shipEntId);

    if (listenerPos == 0)
        listenerPos = &camera;

	float sourceX = msg.getFloat();
	float sourceY = msg.getFloat();
	float relX    = sourceX - listenerPos->getX();
	float relY	  = sourceY - listenerPos->getY();

    relX /= soundRange;
    relY /= soundRange;

	audio.playSoundEx(
		*sounds.getSound(soundName.c_str(), true), 
		relX, 
		relY);
}

void Client::playSound(RecvMessage &msg)
{
    std::string soundName = msg.getString();

    //Sound sound;
    //sound.load(soundName.c_str(), false);

    audio.playSound(*sounds.getSound(soundName.c_str(), false));
}

void Client::setEngines(RecvMessage &msg) {
	int entID     = msg.getInt();
	float radius  = msg.getFloat();
	short engines = msg.getShort();
	entpool[entID]->setEngines(engines, radius);
}

void Client::explodeAt(RecvMessage &msg)
{
	float x				= msg.getFloat();
	float y				= msg.getFloat();
	float radius		= msg.getFloat();
	explosion_type type = explosion_type(msg.getInt());

	particlePool.explodeAt(Position(x, y), radius, type);
}

void Client::updateUI(RecvMessage &msg)
{
	float health = msg.getFloat();
	userInterface.updateHealth(health);
}

void Client::changeColor(RecvMessage &msg)
{
	int entID = msg.getInt();
	int red   = msg.getInt();
	int green = msg.getInt();
	int blue  = msg.getInt();

	//the entID could be invalid if the bot is trying to highlight a waypoint, 
	//but waypoints are hidden
	if (entpool.find(entID) == entpool.end())
		return;

	entpool[entID]->color = Color(red, green, blue);
	entpool[entID]->shouldChangeColor = true;
}

void Client::restoreColor(RecvMessage &msg)
{
	int entID = msg.getInt();

	if (entpool.find(entID) == entpool.end())
		return;

	entpool[entID]->shouldChangeColor = false;
}

void Client::updateScoreboard(RecvMessage &msg)
{
	showScoreboard = true;
	scoreText = msg.getString();
}

void Client::addEdge(RecvMessage &msg)
{
	int ent1ID = msg.getInt();
	int ent2ID = msg.getInt();

	edges.push_back(ClientEdge(ent1ID, ent2ID));
}

void Client::dropEdge(RecvMessage &msg)
{
	int ent1ID = msg.getInt();
	int ent2ID = msg.getInt();
	
	for (CLEdgePool::iterator ii = edges.begin(); ii != edges.end(); ++ii)
	{
		int currentEnt1ID = ii->getEnt1ID();
		int currentEnt2ID = ii->getEnt2ID();

		if ( (currentEnt1ID == ent1ID &&
			  currentEnt2ID == ent2ID) ||
			 (currentEnt2ID == ent1ID && 
			  currentEnt1ID == ent2ID))
		{
			ii = edges.erase(ii);
			--ii;
		}
	}
}

void Client::showAllEdges()
{
	areEdgesVisible = true;
}

void Client::hideAllEdges()
{
	areEdgesVisible = false;
}

void Client::clearEdges()
{
	edges.clear();
}


void Client::setRectangles(RecvMessage &msg)
{
	int numEdges = msg.getInt();
	rectpool.resize(numEdges);
	for(int ii=0; ii<numEdges; ii++)
	{
		rectpool[ii].name  = msg.getString();
		rectpool[ii].min_x = msg.getFloat();
		rectpool[ii].max_x = msg.getFloat();
		rectpool[ii].min_y = msg.getFloat();
		rectpool[ii].max_y = msg.getFloat();
	}
}

void Client::drawLightning(RecvMessage &msg)
{
    float startX = msg.getFloat();
    float startY = msg.getFloat();
    float endX   = msg.getFloat();
    float endY   = msg.getFloat();

    particlePool.lightningTrail(
        Position(startX, startY), 
        Position(endX, endY));
}

void Client::setDrawShield(RecvMessage &msg) {
	int ent = msg.getInt();
	short type = msg.getShort();
	float timer = msg.getFloat();
	float radius = msg.getFloat();
	entpool[ent]->setShield(timer,type,radius);
}

void Client::markAsAlly(RecvMessage &msg)
{
    int entID = msg.getInt();
    entpool[entID]->isAlly = true;
}

void Client::updateDrawEffect(RecvMessage &msg)
{
	int ID = msg.getInt();
	short effect = msg.getShort();
	entpool[ID]->setEffect(effect);
}

void Client::drawShield(int entID, float radius, float &time, int type) {
	Color c(0,0,0,0);
	switch (type) {
		case effect_spawnshield:	c.setComponents(160,160,255,180);		break;
		case effect_hitshield:		c.setComponents(255,160,160,180);		break;
		case effect_extrashield:	c.setComponents(120,220,220,180);		break;
		case effect_generatorshield:c.setComponents(220,220,120,200);		break;
		default:	return;
	}
	radius *= zoom;
	int innerDensity, outerDensity;
	if (time > 1.0 || time < -300.0) {
		innerDensity = 100;
		outerDensity = 80;
	} else if (time < -300.0) {
		innerDensity = radius*50;
		outerDensity = radius*20;
	} else {
		innerDensity = ceil(100 * time);
		outerDensity = ceil(80 * time);
	}
	for (int i = 0; i < innerDensity; i++) {
		float dotX = entpool[entID]->position.getX();
		float dotY = entpool[entID]->position.getY();
		convertToDrawCoords(dotX, dotY);
		float angle = randFloat(0.0, M_PI*2);
		float dist = randFloat(0.0, radius + 3.0);
		dotX += dist*cos(angle);
		dotY += dist*sin(angle);
		glEnable(GL_POINT_SMOOTH);
			if (type == effect_generatorshield) graphics.drawPoint(c,dotX,dotY,3);
			else graphics.drawPoint(c,dotX,dotY,2);
		glDisable(GL_POINT_SMOOTH);
	}
	for (int i = 0; i < outerDensity; i++) {
		float dotX = entpool[entID]->position.getX();
		float dotY = entpool[entID]->position.getY();
		convertToDrawCoords(dotX, dotY);
		float angle = randFloat(0.0, M_PI*2);
		dotX += (radius + 4.0)*cos(angle);
		dotY += (radius + 4.0)*sin(angle);
		glEnable(GL_POINT_SMOOTH);
			if (type == effect_generatorshield) graphics.drawPoint(c,dotX,dotY,3);
			else graphics.drawPoint(c,dotX,dotY,2);
		glDisable(GL_POINT_SMOOTH);
	}
	if (type != effect_generatorshield) time -= getDt();
	if (time < 0.0 && time > -300.0) {time = 0.0;}
}

//utility function; converts a point (x, y) from the server coordinates
//into client viewport drawing coordinates
void Client::convertToDrawCoords(float &x, float &y)
{
	x = (x * zoom) - floor(getCamera().getX() * zoom) + viewport->center_x;
	y = (y * zoom) - floor(getCamera().getY() * zoom) + viewport->center_y;
}

CLIENT_INVERSE_CONSOLE_COMMAND(show_scores)
{
	((Client*)currentNode) -> hideScoreboard();
}


CLIENT_CONSOLE_COMMAND(echo)
{
	std::string output("");
	for(int ii=0; ii<argc; ii++) {
		if(ii>0) output += " ";
		output += argv[ii];
	}
	console.printf("%s", output.c_str());
}

CLIENT_CONSOLE_COMMAND(change_resolution)
{
	if(argc < 2) 
	{
		console.printf("Usage: change_resolution width height");
		return;
	}

    graphics.changeResolution(atoi(argv[0]), atoi(argv[1]));
	Client *cl = (Client*)currentNode;
	cl->resetViewports(graphics.getScreenWidth(), graphics.getScreenHeight());
	cl->stars.reinit();
}

CLIENT_CONSOLE_COMMAND(exit)
	{ exit(0); }
CLIENT_CONSOLE_COMMAND(quit)
	{ exit(0); }

static void changeFullScreen(bool fullscreen);
ClientConsoleVar<bool> fullScreen("fullscreen", true, changeFullScreen);

void changeFullScreen(bool fullscreen) {
	/*if( GameX.IsFullScreen() != fullscreen )
		GameX.ToggleFullscreen();*/
	graphics.setFullScreen(fullscreen);
}
CLIENT_CONSOLE_COMMAND(toggle_fullscreen) {
	fullScreen = !fullScreen;
}

/*
 * 'Player' command: Run a console command as the given player index (for
 * splitscreen). This command needs three versions: one for regular commands,
 * and one each for the direct and inverse versions of invertible commands.
 */
void playerize(int argc, const char **argv, int mode)
 /* mode:
  *  0 normal
  *  1 invertible
  * -1 inverse     */
{
	int playernum;
	
	if(argc < 2) {
		if(mode >= 0)
			console.printf("Usage: player [playernum] [command]");
		return;
	}
	playernum = atoi(argv[0]) - 1;
	if( clients.find(playernum) == clients.end() ) {
		console.printf("Player %i is not in the game.", playernum+1);
		return;
	}
	
	GameNode *prevNode = currentNode;
	currentNode = clients[playernum];
		if(mode==-1)
			clientScripting.runCommandInverted(argv+1, argc-1, playernum);
		else
			clientScripting.runCommand(argv+1, argc-1, playernum);
	currentNode = prevNode;
}

CLIENT_CONSOLE_COMMAND(player)
{
	playerize(argc, argv, 0);
}
CLIENT_INVERTIBLE_CONSOLE_COMMAND(player)
{
	playerize(argc, argv, 1);
}
CLIENT_INVERSE_CONSOLE_COMMAND(player)
{
	playerize(argc, argv, -1);
}

static void changeName(std::string newname)
{
	if(currentNode->isConnected())
	{
		SendMessage namechange(message_name);
			namechange << newname;
		namechange.sendToServer();
	}
}

static void changeMusicStatus(bool playing)
{
	if(playing)
		resumeMusic(playing);
	else
		stopMusic();
}


CLIENT_CONSOLE_COMMAND(soundtrack)
{
	if(argc < 1) 
	{
		printfToClient(who, "Not enough arguments to soundtrack (should be 1)");
		printfToClient(who, "Usage:  soundtrack song [repeat]");

		return;
	}

	//use the repeat flag if specified.  Else, assume repeating.
	if (argc >= 2)
		setSoundtrack(argv[0], atoi(argv[1]));
	else
		setSoundtrack(argv[0], true);
}

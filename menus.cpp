#include "mastrix.hpp"


class MenuBackground :public EventHandler
{
public:
	MenuBackground();
	~MenuBackground();
	
	bool handleEvent(SDL_Event *ev) { return false; }
	void redraw(void);
	bool timepass(void);
	int getPriority(void) { return 4; }
	
private:
	struct BackgroundObject {
		Position pos;
		Image *img;
	};
	Starfield *stars;
	Position camerapos;
	typedef std::list<BackgroundObject> BackgroundObjectPool;
	BackgroundObjectPool objects;
	float nextObjectTime;
	bool prebuffered;
} menuBackground;

///////////////////////////////////////////////////////////////////////////////

class MenuEventHandler :public EventHandler
{
protected:
	void redraw(void) { menuBackground.redraw(); }
	bool timepass(void) { return menuBackground.timepass(); }
	
	void onEnable(void)  { SDL_ShowCursor(SDL_ENABLE); }
	void onDisable(void) { SDL_ShowCursor(SDL_DISABLE); }
	int getPriority(void) { return 3; }
};

///////////////////////////////////////////////////////////////////////////////

class MainMenu :public MenuEventHandler
{
public:
	MainMenu();
	
	bool handleEvent(SDL_Event *ev);
	void redraw(void);
	bool timepass(void);
	
private:
	std::vector<const char*> menuItems;
	std::vector<bool> optionsHidden;
	
	int mouseOverSelection(int x, int y);
	void doMenuAction(int entry);
	int selection;
	Image *title;
} mainMenu;

///////////////////////////////////////////////////////////////////////////////

class JoinGameMenu :public MenuEventHandler
{
public:
	JoinGameMenu();
	
	void next(void) { mainMenu.enable(); }
	bool handleEvent(SDL_Event *ev);
	void redraw(void);
	bool timepass(void);
	int mouseOverSelection(float x, float y);
	void doMenuAction(int action);
	
private:
	int selection;
	void rebuildList(void);
	std::vector<std::string> serverList;
	bool listFinished;
};
static void joinGameMenu(void);

///////////////////////////////////////////////////////////////////////////////

class ShipMenu :public MenuEventHandler
{
public:
	ShipMenu();

	bool handleEvent(SDL_Event *ev);
	void redraw();
	bool timepass() { return MenuEventHandler::timepass(); }

private:
	std::vector<const char *> menuItems;
	std::vector<Image *> shipImages;

	int mouseOverSelection(int x, int y);
	void doMenuAction(int entry);
	int selection;

	bool prebuffered;
} shipMenu;

///////////////////////////////////////////////////////////////////////////////

class ShipHandler :public EventHandler
{
public:
	bool handleEvent(SDL_Event *ev) { return false;}
	void redraw(void)               { shipMenu.redraw();}
	bool timepass(void)             { return shipMenu.timepass();}
	int getPriority(void)           { return 5;}
	bool isOpaque(void)             { return true;}
} shipHandler;

///////////////////////////////////////////////////////////////////////////////

class TextScreen :public MenuEventHandler
{
public:
	TextScreen();
	
	bool handleEvent(SDL_Event *ev);
	void redraw(void);
	bool timepass(void);
	
	virtual void next(void) = 0;
	virtual const char *getText(void) = 0;
};

///////////////////////////////////////////////////////////////////////////////

class ErrorScreen :public TextScreen
{
public:
	ErrorScreen(const char *text) { this->text = text; }
	void next(void) { mainMenu.enable(); }
	const char *getText() { return text.c_str(); }
protected:
	std::string text;
};

///////////////////////////////////////////////////////////////////////////////

class CreditsScreen :public TextScreen {
	void next(void) { mainMenu.enable(); }
	const char *getText(void) { return
		"Mastrix\n"
		"Jim Babcock\n"
		"Eric Hayes\n"
		"Christian Montoya\n"
		"Aaron Smith\n"
		"Yi Xu";
	}
} creditsScreen;

///////////////////////////////////////////////////////////////////////////////

class ControlsScreen :public TextScreen {
	void next(void) { mainMenu.enable(); }
	const char *getText(void) { return
		"Controls:\n\n"
		"W, num 8, arrow up - forward\n"
		"S, num 5, arrow down - reverse\n"
		"A, num 4, arrow left - strafe left\n"
		"D, num 6, arrow right - strafe right\n"
		"left click - fire primary weapon\n"
		"right click - fire secondary weapon\n"
		"mousewheel, num plus, E - change secondary weapon\n"
		"alt + enter - toggle fullscreen\n"
		"TAB - show score\n"
		"F3 - set / change name\n"
		"F4 - change team in a networked game";
	}
} controlsScreen;

///////////////////////////////////////////////////////////////////////////////
//taken out for the showcase!
class GameOverScreen2 :public TextScreen {
	void next(void) { new FadeHandler(this, &mainMenu); }
	const char *getText(void) { return
		"Biatch";
	}
} gameOverScreen2;

class GameOverScreen1 :public TextScreen {
	void next(void) { new FadeHandler(this, &mainMenu); }
	const char *getText(void) { return
		"Game Over";
	}
} gameOverScreen;

class GameCompletedScreen2 :public TextScreen
{
	void next(void) { new FadeHandler(this, &mainMenu); }
	const char *getText(void)
	{
		return "...Two years later, Earth runs out of oil again...";
	}
} gameCompletedScreen2;

class GameCompletedScreen :public TextScreen
{
	void next(void) { new FadeHandler(this, &gameCompletedScreen2); }
	const char *getText(void)
	{
		return "Congratulations, pilot!  You have helped Earth achieve a great victory\n\
			    today.  Thanks to you, Earth will have enough resources for many generations\n\
				to come.  The Mastrix Defense Force threat has been quelled, and we are free\n\
				to exploit them as we please.  Your name will live on forever in Earth's history,\n\
				and you will always be remembered as a hero and champion of the world.";
				
	}
} gameCompletedScreen;

///////////////////////////////////////////////////////////////////////////////


void showMainMenu(void) { mainMenu.enable(); }
void showGameOver(void) { new FadeHandler(&mainGameHandler, &gameOverScreen); }
void showGameCompleted(void) 
{
	new FadeHandler(&mainGameHandler, &gameCompletedScreen);
}

SERVER_CONSOLE_COMMAND(game_over) {
	showGameOver();
}

SERVER_CONSOLE_COMMAND(game_completed)
{
	showGameCompleted();
}



/////////////////////////////////////////////////////////////////////////////
//                         Menu Background                                 //
/////////////////////////////////////////////////////////////////////////////
MenuBackground::MenuBackground()
{
	camerapos = Position(0, 0);
	camerapos.setY_vel(200.0);
	nextObjectTime = getRealTime() + 0.5;
	stars = new Starfield();
	prebuffered = false;
}
MenuBackground::~MenuBackground()
{
	delete stars;
}
void MenuBackground::redraw(void)
{
	if(!prebuffered)
	{
		prebuffered = true;
		images.getImage("images/planet-drilled.png");
		images.getImage("images/planet-neptune.png");
		images.getImage("images/planet-mars.png");
		images.getImage("images/moon-enceladus.png");
		images.getImage("images/moon-io.png");
	}
	
	stars->draw(camerapos, &viewport_full);
	
	for(BackgroundObjectPool::iterator ii=objects.begin(); ii!=objects.end();)
	{
		graphics.drawImage(
			ii->img, 
			ii->pos.getX() - camerapos.getX(), 
			ii->pos.getY() - camerapos.getY(), 
			ii->pos.getR(), 
			0.5);
		if(ii->pos.getY() < -ii->img->getWidth())
			ii = objects.erase(ii);
		else
			ii++;
	}
	
	// Shade the screen to lower brightness
	glColor4f(0, 0, 0, 0.3);
	glBegin(GL_QUADS);
		glVertex2f(0,                         0);
		glVertex2f(graphics.getScreenWidth(), 0);
		glVertex2f(graphics.getScreenWidth(), graphics.getScreenHeight());
		glVertex2f(0,                         graphics.getScreenHeight());
	glEnd();
}
bool MenuBackground::timepass(void)
{
	if(getRealTime() >= nextObjectTime)
	{
		BackgroundObject addition;
		addition.pos = Position(randFloat(0, graphics.getScreenWidth()), graphics.getScreenHeight()+camerapos.getY()+512);
		switch(randInt(0, 4))
		{
			default:
			case 0: addition.img = images.getImage("images/planet-drilled.png"); break;
			case 1: addition.img = images.getImage("images/planet-neptune.png"); break;
			case 2: addition.img = images.getImage("images/planet-mars.png"); break;
			case 3: addition.img = images.getImage("images/moon-enceladus.png"); break;
			case 4: addition.img = images.getImage("images/moon-io.png"); break;
		}
		objects.push_front(addition);
		nextObjectTime = getRealTime() + 5;
	}
	return false;
}




/////////////////////////////////////////////////////////////////////////////
//                            Main Menu                                    //
/////////////////////////////////////////////////////////////////////////////

bool needSinglePlayerInit = false;
bool gameRunning = false;


MainMenu::MainMenu()
{
	title = NULL;
	selection = 1;
	
	menuItems.push_back("Resume Game");
	menuItems.push_back("Single Player");
	menuItems.push_back("Host Game");
	menuItems.push_back("Join Game");
	menuItems.push_back("Controls");
	menuItems.push_back("Credits");
	menuItems.push_back("Quit");
	optionsHidden.resize(menuItems.size(), false);
	optionsHidden[0] = true;
}

const int mainMenuTop = 400;
const int entrySpacing = 25;


bool MainMenu::handleEvent(SDL_Event *ev)
{
	switch(ev->type)
	{
		case SDL_KEYDOWN:
			switch(ev->key.keysym.sym)
			{
				case SDLK_DOWN:
				case SDLK_KP2:
					selection++;
					if(selection>=(signed)menuItems.size()) selection=0;
					break;
				case SDLK_UP:
				case SDLK_KP8:
					selection--;
					if(selection<0) selection=menuItems.size()-1;
					break;
				case SDLK_RETURN:
				case SDLK_KP_ENTER:
					if(ev->key.keysym.mod & KMOD_ALT) {
						fullScreen = !fullScreen;
					} else
						doMenuAction(selection);
					break;
			}
			return true;

		case SDL_MOUSEBUTTONDOWN:
			if(mouseOverSelection(ev->button.x, ev->button.y) != -1)
				doMenuAction(mouseOverSelection(ev->button.x, ev->button.y));
			return true;
		
		case SDL_MOUSEMOTION:
			if(mouseOverSelection(ev->motion.x, ev->motion.y) != -1)
				selection = mouseOverSelection(ev->motion.x, ev->motion.y);
			break;

		default:
			return false;
	}
	return false;
}

void MainMenu::redraw(void)
{
	float center = graphics.getScreenWidth()/2;
	// Draw highlight behind selected menu item
	float y = mainMenuTop + selection*entrySpacing;
	
	MenuEventHandler::redraw();
	
	// Show 'resume game' only if a client is active
	if(clients.size()) optionsHidden[0] = false;
	else               optionsHidden[0] = true;
	// If already a server, disable 'host game'
	if(networkServer) optionsHidden[2] = true;
	else              optionsHidden[2] = false;
	
	glBegin(GL_QUADS);
		glColor4f(0.0, 0.5, 1.0, 0.3);
			glVertex2f(center-50, y+3);
			glVertex2f(center+50, y+3);
			glVertex2f(center+50, y-entrySpacing+3);
			glVertex2f(center-50, y-entrySpacing+3);
		
			glVertex2f(center-50, y+3);
			glVertex2f(center-50, y-entrySpacing+3);
		glColor4f(0.0, 0.5, 1.0, 0.0);
			glVertex2f(center-130, y-entrySpacing+3);
			glVertex2f(center-130, y+3);
		
		glColor4f(0.0, 0.5, 1.0, 0.3);
			glVertex2f(center+50, y+3);
			glVertex2f(center+50, y-entrySpacing+3);
		glColor4f(0.0, 0.5, 1.0, 0.0);
			glVertex2f(center+130, y-entrySpacing+3);
			glVertex2f(center+130, y+3);
	glEnd();
	
	if(!title) title = images.getImage("images/title.png");
	graphics.drawImage(title, center, 200);
	
	// Draw choices
	glColor3ub(255,255,255);
	for(unsigned ii=0; ii<menuItems.size(); ii++)
	{
		if(optionsHidden[ii])
			continue;
		graphics.drawTextCentered(
			menuItems[ii],
			center,
			mainMenuTop+ii*entrySpacing,
			GLUT_BITMAP_HELVETICA_18);
	}
}

bool MainMenu::timepass(void)
{
	return MenuEventHandler::timepass();
}

void MainMenu::doMenuAction(int entry)
{
	switch(entry)
	{
		case 0:
			if(!clients.size()) // No game running
				break;
			remove();
			mainGameHandler.enable();
			clientScripting.command("unfreeze", false);
			break;
		case 1:
			gameRunning = true;
			startSinglePlayer();
//			new FadeHandler(this, &mainGameHandler);
			remove();
			mainGameHandler.enable();
			clientScripting.command("unfreeze", false);
			break;
		case 2:
			gameRunning = true;
			startNetworkServer();
			remove();
			mainGameHandler.enable();
			clientScripting.command("unfreeze", false);
			break;
		case 3:
			remove();
			joinGameMenu();
			break;
		case 4:
			remove();
			controlsScreen.enable();
			break;
		case 5:
			remove();
			creditsScreen.enable();
			break;
		case 6:
			exit(0);
			break;
	}
}

int MainMenu::mouseOverSelection(int x, int y)
{
	float center = graphics.getScreenWidth()/2;
	if(x < center-150 || x > center+150) return -1;
	
	int ret = (y-mainMenuTop+entrySpacing-3) / entrySpacing;
	
	if(ret < 0 || ret >= (signed)menuItems.size()) return -1;
	if(optionsHidden[ret]) return -1;
	return ret;
}


CLIENT_CONSOLE_COMMAND(mainmenu)
{
	if(!networkClient)
		clientScripting.command("freeze", false);
	mainMenu.enable();
	mainGameHandler.remove();
}

void Client::serverDisconnect(RecvMessage &msg)
{
	std::string cause = msg.getString();
//	errorDialog((char*)cause.c_str());
//	exit(0);
	if(gameRunning) {
		EventHandler *handler = new ErrorScreen(cause.c_str());
		mainGameHandler.remove();
		handler->enable();
		gameRunning = false;
	}
}


/////////////////////////////////////////////////////////////////////////////
//                         Credits Screen                                  //
/////////////////////////////////////////////////////////////////////////////

const float textTop = 250;

TextScreen::TextScreen()
{
}

bool TextScreen::handleEvent(SDL_Event *ev)
{
	switch(ev->type)
	{
		case SDL_KEYDOWN:
		case SDL_MOUSEBUTTONDOWN:
			if(fadeInProgress) return false;
			remove();
			next();
			return true;
			
		default:
			return false;
	}
	return false;
}
void TextScreen::redraw(void)
{
	char *text = strdup(getText());
	char *pos;
	
	float center = graphics.getScreenWidth()/2;
	int num=0;
	
	MenuEventHandler::redraw();
	
	pos = strtok(text, "\n");
	
	while(pos) {
		graphics.drawTextCentered(
			pos,
			center,
			textTop+(num++)*entrySpacing,
			GLUT_BITMAP_HELVETICA_18);
		pos = strtok(NULL, "\n");
	}
	free(text);
}
bool TextScreen::timepass(void)
{
	return MenuEventHandler::timepass();
}


/////////////////////////////////////////////////////////////////////////////
//                         Server Picker                                   //
/////////////////////////////////////////////////////////////////////////////
JoinGameMenu *joinMenu = NULL;

const float pickerTop = 250;
const float pickerSpacing = 18;
float cancelButtonX = 90,
      cancelButtonY = 700;
float refreshButtonX = 250,
      refreshButtonY = 700;

static void joinGameMenu(void)
{
	if(!joinMenu) joinMenu = new JoinGameMenu();
	joinMenu->enable();
}

JoinGameMenu::JoinGameMenu()
{
	requestServerList();
}
bool JoinGameMenu::handleEvent(SDL_Event *ev)
{
	switch(ev->type)
	{
		case SDL_KEYDOWN:
			switch(ev->key.keysym.sym)
			{
				case SDLK_DOWN:
				case SDLK_KP2:
					selection++;
					if(selection>=(signed)serverList.size())
						selection=0;
					break;
				case SDLK_UP:
				case SDLK_KP8:
					selection--;
					if(selection<0)
						selection=serverList.size()-1;
					break;
				case SDLK_RETURN:
				case SDLK_KP_ENTER:
					if(ev->key.keysym.mod & KMOD_ALT) {
						fullScreen = !fullScreen;
					} else if(serverList.size()) {
						doMenuAction(selection);
					}
					break;
			}
			return true;

		case SDL_MOUSEBUTTONDOWN:
			if(mouseOverSelection(ev->button.x, ev->button.y) != INT_MAX)
				doMenuAction(mouseOverSelection(ev->button.x, ev->button.y));
			return true;
		
		case SDL_MOUSEMOTION:
			if(mouseOverSelection(ev->motion.x, ev->motion.y) != INT_MAX)
				selection = mouseOverSelection(ev->motion.x, ev->motion.y);
			break;
			
		default:
			return false;
	}
	return false;
}
int JoinGameMenu::mouseOverSelection(float x, float y)
{
	if(x > cancelButtonX-70 && x<cancelButtonX+70 &&
	   y > cancelButtonY-30 && y<cancelButtonY+5)
		return -1;
	if(x > refreshButtonX-70 && x<refreshButtonX+70 &&
	   y > refreshButtonY-30 && y<refreshButtonY+5)
		return -2;
	
	float center = graphics.getScreenWidth()/2;
	if(x < center-150 || x > center+150) return INT_MAX;
	
	int ret = (y-pickerTop+pickerSpacing-3) / pickerSpacing;
	
	if(ret < 0 || ret >= (signed)serverList.size()) return INT_MAX;
	else return ret;
}
void JoinGameMenu::doMenuAction(int action)
{
	switch(action)
	{
		case -2:
			requestServerList();
			break;
		case -1:
			remove();
			next();
			break;
		case INT_MAX:
			break;
		default:
			if(action<0 || action>=serverList.size())
				break;
			gameRunning = true;
			startNetworkClient(serverList[action].c_str());
			remove();
			mainGameHandler.enable();
			break;
	}
}
void JoinGameMenu::redraw(void)
{
	float center = graphics.getScreenWidth()/2;
	
	MenuEventHandler::redraw(); // draw background
	rebuildList(); // Get list of servers
	
	// Place highlight
	float x, y;
	switch(selection)
	{
		default:
			y = pickerTop + selection*pickerSpacing + 3;
			x = center;
			break;
		case -1: x =  cancelButtonX, y =  cancelButtonY; break;
		case -2: x = refreshButtonX, y = refreshButtonY; break;
	}
	// Draw highlight
	glBegin(GL_QUADS);
		glColor4f(0.0, 0.5, 1.0, 0.3);
			glVertex2f(x-50, y);
			glVertex2f(x+50, y);
			glVertex2f(x+50, y-pickerSpacing);
			glVertex2f(x-50, y-pickerSpacing);
		
			glVertex2f(x-50, y);
			glVertex2f(x-50, y-pickerSpacing);
		glColor4f(0.0, 0.5, 1.0, 0.0);
			glVertex2f(x-130, y-pickerSpacing);
			glVertex2f(x-130, y);
		
		glColor4f(0.0, 0.5, 1.0, 0.3);
			glVertex2f(x+50, y);
			glVertex2f(x+50, y-pickerSpacing);
		glColor4f(0.0, 0.5, 1.0, 0.0);
			glVertex2f(x+130, y-pickerSpacing);
			glVertex2f(x+130, y);
	glEnd();
	
	// Draw list of servers
	if(!serverList.size()) {
		if(listFinished)
			graphics.drawTextCentered("No servers found.", center, pickerTop);
		else
			graphics.drawTextCentered("Checking for servers...", center, pickerTop);
	}
	for(int ii=0; ii<serverList.size(); ii++)
		graphics.drawTextCentered(serverList[ii].c_str(), center, pickerTop+pickerSpacing*ii);
	
	// Draw peripheral buttons
	graphics.drawTextCentered("Cancel", cancelButtonX, cancelButtonY, GLUT_BITMAP_HELVETICA_18);
	graphics.drawTextCentered("Refresh", refreshButtonX, refreshButtonY, GLUT_BITMAP_HELVETICA_18);
}
void JoinGameMenu::rebuildList()
{
	std::string serversString = getServerList();
	char *tokstr = strdup(serversString.c_str());
	listFinished = serverListFinished();
	
	serverList.clear();
	
	char *pos = strtok(tokstr, "\n\r\x1a");
	do {
		if(pos)
			serverList.push_back(std::string(pos));
	} while((pos = strtok(NULL, "\n\r\x1a")));
	
	free(tokstr);
}


bool JoinGameMenu::timepass(void)
{
	return MenuEventHandler::timepass();
}

/////////////////////////////////////////////////////////////////////////////
//                         Ship Screen                                     //
/////////////////////////////////////////////////////////////////////////////

ShipMenu::ShipMenu()
{
	selection = 0;
	
	menuItems.push_back("leet ship");
	menuItems.push_back("Done");

	prebuffered = false;
}

const int shipMenuTop = 400;
const int shipEntrySpacing = 25;


bool ShipMenu::handleEvent(SDL_Event *ev)
{
	switch(ev->type)
	{
		case SDL_KEYDOWN:
			switch(ev->key.keysym.sym)
			{
				case SDLK_DOWN:
				case SDLK_KP2:
					selection++;
					if(selection>=(signed)menuItems.size()) selection=0;
					break;
				case SDLK_UP:
				case SDLK_KP8:
					selection--;
					if(selection<0) selection=menuItems.size()-1;
					break;
				case SDLK_RETURN:
				case SDLK_KP_ENTER:
					if(ev->key.keysym.mod & KMOD_ALT) {
						fullScreen = !fullScreen;
					} else
						doMenuAction(selection);
					break;
			}
			return true;

		case SDL_MOUSEBUTTONDOWN:
			if(mouseOverSelection(ev->button.x, ev->button.y) != -1)
				doMenuAction(mouseOverSelection(ev->button.x, ev->button.y));
			return true;
		
		case SDL_MOUSEMOTION:
			if(mouseOverSelection(ev->motion.x, ev->motion.y) != -1)
				selection = mouseOverSelection(ev->motion.x, ev->motion.y);
			break;

		default:
			return false;
	}
	return false;
}

void ShipMenu::redraw(void)
{
	if (!prebuffered)
	{
		prebuffered = true;

		shipImages.push_back(images.getImage("images/ship.png"));
	}

	float center = graphics.getScreenWidth()/2;
	// Draw highlight behind selected menu item
	float y = mainMenuTop + selection*shipEntrySpacing;
	
	MenuEventHandler::redraw();
	
	glBegin(GL_QUADS);
		glColor4f(0.0, 0.5, 1.0, 0.3);
			glVertex2f(center-50, y+3);
			glVertex2f(center+50, y+3);
			glVertex2f(center+50, y-shipEntrySpacing+3);
			glVertex2f(center-50, y-shipEntrySpacing+3);
		
			glVertex2f(center-50, y+3);
			glVertex2f(center-50, y-shipEntrySpacing+3);
		glColor4f(0.0, 0.5, 1.0, 0.0);
			glVertex2f(center-130, y-shipEntrySpacing+3);
			glVertex2f(center-130, y+3);
		
		glColor4f(0.0, 0.5, 1.0, 0.3);
			glVertex2f(center+50, y+3);
			glVertex2f(center+50, y-shipEntrySpacing+3);
		glColor4f(0.0, 0.5, 1.0, 0.0);
			glVertex2f(center+130, y-shipEntrySpacing+3);
			glVertex2f(center+130, y+3);
	glEnd();
	
	
	// Draw choices
	glColor3ub(255,255,255);
	for(unsigned ii=0; ii<menuItems.size(); ii++)
	{
		graphics.drawTextCentered(
			menuItems[ii],
			center, 
			mainMenuTop+ii*shipEntrySpacing,
			GLUT_BITMAP_HELVETICA_18);
	}
}

//bool ShipMenu::timepass(void)
//{
//	return ShipHandler::timepass();
//}

void ShipMenu::doMenuAction(int entry)
{
	switch(entry)
	{
		case 0:
			new FadeHandler(this, &mainGameHandler);
			break;
		case 1:
			new FadeHandler(this, &mainGameHandler);
			break;
	}
}

int ShipMenu::mouseOverSelection(int x, int y)
{
	float center = graphics.getScreenWidth()/2;
	if(x < center-150 || x > center+150) return -1;
	
	int ret = (y-mainMenuTop+shipEntrySpacing-3) / shipEntrySpacing;
	
	if(ret < 0 || ret >= (signed)menuItems.size()) return -1;
	else return ret;
}
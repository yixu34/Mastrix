#include "mastrix.hpp"

UI::UI()
{
	cursor = images.getImage("images/crosshair.png");
}

UI::~UI()
{
	cursor = 0;
}

void UI::setView(int l, int r, int t, int b)
{
	hm.setView(l, r, t, b);
	mb.setView(l, r, t, b);
	rd.setView(l, r, t, b);
	wi.setView(l, r, t, b);
	en.setView(l, r, t, b);
}

void UI::draw()
{
	hm.draw();
	mb.draw();
	rd.draw();
	wi.draw();
	en.draw();

	graphics.drawImage(cursor, getMouseX(), getMouseY());
}

void UI::radarDraw(const Position &pos, int entType, int team, float size, int ID, int effect, int avatar)
{
	rd.draw(pos, entType, team, size, ID, effect, avatar);
}

void UI::setRadarBounds(float l, float r, float t, float b)
{
	rd.setBounds(l,r,t,b);
}

void UI::updateHealth(float newHealth)
{
	hm.draw(newHealth);
}

void UI::updateMessage(std::string message)
{
	mb.draw(message);
}

void UI::updateEngineDiamond(RecvMessage &msg)
{
	float f=msg.getFloat(),
	      l=msg.getFloat(),
	      r=msg.getFloat(),
	      d=msg.getFloat(),
	      w=msg.getFloat();
	en.setValues(f,l,r,d,w);
}

CLIENT_CONSOLE_COMMAND(menu)
{
	if(argc != 1) {
		console.printf("Usage: menu [selection]");
		return;
	}
	int index = atoi(argv[0]) - 1; // Indices are given in 1-9 but used as 0-8
	if(index<0)
		return;
	Client *cl = (Client*)currentNode;
	std::string cmd("");
	if(cl->showMenu && cl->menuCommands.size() > (unsigned)index)
		cmd = cl->menuCommands[index].c_str();
	cl->showMenu = false;
	
	if(cmd != "")
		console.command(cmd);
}

CLIENT_CONSOLE_COMMAND(show_menu)
{
	if(!(argc % 2)) {
		console.printf("Usage: show_menu [header] [text1][command1] [text2][command2] ...");
		return;
	}
	std::string text(argv[0]);
	Client *cl = (Client*)currentNode;
	cl->menuCommands.clear();
	
	for(int ii=1; (ii+1)<argc; ii+=2)
	{
		text += retprintf("\n%i. %s", (ii+1)/2, argv[ii]);
		cl->menuCommands.push_back( std::string(argv[ii+1]) );
	}
	cl->menuText = text;
	cl->showMenu = true;
}

CLIENT_CONSOLE_COMMAND(close_menu)
{
	Client *cl = (Client*)currentNode;
	cl->showMenu = false;
	cl->menuText = "";
	cl->menuCommands.clear();
}



EngineDiamond::EngineDiamond() {
	img = images.getImage("images/Sphere1.png");
	forward=left=right=down=1; warmup=0;
}
void EngineDiamond::setView(int l, int r, int t, int b)
{
	x = l+73;
	y = t+15;
}

void EngineDiamond::draw(void)
{
	float offset = 9;
	
	glColor3f(1-forward, 1-forward-warmup, 1-forward-warmup);
	graphics.drawColoredImage(img, x, y+offset, 0, 0.15);
	
	glColor3f(1-left, 1-left, 1-left);
	graphics.drawColoredImage(img, x-offset, y, 0, 0.15);
	glColor3f(1-right, 1-right, 1-right);
	graphics.drawColoredImage(img, x+offset, y, 0, 0.15);
	glColor3f(1-down, 1-down, 1-down);
	graphics.drawColoredImage(img, x, y-offset, 0, 0.15);
}

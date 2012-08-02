//drawing code for the user interface
#ifndef UI_HPP
#define UI_HPP

/*
class HUD_Element {
public:
	update();
	draw();
}
*/

struct ScreenMessage {
	std::string msg;
	float lifetime;
};

struct RadarBlip {
	int r;
	int g;
	int b;
};

class Healthmeter
{
public:
	Healthmeter();
	void setView(int l, int r, int t, int b);
	void draw();
	void draw(float length);

private:
	void updateColor(void);
	
	int r, g, b;
	float length, tempLength;

	float x, y;

	Image *img;
};

class Radar
{
public:
	Radar();
	void setView(int l, int r, int t, int b);
	void setBounds(float l, float r, float t, float b);
	void draw();
	void draw(const Position &pos, int type, int team, float size, int ID, int effect, int avatar);

private:
	float xleft, xright, ytop, ybottom;
	float boundleft, boundright, boundtop, boundbottom;
	float dimhor, dimvert;

	Color radarColor;
};

class WeaponInfo
{
public:
	WeaponInfo();
	void setView(int l, int r, int t, int b);
	void draw();
	void updateWeapons(RecvMessage &msg);

protected:
	struct WeaponInfoEntry {
		std::string name;
		int ammo;
	};
	float x,xmax,y;
	int currWeapon;
	std::vector<WeaponInfoEntry> weapons;
};

class Messagebox
{
public:
	Messagebox();
	void setView(int l, int r, int t, int b);
	void draw();
	void draw(std::string message);

private:

	typedef std::deque<ScreenMessage> MessageQueue;
	MessageQueue messageList;
	float xleft, xright, ytop, ybottom;
	//int numLines;
	int lineHeight;
};

class EngineDiamond
{
public:
	EngineDiamond();
	void setView(int l, int r, int t, int b);
	void setValues(float f, float l, float r, float d, float w)
		{ forward=f; left=l; right=r; down=d; warmup=w; }
	void draw();
	
protected:
	Image *img;
	float x, y;
	float forward, left, right, down, warmup;
};

class UI
{
public:
	UI();

	~UI();

	void setView(int l, int r, int t, int b);
	void radarDraw(const Position &pos);
	void radarDraw(const Position &pos, int entType, int team, float size, int ID, int effect, int avatar);
	void setRadarBounds(float l, float r, float t, float b);
	void draw();

	void updateHealth(float health);
	void updateMessage(std::string message);
	void updateEngineDiamond(RecvMessage &msg);
	void updateWeaponInfo(RecvMessage &msg) { wi.updateWeapons(msg); }

private:
	Healthmeter hm;
	Messagebox mb;
	Radar rd;
	WeaponInfo wi;
	EngineDiamond en;

	Image *cursor;
};

#endif	//UI_HPP


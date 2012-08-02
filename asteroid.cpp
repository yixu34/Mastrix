#include "mastrix.hpp"

ServerConsoleVar<float> astDrop("ast_drop", 0);
ServerConsoleVar<std::string> astDropCmd("ast_drop_cmd", "");
ServerConsoleVar<float> astSpeed("ast_speed", 30);

class Asteroid :public Entity
{
public:
	Asteroid(float x, float y, int size);
	void die(int killer);
	void takeDamage(float amt, int source);
	void timepass(void);
	bool canCollideWith(Entity *ent) { return !ent->isAsteroid(); }
	
	bool staysInBounds(void) { return false; }
	int getCollisionEffect(void) { return collide_bounce | collide_damage | collide_no_gravity; }
	float getCollisionDamage() { return 3.0; }
	bool isAsteroid() { return true; }
protected:
	int size;
	float hps;
};

SERVER_CONSOLE_COMMAND(asteroid)
{
	if(argc != 3) {
		printfToClient(who, "Usage: asteroid x y size");
		return;
	}
	
	float pos_x = atof(argv[0]),
	      pos_y = atof(argv[1]);
	int size = atoi(argv[2]);
	
	Asteroid *ent = new Asteroid(pos_x, pos_y, size);
	serverScripting.defineVariable("_", retprintf("%i", ent->getEntId()));
}

SERVER_CONSOLE_COMMAND(astfield)
{
	if(argc != 2) {
		printfToClient(who, "Usage: astfield region count");
		return;
	}
	Server::Region field = server->findRegion(argv[0]);
	int count = atoi(argv[1]);
	
	for(int ii=0; ii<count; ii++)
	{
		float x = randFloat(field.min_x, field.max_x),
		      y = randFloat(field.min_y, field.max_y);
		new Asteroid(x, y, randInt(1,3));
	}
}

Asteroid::Asteroid(float x, float y, int size)
{
	explodeType = explode_asteroid;
	entType   = ent_large_obstacle;
	this->size = size;
	switch(size) {
		case 1: sprite = "images/ast-small.png"; drawScale = 1.0; break;
		case 2: sprite = "images/ast-med.png";   drawScale = 0.4; break;
		case 3: sprite = "images/ast-large.png"; drawScale = 0.5; break;
	}
	position = Position(x, y);
	position.setX_vel(randFloat(-astSpeed, astSpeed));
	position.setY_vel(randFloat(-astSpeed, astSpeed));
	
	Image *img = images.getImage(sprite.c_str());
	assert(img);
	radius = (img->getWidth() / 2) * drawScale;
	this->hps = 1.0;
	
	sendAdd();
}

void Asteroid::die(int killer)
{
	Entity::die(killer);
	if(size>1) {
		float separationX = randFloat(40, 120);
		float separationY = randFloat(40, 120);
		for(int ii=0; ii<2; ii++) {
			Asteroid *ast = new Asteroid(position.getX(), position.getY(), size-1);
			ast->position = position;
			ast->position.impulse(separationX, separationY);
			separationX = -separationX;
			separationY = -separationY;
		}
		
		if(randFloat(0.0, 1.0) < astDrop)
		{
			std::string cmd = retprintf("%s %f %f",
					astDropCmd.getValue().c_str(),
					position.getX(), position.getY());
			serverScripting.command(cmd.c_str());
		}
	}
}

void Asteroid::takeDamage(float amt, int source)
{
	hps -= amt;
	if(hps <= 0)
		die(source);
}

void Asteroid::timepass(void)
{
	Entity::timepass();
	wrap();
}

#include "mastrix.hpp"
#include <cassert>
#include <float.h>

ServerConsoleVar<float> gravConst("grav", 0);
ServerConsoleVar<float> maxSpeed("max_speed", 600.0f);

Entity::Entity()
{
	this->label = "";
	entID	  = server->registerEntity(this);
	drawScale = 1;
	radius	  = 1;
	position  = Position();
	isDead    = false;
//	owner     = this;

	explodeType = explode_none;
	mass=radius;

	team = (team_type)-1;

	visible = true;	//not using show() because the clentity might not be ready!
}

Entity::~Entity()
{
	sendDrop();
}

void Entity::sendDrop(void)
{
	SendMessage drop(message_drop_entity);
		drop.putInt(entID);
	drop.sendToEveryone();
}

void Entity::timepass(void)
{
	if (position.getChanged())
	{
		position.setChanged(false);
		sendUpdate();
	}
	
}

void Entity::die(int killer)
{
	if(explodeType != explode_none)
	{
		SendMessage explodeMsg(message_explode);
			explodeMsg.putFloat(getPosition().getX());
			explodeMsg.putFloat(getPosition().getY());
			explodeMsg.putFloat(getRadius());
			explodeMsg.putInt(getExplodeType());
		explodeMsg.sendToEveryone();
		explodeMsg.sendToMyself();
	}
	
	deleteMe();
}

void Entity::deleteMe()
{ 
	isDead = true; 
}

void Entity::gravity_affect(Entity *object)
{
	float distanceSq;
	float Px,Py,P;
	float theta;
	Vector2D vec1 = position.positionVector(),
	         vec2 = object->getPosition().positionVector(),
	         displacement = vec1-vec2;
	distanceSq = displacement.getMagnitudeSquared();
	
	theta = atan2(displacement.y, displacement.x);
	P = -gravConst*object->getMass()/(distanceSq + 10.0);
	Px = P*cos(theta);
	Py = P*sin(theta);

	position.impulse(Px,Py);
}

void Entity::sendAdd(void) {
	if(!visible) return;
	SendMessage add(message_add_entity);
		msgAddBody(add);
	add.sendToEveryone();
}
void Entity::sendAdd(int destination) {
	if(!visible) return;
	SendMessage add(message_add_entity);
		msgAddBody(add);
	add.sendToClient(destination);
}
void Entity::msgAddBody(SendMessage &add) {
	add.putInt(entID);
	add.putString(sprite.c_str());
	add.putFloat(drawScale);
	position.writeToMessage(add);
	add.putInt(entType);
	add.putInt(team);
	add.putString(label.c_str());
	add.putFloat(radius);
	add.putShort(getDrawEffect());
}


void Entity::sendUpdate(void) {
	if(!visible) return;
	SendMessage update(message_update_entity);
		msgUpdateBody(update);
	update.sendToEveryone();
}
void Entity::sendUpdate(int destination) {
	if(!visible) return;
	SendMessage update(message_update_entity);
		msgUpdateBody(update);
	update.sendToClient(destination);
}
void Entity::msgUpdateBody(SendMessage &update) {
	update.putInt(entID);
	position.writeToMessage(update);
}

void Entity::hide()
{
	if(visible)
	{
		visible = false;
		SendMessage msg(message_drop_entity);
			msg.putInt(entID);
		msg.sendToEveryone();
	}
}

void Entity::show()
{
	if(!visible)
	{
		visible = true;
		sendAdd();
	}
}

float Entity::getSpeedSquared() const
{
	float xVel = position.getX_vel();
	float yVel = position.getY_vel();

	float speedSquared = (xVel * xVel) + (yVel * yVel);

	return speedSquared;
}

float Entity::getMaxSpeed() const
{
	//default entity speed is set by the consolevar max_speed
	return maxSpeed;
}

void Entity::wrap(void)
{
	if(position.getX()+radius < leftBorder)
		position.setX(rightBorder+radius);
	if(position.getX()-radius > rightBorder)
		position.setX(leftBorder-radius);
	if(position.getY()+radius < topBorder)
		position.setY(bottomBorder+radius);
	if(position.getY()-radius > bottomBorder)
		position.setY(topBorder-radius);
}


EnvironmentEntity::EnvironmentEntity(const char *filename, float x, float y, float scale, float rotation, const char *label, int flags)
{
	entType   = ent_large_obstacle;
	sprite    = filename;
	position  = Position(x, y, rotation);
	drawScale = scale;
	this->label = label;
	this->flags = flags;
	
	Image *img = images.getImage(sprite.c_str());
	assert(img);
	radius = (img->getWidth() / 2) * drawScale;
	if (sprite == "") radius = scale;
	
	if(flags & env_rectangular) {
		width = img->getWidth() * drawScale;
		height = img->getHeight() * drawScale;
		radius = sqrt(width*width + height*height);
		entType = ent_rectangular;
	}
	if(flags & env_hidden) {
		visible = false;
	}
	mass = radius*1000;
	sendAdd();
}

DummyEntity::DummyEntity(float x, float y)
{
	entType = ent_invisible;
	sprite = "";
	position = Position(x, y);
	sendAdd();
}

std::string EnvironmentEntity::creationString(void)
{
	return retprintf("addent %f %f \"%s\" %f %f \"%s\"%s%s%s%s%s%s\n",
		position.getX(), position.getY(), sprite.c_str(), drawScale, position.getR(), label.c_str(),
		(flags&env_no_mass)?" nomass":"",
		(flags&env_no_collide)?" nocollide":"",
		(flags&env_no_boundaries)?" nobound":"",
		(flags&env_rectangular)?" rect":"",
		(flags&env_wraps)?" wraps":"",
		(flags&env_hidden)?" hidden":""
		);
}

int EnvironmentEntity::flagFromString(const char *str)
{
	if(!strcmp(str, "nomass")) {
		return env_no_mass;
	} else if(!strcmp(str, "nocollide")) {
		return env_no_collide;
	} else if(!strcmp(str, "nobound")) {
		return env_no_boundaries;
	} else if(!strcmp(str, "rect")) {
		return env_rectangular;
	} else if(!strcmp(str, "wraps")) {
		return env_wraps;
	} else if(!strcmp(str, "hidden")) {
		return env_hidden;
	} else
		return 0;
}

void EnvironmentEntity::timepass(void)
{
	Entity::timepass();
	
	if(flags & env_wraps)
		wrap();
}

SERVER_CONSOLE_COMMAND(addent)
{
	int flags = 0;
	if(argc < 3) {
		printfToClient(who, "Not enough arguments to addent (need at least 3)");
		printfToClient(who, "Arguments: pos_x pos_y imagename [scale] [rotation] [label] [flags]");
		return;
	}
	
	float pos_x = atof(argv[0]),
	      pos_y = atof(argv[1]);
	const char *imagename = argv[2];
	float scale = (argc>=4) ? atof(argv[3]) : 1.0;
	float rotation = (argc>=5) ? atof(argv[4]) : 0.0;
	const char *label = (argc>=6) ? argv[5] : " ";
	
	for(int ii=6; ii<argc; ii++)
	{
		int flag = EnvironmentEntity::flagFromString(argv[ii]);
		if(!flag)
			printfToClient(-1, "Unrecognized flags: \"%s\"", argv[ii]);
		else
			flags |= flag;
	}
	
	EnvironmentEntity *ent = new EnvironmentEntity(imagename, pos_x, pos_y, scale, rotation, label, flags);
	serverScripting.defineVariable("_", retprintf("%i", ent->getEntId()));
}

SERVER_CONSOLE_COMMAND(dummy_ent)
{
	if(argc != 2) {
		printfToClient(who, "Usage: dummy_ent x y");
		return;
	}
	
	float pos_x = atof(argv[0]),
	      pos_y = atof(argv[1]);
	
	DummyEntity *ent = new DummyEntity(pos_x, pos_y);
	serverScripting.defineVariable("_", retprintf("%i", ent->getEntId()));
}

SERVER_CONSOLE_COMMAND(shield_ent)
{
	if (argc !=3) {
		printfToClient(who, "Usage: shield_ent x y size");
		return;
	}

	float pos_x = atof(argv[0]),
		  pos_y = atof(argv[1]),
		  size	= atof(argv[2]);

	EnvironmentEntity *ent = new EnvironmentEntity("", pos_x, pos_y, size, 0.0, "", 0);
	serverScripting.defineVariable("_", retprintf("%i", ent->getEntId()));

	SendMessage msg(message_shield);
		msg.putInt(ent->getEntId());
		msg.putShort(effect_generatorshield);
		msg.putFloat(-600.0);
		msg.putFloat(size);
		msg.sendToEveryone();
}

SERVER_CONSOLE_COMMAND(addbot)
{
	if(argc < 2)
	{
		printfToClient(who, "Wrong number of arguments to addbot (need at least 2)");
		printfToClient(who, "Usage: addbot pos_x, pos_y [type] [team]");
		return;
	}
	ShipType *shiptype = getBotShipType();
	if(argc >= 3)
		shiptype = shipTypeByName(argv[2]);

	team_type teamID = team_enemy;
	if(argc >= 4)
		teamID = (team_type)atoi(argv[3]);

	AIPlayer *bot = 0;
	if (shiptype->isMobile())
	{
		bot = new AIShip(
					atof(argv[0]), 
					atof(argv[1]), 
					shiptype, 
					teamID);
	}

	else
	{
		bot = new AITurret(
					atof(argv[0]), 
					atof(argv[1]), 
					shiptype, 
					teamID);
	}

	serverScripting.defineVariable("_", retprintf("%i", bot->getEntId()));
}

SERVER_CONSOLE_COMMAND(make_relative)
{
	if(argc < 2)
	{
		printfToClient(who, "Usage: make_relative ent1 ent2");
		return;
	}
	int ent1 = atoi(argv[0]),
	    ent2 = atoi(argv[1]);
	server->entities[ent2]->getPosition().makeRelative(ent1);
}

SERVER_CONSOLE_COMMAND(kill)
{
	Player *me = server->getAvatar(who);
	if (me && !me->shouldDelete())
		me->die(-1);
}

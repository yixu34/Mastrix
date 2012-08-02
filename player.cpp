#include "mastrix.hpp"

ServerConsoleVar<float> leftBorder("border_left", -1250);
ServerConsoleVar<float> rightBorder("border_right", 1250);
ServerConsoleVar<float> topBorder("border_top", -1250);
ServerConsoleVar<float> bottomBorder("border_bottom", 1250);

ServerConsoleVar<bool>  allowStrafe("allow_strafe", false);
ServerConsoleVar<bool>  allowMouseAim("allow_mouse_aim", false);

ServerConsoleVar<float> sharedABStrength("ab_shared", 500.0);
ServerConsoleVar<float> strafeABStrength("ab_strafe", 500.0);
ServerConsoleVar<float> thrustABStrength("ab_thrust", 500.0);
ServerConsoleVar<float> brakeABStrength ("ab_brake", 500.0);

ServerConsoleVar<float> sharedABMax ("ab_shared_max",     500.0);
ServerConsoleVar<float> strafeABMax ("ab_strafe_max",     1000.0);
ServerConsoleVar<float> thrustABMax ("ab_thrust_max",     1000.0);
ServerConsoleVar<float> brakeABMax  ("ab_brake_max",      1000.0);
ServerConsoleVar<float> warmupFactor("thrust_warmup",     100);
ServerConsoleVar<float> maxWarmup   ("thrust_warmup_max", 3);
ServerConsoleVar<float> cooldownFactor("thrust_cooldown", 3);

Player::Player(ShipType *type)
{
	weapons.setOwner(this);
	sprite = type->getSprite();

	//entType = ent_player_ship;
	drawScale = type->getDrawScale();
	turningLeft = turningRight = thrusting = braking = shooting = false;
	strafingLeft = strafingRight = false;
	mouseX = mouseY = 0.0;
	
	type->giveDefaultWeapons(this);
	assert(images.getImage(sprite.c_str()) != 0);
	radius = type->getBoundingRadius();

	health = maxHealth = type->getHitpoints();
	engines_set      = 0;
	shieldTime       = 0;
	name			 = "unnamed";
	afterburn_global = sharedABMax;
	afterburn_left   = afterburn_right = strafeABMax;
	afterburn_thrust = thrustABMax;
	afterburn_brake  = brakeABMax;
	warmup = 0;
	
	explodeType = explode_ship;
	this->type = type;

	setTeam(team_free);
}

Player::~Player()
{}

void Player::setName(std::string name)
{
	this->name = name;
}


void Player::takeDamage(float amt, int source)
{
	if(shouldDelete()) return;
	if (!shielding) {
		health -= amt;
		activateShield(0.5, effect_hitshield);
		if(health <= 0) {
			health = 0;
			die(source);
		}
	}
}

void Player::heal(float amt)
{
	health += amt;
	if(health > maxHealth) health = maxHealth;
}

ServerConsoleVar<bool> noPlayerGravity("no_player_gravity", false);


int Player::getCollisionEffect(void)
{
	if(noPlayerGravity)
		return collide_damage | collide_no_gravity;
	else
		return collide_damage;
}

void Player::activateShield(float time, int type) {
	SendMessage msg(message_shield);
		msg.putInt(entID);
		msg.putShort(type);
		msg.putFloat(time);
		msg.putFloat(radius*1.6);
	msg.sendToEveryone();
}

bool Player::canCollideWith(Entity *ent)
{
	if (ent->isPlayer())
	{
		Player *opponent = (Player *)ent;

		if (this->getTeam() == opponent->getTeam())
		{
			if (this->getTeam() == team_free) { return true;}
			return false;
		}
	}
	return true;
}

void Player::processInput()
{
	float r_vel = 0;

	short new_engines_set = 0;
	
	if(allowMouseAim && (abs(mouseX) > 0.1 || abs(mouseY) > 0.1))
	{
		float target_angle = atan2(-mouseY, mouseX);
		float current_angle = position.getR();
		float angle_difference = current_angle-target_angle;
		while(angle_difference < -M_PI) angle_difference += 2*M_PI;
		while(angle_difference > M_PI) angle_difference -= 2*M_PI;
		
		if(abs(angle_difference) < type->getTurnSpeed()*getDt()) {
			turningLeft = turningRight = false;
			position.setR(target_angle);
		} else if(angle_difference > 0) {
			turningLeft = false;
			turningRight = true;
		} else {
			turningLeft = true;
			turningRight = false;
		}
	}
	
	if(!thrusting && !braking && !strafingLeft && !strafingRight) {
		afterburn_global += sharedABStrength * getDt();
	}
	if(!strafingLeft)  afterburn_left   += strafeABStrength * getDt();
	if(!strafingRight) afterburn_right  += strafeABStrength * getDt();
	if(!thrusting)     afterburn_thrust += thrustABStrength * getDt();
	if(!braking)       afterburn_brake  += brakeABStrength  * getDt();
	
	if(afterburn_global > sharedABMax) afterburn_global = sharedABMax;
	if(afterburn_left   > strafeABMax) afterburn_left   = strafeABMax;
	if(afterburn_right  > strafeABMax) afterburn_right  = strafeABMax;
	if(afterburn_thrust > thrustABMax) afterburn_thrust = thrustABMax;
	if(afterburn_brake  > brakeABMax) afterburn_brake  = brakeABMax;
	
	if(turningLeft)
	{
		r_vel += type->getTurnSpeed();
		position.setChanged(true);
		//new_engines_set += 0x10;
	}
	if(turningRight)
	{	
		r_vel -= type->getTurnSpeed();
		position.setChanged(true);
		//new_engines_set += 0x20;
	}
	if(!thrusting || !braking)	
	{
		if(thrusting)
		{
			warmup += getDt();
			if(warmup > maxWarmup) warmup = maxWarmup;

			//disable thrusting afterburners for bot ship types
			float strength;
			if (type->hasAfterburners())
				strength = afterburn_global + afterburn_thrust +
				           type->getThrustStrength()*getDt() + warmup*warmupFactor*getDt();
			else
				strength = type->getThrustStrength()*getDt();

			afterburn_global = afterburn_thrust = 0;
			
			position.impulse((strength * cos(position.getR())), 
							 (strength *-sin(position.getR())));
			new_engines_set |= 0x1;
		} else {
			warmup -= getDt()*cooldownFactor;
			if(warmup<0) warmup = 0;
		}
		
		if(braking) {
			//disable thrusting afterburners for bot ship types
			float strength;
			if (type->hasAfterburners())
				strength = afterburn_global + afterburn_brake + type->getBrakeStrength()*getDt();
			else
				strength = type->getBrakeStrength();

			afterburn_global = afterburn_brake = 0;
			position.impulse(
				-strength *  cos(position.getR()),
				-strength * -sin(position.getR()));
			new_engines_set |= 0x2;
		}
	}
	
	if(strafingLeft)
	{
		float strength = afterburn_global + afterburn_left + type->getStrafeStrength()*getDt();
		afterburn_global = afterburn_left = 0;
		position.impulse((strength * cos(position.getR()+M_PI/2)),
		                 (strength *-sin(position.getR()+M_PI/2)));
		new_engines_set |= 0x4;
	}
	if(strafingRight)
	{
		float strength = afterburn_global + afterburn_right + type->getStrafeStrength()*getDt();
		afterburn_global = afterburn_right = 0;
		position.impulse((strength * cos(position.getR()-M_PI/2)),
		                 (strength *-sin(position.getR()-M_PI/2)));
		new_engines_set |= 0x8;
	}

	if (new_engines_set != engines_set) {
		SendMessage msg(message_engine);
			msg.putInt(entID);
			msg.putFloat(radius);
			msg.putShort(new_engines_set);
		msg.sendToEveryone();
	}
	engines_set = new_engines_set;

    if (weapons.primary())
	    weapons.primary()->timepass();
	if(weapons.secondary())
		weapons.secondary()->timepass();
	weapons.checkDepletion();

	//process rotation
	if(r_vel != position.getR_vel() || r_vel != 0)
		position.setR_vel( r_vel );
}

/*
bool Player::inBoundsY(void)
{
	if ((position.getY() < topBorder + 5) && (sin(position.getR()) > 0) 
		|| ((position.getY() > bottomBorder - 5) && (sin(position.getR()) < 0)))
		return false;
	else return true;
}

bool Player::inBoundsX(void)
{
	if ((position.getX()>rightBorder - 5) && (cos(position.getR())>0) 
		|| ((position.getX() < leftBorder +5) && (cos(position.getR()) < 0)))
		return false;
	else return true;
}*/

void Player::timepass(void)
{
	if (!gameFrozen)
		processInput();
}

void Player::die(int killer)
{
	if(isHuman())
		server->playSoundAt("sounds/Explosion4.wav", position);
	else
		server->playSoundAt("sounds/Explosion3.wav", position);
	Entity::die(killer);
}

void Player::deleteMe()
{
	Entity::deleteMe();
}

void Player::trackMouse(float x, float y)
{
	mouseX = x;
	mouseY = y;
}

SERVER_CONSOLE_COMMAND(showvel)
{
	if(!server->getAvatar(who)) return;
	Position pos = server->getAvatar(who)->getPosition();
	printfToClient(who, "Vel is (%f,%f), pos is (%f,%f)",
		pos.getX_vel(), pos.getY_vel(),
		pos.getX(), pos.getY());
}

SERVER_CONSOLE_COMMAND(mousepos)
{
	if(argc != 2) {
		printfToClient(who, "Usage: mousepos x y");
		return;
	}
	if(!server->getAvatar(who))
		return;
	server->getAvatar(who)->trackMouse( atof(argv[0]), atof(argv[1]) );
}


SERVER_INVERTIBLE_CONSOLE_COMMAND(left)   { if(server->getAvatar(who)) server->getAvatar(who)->turnLeft(true); }
SERVER_INVERSE_CONSOLE_COMMAND   (left)   { if(server->getAvatar(who)) server->getAvatar(who)->turnLeft(false); }
SERVER_INVERTIBLE_CONSOLE_COMMAND(right)  { if(server->getAvatar(who)) server->getAvatar(who)->turnRight(true); }
SERVER_INVERSE_CONSOLE_COMMAND   (right)  { if(server->getAvatar(who)) server->getAvatar(who)->turnRight(false); }

SERVER_INVERTIBLE_CONSOLE_COMMAND(strafe_left)   { if(server->getAvatar(who)) server->getAvatar(who)->strafeLeft(true); }
SERVER_INVERSE_CONSOLE_COMMAND   (strafe_left)   { if(server->getAvatar(who)) server->getAvatar(who)->strafeLeft(false); }
SERVER_INVERTIBLE_CONSOLE_COMMAND(strafe_right)  { if(server->getAvatar(who)) server->getAvatar(who)->strafeRight(true); }
SERVER_INVERSE_CONSOLE_COMMAND   (strafe_right)  { if(server->getAvatar(who)) server->getAvatar(who)->strafeRight(false); }

SERVER_INVERTIBLE_CONSOLE_COMMAND(thrust) { if(server->getAvatar(who)) server->getAvatar(who)->thrust(true);  }
SERVER_INVERSE_CONSOLE_COMMAND   (thrust) { if(server->getAvatar(who)) server->getAvatar(who)->thrust(false); }
SERVER_INVERTIBLE_CONSOLE_COMMAND(brake)  { if(server->getAvatar(who)) server->getAvatar(who)->brake(true);   }
SERVER_INVERSE_CONSOLE_COMMAND   (brake)  { if(server->getAvatar(who)) server->getAvatar(who)->brake(false);  }
SERVER_INVERTIBLE_CONSOLE_COMMAND(fire1)  { if(server->getAvatar(who)) server->getAvatar(who)->fire1(true);   }
SERVER_INVERSE_CONSOLE_COMMAND   (fire1)  { if(server->getAvatar(who)) server->getAvatar(who)->fire1(false);  }
SERVER_INVERTIBLE_CONSOLE_COMMAND(fire2)  { if(server->getAvatar(who)) server->getAvatar(who)->fire2(true);   }
SERVER_INVERSE_CONSOLE_COMMAND   (fire2)  { if(server->getAvatar(who)) server->getAvatar(who)->fire2(false);  }
void Player::turnLeft (bool active)   { turningLeft   = active; }
void Player::turnRight(bool active)   { turningRight  = active; }
void Player::strafeLeft (bool active) { strafingLeft  = active; }
void Player::strafeRight(bool active) { strafingRight = active; }
void Player::thrust(bool active)      { thrusting     = active; }
void Player::brake(bool active)       { braking       = active; }

SERVER_CONSOLE_COMMAND(next_weapon) { if(server->getAvatar(who)) server->getAvatar(who)->weapons.selectNext(); }
SERVER_CONSOLE_COMMAND(previous_weapon) { if(server->getAvatar(who)) server->getAvatar(who)->weapons.selectPrevious(); }

void Player::fire1(bool active) {
	if(active)
		weapons.primary()->startShooting();
	else
		weapons.primary()->stopShooting();
}
void Player::fire2(bool active) {
	if(active) {
		if(weapons.secondary()) weapons.secondary()->startShooting();
	} else {
		if(weapons.secondary()) weapons.secondary()->stopShooting();
	}
}

SERVER_CONSOLE_COMMAND(setpos)
{
	if(!server->getAvatar(who)) return;
	if(argc != 2) {
		printfToClient(who, "Usage: setpos x y");
		return;
	}
	Position pos(atof(argv[0]), atof(argv[1]));
	server->getAvatar(who)->setPosition(pos);
}

#include "mastrix.hpp"

class Powerup :public Entity
{
public:
	Powerup(Position pos);
	virtual int getCollisionEffect(void) { return collide_custom | collide_no_gravity; }
	virtual void collideWith(Entity *ent);
	virtual void powerup(HumanPlayer *plr) = 0;
	virtual bool canGetPowerup(HumanPlayer *plr) { return true; }
	virtual std::string powerupName(void) = 0;
	std::string creationString(void);
};

struct HealthPowerup :public Powerup {
	HealthPowerup(Position pos);
	void powerup(HumanPlayer *plr);
	std::string powerupName(void) { return "heal"; }
};
struct LaserPowerup :public Powerup {
	LaserPowerup(Position pos);
	void powerup(HumanPlayer *plr);
	std::string powerupName(void) { return "laser"; }
};
struct LaserLevelPowerup : public Powerup {
	LaserLevelPowerup(Position pos);
	void powerup(HumanPlayer *plr);
	std::string powerupName(void) { return "laserlevel"; }
};
struct LaserNumPowerup : public Powerup {
	LaserNumPowerup(Position pos);
	void powerup(HumanPlayer *plr);
	std::string powerupName(void) { return "lasernum"; }
};
struct ShieldPowerup : public Powerup {
	ShieldPowerup(Position pos);
	void powerup(HumanPlayer *plr);
	std::string powerupName(void) { return "shield"; }
};
struct MissilePowerup : public Powerup {
	MissilePowerup(Position pos);
	void powerup(HumanPlayer *plr);
	std::string powerupName(void) { return "missiles"; }
};
struct HomingMissilePowerup : public Powerup {
	HomingMissilePowerup(Position pos);
	void powerup(HumanPlayer *plr);
	std::string powerupName(void) { return "homing"; }
};
struct FireballPowerup : public Powerup {
	FireballPowerup(Position pos);
	void powerup(HumanPlayer *plr);
	std::string powerupName(void) { return "fireballs"; }
};
struct GoldPowerup : public Powerup {
	GoldPowerup(Position pos);
	void powerup(HumanPlayer *plr);
	std::string powerupName(void) { return "gold"; }
};
struct SilverPowerup : public Powerup {
	SilverPowerup(Position pos);
	void powerup(HumanPlayer *plr);
	std::string powerupName(void) { return "silver"; }
};
struct FlagPowerup : public Powerup {
	FlagPowerup(Position pos, team_type team, bool inBase);
	void powerup(HumanPlayer *plr);
	std::string powerupName(void) { return "flag"; }
	bool canGetPowerup(HumanPlayer *plr);
	
	team_type owner;
	bool isInBase;
};

Powerup::Powerup(Position pos)
{
	entType   = ent_powerup;
	drawScale = 0.5;
	radius = 40;
	this->position = pos;
}

void Powerup::collideWith(Entity *ent)
{
	if(ent->isHuman() && canGetPowerup((HumanPlayer*)ent)) {
		powerup((HumanPlayer*)ent);
		deleteMe();
	}
}

std::string Powerup::creationString(void)
{
	return retprintf("powerup %f %f %s\n", position.getX(), position.getY(), powerupName().c_str());
}

////////////////////////////////////////////////////////////////////////////
LaserPowerup::LaserPowerup(Position pos) :Powerup(pos)
{
	sprite = "images/powerup-laser.png";
	sendAdd();
}
void LaserPowerup::powerup(HumanPlayer *plr)
{
	messageToClient(plr->getClientId(), "Lasers upgraded!");
	plr->weapons.upgradeLaser();
}
////////////////////////////////////////////////////////////////////////////
LaserLevelPowerup::LaserLevelPowerup(Position pos) :Powerup(pos)
{
	sprite = "images/powerup-lightning.png";
	sendAdd();
}
void LaserLevelPowerup::powerup(HumanPlayer *plr)
{
	messageToClient(plr->getClientId(), "Laser level upgrade!");
	plr->weapons.upgradeLaserLevel();
}
////////////////////////////////////////////////////////////////////////////
LaserNumPowerup::LaserNumPowerup(Position pos) :Powerup(pos)
{
	sprite = "images/powerup-trilaser.png";
	sendAdd();
}
void LaserNumPowerup::powerup(HumanPlayer *plr)
{
	messageToClient(plr->getClientId(), "Laser spread upgrade!");
	plr->weapons.upgradeLaserNum();
}
////////////////////////////////////////////////////////////////////////////
HealthPowerup::HealthPowerup(Position pos) :Powerup(pos)
{
	sprite = "images/powerup-health.png";
	sendAdd();
}
void HealthPowerup::powerup(HumanPlayer *plr)
{
	messageToClient(plr->getClientId(), "All damage repaired!");
	plr->heal(100);
	server->playSoundAt("sounds/burst2.wav", position);
}
////////////////////////////////////////////////////////////////////////////
ShieldPowerup::ShieldPowerup(Position pos) :Powerup(pos)
{
	sprite = "images/powerup-shield.png";
	sendAdd();
}
void ShieldPowerup::powerup(HumanPlayer *plr)
{
	messageToClient(plr->getClientId(), "Shields up!");
	plr->shieldsUp(10);
}
////////////////////////////////////////////////////////////////////////////
MissilePowerup::MissilePowerup(Position pos) :Powerup(pos)
{
	sprite = "images/powerup-missile.png";
	sendAdd();
}
void MissilePowerup::powerup(HumanPlayer *plr)
{
	messageToClient(plr->getClientId(), "Missiles!");
	if( plr->weapons.giveAmmo(ammo_missile, 25) )
		return;
	plr->weapons.addWeapon( new MissileLauncher() );
	plr->weapons.giveAmmo(ammo_missile, 25);
}
////////////////////////////////////////////////////////////////////////////
HomingMissilePowerup::HomingMissilePowerup(Position pos) :Powerup(pos)
{
	sprite = "images/powerup-homingmissile.png";
	sendAdd();
}
void HomingMissilePowerup::powerup(HumanPlayer *plr)
{
	messageToClient(plr->getClientId(), "Homing Missiles!");
	if( plr->weapons.giveAmmo(ammo_homing_missile, 10) )
		return;
	plr->weapons.addWeapon( new HomingMissileLauncher() );
	plr->weapons.giveAmmo(ammo_homing_missile, 10);
}
////////////////////////////////////////////////////////////////////////////
FireballPowerup::FireballPowerup(Position pos) :Powerup(pos)
{
	sprite = "images/powerup-fireball.png";
	sendAdd();
}
void FireballPowerup::powerup(HumanPlayer *plr)
{
	messageToClient(plr->getClientId(), "NUCLEAR LAUNCHERS DETECTED");
	if( plr->weapons.giveAmmo(ammo_fireball, 5) )
		return;
	plr->weapons.addWeapon( new FireballLauncher() );
	plr->weapons.giveAmmo(ammo_fireball, 5);
}
////////////////////////////////////////////////////////////////////////////
GoldPowerup::GoldPowerup(Position pos) :Powerup(pos)
{
	sprite = "images/resource-gold.png";
	sendAdd();
}
void GoldPowerup::powerup(HumanPlayer *plr)
{
	server->playSoundAt("sounds/cash.wav", position);
	std::string moneyStr = serverScripting.lookupVariable("money");
	int money = atoi(moneyStr.c_str());
	serverScripting.defineVariable("money", retprintf("%i", money+3));
	serverScripting.defineVariable("got_gold", "1");
}
////////////////////////////////////////////////////////////////////////////
SilverPowerup::SilverPowerup(Position pos) :Powerup(pos)
{
	sprite = "images/resource-silver.png";
	sendAdd();
}
void SilverPowerup::powerup(HumanPlayer *plr)
{
	server->playSoundAt("sounds/cash.wav", position);
	std::string moneyStr = serverScripting.lookupVariable("money");
	int money = atoi(moneyStr.c_str());
	serverScripting.defineVariable("money", retprintf("%i", money+1));
	serverScripting.defineVariable("got_silver", "1");
}
////////////////////////////////////////////////////////////////////////////
FlagPowerup::FlagPowerup(Position pos, team_type team, bool base) :Powerup(pos)
{
	sprite = "images/resource-gold.png";
	entType = ent_flag;
	sendAdd();
	owner = team;
	isInBase = base;
}
void FlagPowerup::powerup(HumanPlayer *plr)
{
	server->playSoundAt("sounds/cash.wav", position);
	if(plr->getTeam() == owner) {
		messageToClient(-1, "%s recovered his team's money.", plr->getName().c_str());
		server->giveFlagTo(owner);
	} else {
		if(plr->getTeam() == team_red)
			messageToClient(-1, "Red (%s) has taken a pile of cash!", plr->getName().c_str());
		else
			messageToClient(-1, "Blue (%s) has taken a pile of cash!", plr->getName().c_str());
		plr->acquireFlag();
	}
}

bool FlagPowerup::canGetPowerup(HumanPlayer *plr)
{
	if(plr->hasFlag())
		return false;
	if(plr->getTeam() == owner && isInBase)
		return false;
	return true;
}

////////////////////////////////////////////////////////////////////////////

Powerup *powerupByType(const char *type, Position pos)
{
	if(!strcasecmp(type, "heal"))
		return new HealthPowerup(pos);
	else if(!strcasecmp(type, "laser"))
		return new LaserPowerup(pos);
	else if(!strcasecmp(type, "laserlevel"))
		return new LaserLevelPowerup(pos);
	else if(!strcasecmp(type, "lasernum"))
		return new LaserNumPowerup(pos);
	else if(!strcasecmp(type, "shield"))
		return new ShieldPowerup(pos);
	else if(!strcasecmp(type, "missiles"))
		return new MissilePowerup(pos);
	else if(!strcasecmp(type, "homing"))
		return new HomingMissilePowerup(pos);
	else if(!strcasecmp(type, "fireballs"))
		return new FireballPowerup(pos);
	else if(!strcasecmp(type, "gold"))
		return new GoldPowerup(pos);
	else if(!strcasecmp(type, "silver"))
		return new SilverPowerup(pos);
	// This is a total cheat... this is so bad...
	else if(!strcasecmp(type, "ultra"))
	{
		powerupByType("lasernum", pos);
		powerupByType("laserlevel", pos);
		powerupByType("shield", pos);
		powerupByType("missiles", pos);
		powerupByType("missiles", pos);
		powerupByType("homing", pos);
		powerupByType("homing", pos);
		powerupByType("fireballs", pos);
		powerupByType("fireballs", pos);
		return new HealthPowerup(pos);
	}
	else if(!strcasecmp(type, "random"))
	{
		int sel = randInt(0, 99);
		if(sel < 20) return new HealthPowerup(pos);         sel -= 20;
		if(sel < 30) return new MissilePowerup(pos);        sel -= 30;
		if(sel < 20) return new HomingMissilePowerup(pos);  sel -= 20;
		if(sel < 20) return new ShieldPowerup(pos);         sel -= 20;
		if(sel < 10) return new FireballPowerup(pos);       sel -= 10;
		return new LaserPowerup(pos);
	}
	else
		return NULL;
}

void placeFlagAt(float x, float y, team_type team, bool base)
{
	new FlagPowerup(Position(x,y), team, base);
}

SERVER_CONSOLE_COMMAND(powerup)
{
	if(argc < 3)
	{
		printfToClient(who, "Usage: powerup x y type");
		return;
	}
	Powerup *p = powerupByType(argv[2], Position(atof(argv[0]), atof(argv[1])));
	if(!p) {
		printfToClient(who, "%s is not a valid powerup type.", argv[2]);
		return;
	}
}

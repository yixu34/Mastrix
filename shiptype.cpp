#include "mastrix.hpp"

void updatePlayerNumLasers(int numLasers);
void updatePlayerLaserLevel(int laserLevel);
void updatePlayerNumMissiles(int numMissiles);

ServerConsoleVar<float> turnSpeed("turn_speed", 9);
ServerConsoleVar<float> thrustStrength("thrust_strength", 120.0);
ServerConsoleVar<float> brakeStrength("brake_strength", 120.0);
ServerConsoleVar<float> strafeStrength("strafe_strength", 120.0);
ServerConsoleVar<float> hitPoints("hitpoints", 15);
ServerConsoleVar<float> maxBotSpeed("max_bot_speed", 400.0f);
ServerConsoleVar<float> maxPlayerSpeed("player_speed", 600.0f);

ServerConsoleVar<float> shotDelay("shot_delay", 0.2);
ServerConsoleVar<float> shotSpeed("shotspeed", 1200);
ServerConsoleVar<float> botShotSpeed("shotspeed", 650);

ServerConsoleVar<int> startLaserNum("laser_num", 1, updatePlayerNumLasers);
ServerConsoleVar<int> startLaserLevel("laser_level", 1, updatePlayerLaserLevel);
ServerConsoleVar<int> startMissiles("start_missiles", 0, updatePlayerNumMissiles);

//These functions update the player ship type to match the starting
//weapon ammo quantities

void updatePlayerStartingWeapons(
	int numLasers, 
	int laserLevel, 
	int numMissiles);

//updates the player's number of starting laser shots whenever the cvar
//startLaserNum (laser_num) changes
void updatePlayerNumLasers(int numLasers)
{
	updatePlayerStartingWeapons(
		numLasers, 
		startLaserLevel, 
		startMissiles);	
}

void updatePlayerLaserLevel(int laserLevel)
{
	updatePlayerStartingWeapons(
		startLaserNum, 
		laserLevel, 
		startMissiles);	
}

void updatePlayerNumMissiles(int numMissiles)
{
	updatePlayerStartingWeapons(
		startLaserNum, 
		startLaserLevel, 
		numMissiles);	
}

void updatePlayerStartingWeapons(
	int numLasers, 
	int laserLevel, 
	int numMissiles)
{
	if (server->shipTypes.find("player") == server->shipTypes.end())
	{
		printfToClient(-1, "Cannot update player starting laser number - \
						   player ship type has not be defined.");
		return;
	}

	ShipType *playerShipType = server->shipTypes["player"];
	
	//create a new weapon info structure based on the cvars
	ShipType::WeaponAmmoInfo oldPrimaryInfo;
	oldPrimaryInfo = playerShipType->getPrimaryWeaponInfo();
	oldPrimaryInfo.weapon = new RedPeaShooter(
									numLasers, 
									laserLevel, 
									shotDelay, 
									shotSpeed);	

	playerShipType->setPrimaryWeapon(oldPrimaryInfo);

	//now find the player's missile launcher and update the default ammo
	ShipType::SecondaryWeaponPool &secondaryInfo = playerShipType->getSecondaryWeapons();
	ShipType::SecondaryWeaponPool::iterator ii;
	for (ii = secondaryInfo.begin(); ii != secondaryInfo.end(); ++ii)
	{
		if (ii->ammoType == ammo_missile)
			ii->ammoAmount = numMissiles;
	}

}

ShipType::ShipType(
	const std::string &sprite,
	float drawScale,
	float boundingRadius,
	float turnSpeed,
	float thrustStrength,
	float brakeStrength,
	float strafeStrength,
	float maxSpeed,
	float hitpoints,
	bool onHumanTeam,
	bool mobile,
	bool weapons,
	bool afterBurners, 
	bool playerShipType)
{
	this->sprite         = sprite;
	this->drawScale      = drawScale;
	this->boundingRadius = boundingRadius;
	this->turnSpeed      = turnSpeed;
	this->thrustStrength = thrustStrength;
	this->brakeStrength  = brakeStrength;
	this->strafeStrength = strafeStrength;
	this->maxSpeed       = maxSpeed;
	this->hitpoints      = hitpoints;
	this->onHumanTeam    = onHumanTeam;
	this->mobile         = mobile;
	this->weapons        = weapons;
	this->afterBurners   = afterBurners;
	this->playerShipType = playerShipType;
	
	//This gets set later when reading in the weapon data
	usesBossMounts = false;

    //clear out the weapon info structure
    primaryWeapon = WeaponAmmoInfo();
}

void ShipType::setPrimaryWeapon(const WeaponAmmoInfo &primaryInfo)
{
	//clear out the old weapon if adding a new one
	if (primaryWeapon.weapon != 0)
		delete primaryWeapon.weapon;

	primaryWeapon.weapon     = primaryInfo.weapon;
    primaryWeapon.ammoType   = primaryInfo.ammoType;
    primaryWeapon.ammoAmount = primaryInfo.ammoAmount;
}

void ShipType::addSecondaryWeapon(const WeaponAmmoInfo &secondaryInfo)
{
    secondaryWeapons.push_back(secondaryInfo);
}

//copy the weapons associated with this ship type to the player's weapon pool
void ShipType::giveDefaultWeapons(Player *plr)
{
    assert(plr != 0);

    if (primaryWeapon.weapon != 0)
    {
   	    plr->weapons.addPrimary(primaryWeapon.weapon->clone());
        plr->weapons.giveAmmo(
                        primaryWeapon.ammoType, 
                        primaryWeapon.ammoAmount);
    }

    SecondaryWeaponPool::iterator ii;
    for (ii = secondaryWeapons.begin(); ii != secondaryWeapons.end(); ++ii)
    {
        WeaponAmmoInfo weaponInfo = (*ii);

        if (weaponInfo.weapon == 0)
            continue;

        plr->weapons.addWeapon(weaponInfo.weapon->clone());
        plr->weapons.giveAmmo(
                          weaponInfo.ammoType, 
                          weaponInfo.ammoAmount);
    }
}

const Position* ShipType::getWeaponMounts(unsigned numMounts)
{
	if (usesBossMounts)
		return getBossMounts(numMounts);
	else
		return getDefaultMounts(numMounts);
}

const Position *ShipType::getDefaultMounts(unsigned numMounts)
{
	static const Position shots1[] = { Position(40, 0) };
	static const Position shots2[] = { Position(40, 15), Position(40, -15) };
	static const Position shots3[] = { Position(0, 20), Position(0, -20), Position(40, 0) };
	static Position *shotsVar = NULL;
	unsigned shotsVarSize = 0;
	
	switch(numMounts) {
		case 0: return shots1;
		case 1: return shots1;
		case 2: return shots2;
		case 3: return shots3;
		
			// Handle weird cases like a ship asking for 50 weapon mounts by just
			// returning an array of many copies of one.
		default:
			if(numMounts > shotsVarSize) {
				delete[] shotsVar;
				shotsVar = new Position[numMounts];
				shotsVarSize = numMounts;
				for(unsigned ii=0; ii<numMounts; ii++)
					shotsVar[ii] = Position(40, 0);
			}
			return shotsVar;
	}
}

const Position *ShipType::getBossMounts(unsigned numMounts)
{
	if(numMounts != 4) 
		return ShipType::getDefaultMounts(numMounts);
	
	static Position mounts[] = { 
						Position(40, 20), 
						Position(40, -20), 
						Position(10, 80), 
						Position(10, -80) };
	return mounts;
}

//
//class PlayerShipType
//	:public ShipType
//{
//public:
//	const char *getSprite(void)   { return "images/ship.png"; }
//	float getDrawScale(void)      { return 0.6;               }
//	float getBoundingRadius(void) { return 22;                }
//	float getTurnSpeed(void)      { return turnSpeed;         }
//	float getThrustStrength(void) { return thrustStrength;    }
//	float getBrakeStrength(void)  { return brakeStrength;     }
//	float getStrafeStrength(void) { return strafeStrength;    }
//	bool  isOnHumanTeam(void)     { return true;              }
//	float getHitpoints(void)      { return hitPoints;         }
//	bool  isMobile(void)          { return true;              }
//	bool  hasAfterburners(void)   { return true;              }
//	float getMaxSpeed(void)       { return maxPlayerSpeed;    }
//	void  giveDefaultWeapons(Player *plr) {
//		plr->weapons.addPrimary(new RedPeaShooter(startLaserNum, startLaserLevel, shotDelay, shotSpeed));
//		if(startMissiles > 0) {
//			plr->weapons.addWeapon(new MissileLauncher());
//			plr->weapons.giveAmmo(ammo_missile, startMissiles);
//		}
//	}
//};
//
//class BotShipType
//	:public ShipType
//{
//public:
//	const char *getSprite(void)     { return "images/player1.png"; }
//	float getDrawScale(void)        { return 1.0;               }
//	float getBoundingRadius(void)   { return 22;                }
//	float getTurnSpeed(void)        { return turnSpeed;         }
//	float getThrustStrength(void)   { return thrustStrength;    }
//	float getBrakeStrength(void)    { return brakeStrength;     }
//	float getStrafeStrength(void)   { return strafeStrength;    }
//	virtual float getMaxSpeed(void) { return maxBotSpeed;       }
//	bool  isOnHumanTeam(void)       { return false;             }
//	float getHitpoints(void)        { return 5;                 }
//	bool  isMobile(void)            { return true;              }
//	void  giveDefaultWeapons(Player *plr) {
//		plr->weapons.addPrimary(new RedPeaShooter(1, 1, shotDelay*2, botShotSpeed));
//	}
//};
//
//class HelplessBotShipType : public ShipType
//{
//public:
//	const char *getSprite(void)     { return "images/player1.png"; }
//	float getDrawScale(void)        { return 1.0;               }
//	float getBoundingRadius(void)   { return 22;                }
//	float getTurnSpeed(void)        { return turnSpeed;         }
//	float getThrustStrength(void)   { return thrustStrength;    }
//	float getBrakeStrength(void)    { return brakeStrength;     }
//	float getStrafeStrength(void)   { return strafeStrength;    }
//	virtual float getMaxSpeed(void) { return maxBotSpeed;       }
//	bool  isOnHumanTeam(void)       { return false;             }
//	float getHitpoints(void)        { return 5;                 }
//	bool  isMobile(void)            { return true;              }
//	bool hasWeapons()				{ return false;				}
//};
//
//class SlowBotShipType
//	:public BotShipType
//{
//	float getMaxSpeed(void) { return 200;       }
//};
//
//class BossShipType
//	:public ShipType
//{
//public:
//	const char *getSprite(void)   { return "images/ship2.png";   }
//	float getDrawScale(void)      { return 0.6f;                 }
//	float getBoundingRadius(void) { return 90;                   }
//	float getTurnSpeed(void)      { return turnSpeed;            }
//	float getThrustStrength(void) { return thrustStrength;       }
//	float getBrakeStrength(void)  { return brakeStrength;        }
//	float getStrafeStrength(void) { return strafeStrength;       }
//	float getMaxSpeed(void)       { return maxBotSpeed;          }
//	bool  isOnHumanTeam(void)     { return false;                }
//	float getHitpoints(void)      { return 50;                   }
//	bool  isMobile(void)          { return true;                 }	
//	const Position* getWeaponMounts(unsigned num) {
//		if(num != 4) return ShipType::getWeaponMounts(num);
//		static Position mounts[] = { Position(40, 20), Position(40, -20), Position(10, 80), Position(10, -80) };
//		return mounts;
//	}
//	void  giveDefaultWeapons(Player *plr) {
//		plr->weapons.addPrimary(new RedPeaShooter(4, 1, shotDelay*2, botShotSpeed));
//	}
//};
//
//class TurretShipType
//	:public ShipType
//{
//public:
//	const char *getSprite()       { return "images/station.png"; }
//	float getDrawScale()          { return 0.2f;                 }
//	float getBoundingRadius()     { return 20;                   }
//	float getTurnSpeed()          { return 3;                    }
//	float getThrustStrength()     { return 0;                    }
//	float getBrakeStrength()      { return 0;                    }
//	float getStrafeStrength()     { return 0;                    }
//	float getMaxSpeed(void)       { return 0;                    }
//	bool  isOnHumanTeam()         { return false;                }
//	float getHitpoints()          { return 3;                    }
//	bool  isMobile(void)          { return false;                }
//	virtual void  giveDefaultWeapons(Player *plr) {
//		plr->weapons.addPrimary(new RedPeaShooter(1, 2, 1.0, botShotSpeed));
//	}
//};
//
//class MissileTurretShipType
//	:public TurretShipType
//{
//public:
//	float getHitpoints()          { return 5; }
//	
//	void  giveDefaultWeapons(Player *plr) {
//		plr->weapons.addPrimary(new SlowMissileLauncher());
//		plr->weapons.giveAmmo(ammo_missile, 100000);
//	}
//};
//
//class SuperTurretShipType
//	:public MissileTurretShipType
//{
//public:
//	float getTurnSpeed()          { return 6;  }
//	float getHitpoints()          { return 50; }	
//};
//
//class TankerShipType
//	:public ShipType
//{
//public:
//	const char *getSprite(void)   { return "images/tanker.png"; }
//	float getDrawScale(void)      { return 0.5;               }
//	float getBoundingRadius(void) { return 50;                }
//	float getTurnSpeed(void)      { return 1;                 }
//	float getThrustStrength(void) { return 60;                }
//	float getBrakeStrength(void)  { return 60;                }
//	float getStrafeStrength(void) { return 0;                 }
//	float getMaxSpeed(void)       { return 400;               }
//	bool  isOnHumanTeam(void)     { return true;              }
//	bool  hasWeapons(void)        { return false;             }
//	float getHitpoints(void)      { return 35;                }
//	bool  isMobile(void)          { return true;              }
//};
//
//static PlayerShipType playerShipType;
//static TankerShipType tankerShipType;
//static BotShipType botShipType;
//static SlowBotShipType slowBotShipType;
//static TurretShipType turretShipType;
//static MissileTurretShipType missileTurretShipType;
//static BossShipType bossShipType;
//static SuperTurretShipType superTurretShipType;
//static HelplessBotShipType helplessBotShipType;

ShipType *getPlayerShipType(void) 
{ 
	return server->shipTypes["player"];
	//return &playerShipType; 
}
ShipType *getBotShipType(void) 
{
	return server->shipTypes["bot"];
	//return &botShipType; 
}


ShipType *shipTypeByName(const std::string &name)
{
	if (server->shipTypes.find(name) == server->shipTypes.end())
	{
		printfToClient(-1, "Ship type not found!");
		return 0;
	}

	return server->shipTypes[name];

	/*if(name=="player")
		return &playerShipType;
	else if(name=="bot")
		return &botShipType;
	else if(name=="slowbot")
		return &slowBotShipType;
	else if(name=="tanker")
		return &tankerShipType;
	else if (name == "turret")
		return &turretShipType;
	else if (name == "missileturret")
		return &missileTurretShipType;
	else if (name == "superturret")
		return &superTurretShipType;
	else if (name == "boss")
		return &bossShipType;
	else if (name == "helpless")
		return &helplessBotShipType;
	else
		return &playerShipType;*/
}


SERVER_CONSOLE_COMMAND(add_ship_type)
{
    if (argc < 14)
    {
        printfToClient(who, "Usage:  add_ship_type name sprite drawScale "
                            "boundingRadius turnSpeed thrustStrength "
                            "brakeStrength strafeStrength maxSpeed "
                            "hitpoints isOnHumanTeam isMobile hasWeapons "
                            "hasAfterburners [player]");
        return;							 
    }

    std::string name = argv[0];
    if (server->isShipTypeDeclared(name))
    {
        /*std::string duplicatedName = name;
        duplicatedName += " ship type already exists!";
        printfToClient(who, duplicatedName.c_str());*/
        return;
    }

	bool isPlayerShipType = false;
	if (argc >= 15 && !strcasecmp(argv[14], "player"))
		isPlayerShipType = true;

    ShipType *shipType = new ShipType(
        argv[1],		//sprite
        atof(argv[2]),  //drawScale
        atof(argv[3]),	//boundingRadius
        atof(argv[4]),  //turnSpeed
        atof(argv[5]),  //thrustStrength
        atof(argv[6]),  //brakeStrength
        atof(argv[7]),  //strafeStrength
        atof(argv[8]),  //maxSpeed
        atof(argv[9]),  //hitpoints
        atoi(argv[10]), //isOnHumanTeam
        atoi(argv[11]), //isMobile
        atoi(argv[12]), //hasWeapons
        atoi(argv[13]), //hasAfterburners
		isPlayerShipType); 

    server->shipTypes[name] = shipType;
}

SERVER_CONSOLE_COMMAND(remove_ship_type)
{
	if (argc < 1)
	{
		printfToClient(who, "Not enough arguments to remove_ship_type (should be 1 )");
		printfToClient(who, "Usage:  remove_ship_type type");
		return;
	}
	std::string shipTypeName = argv[0];

	Server::ShipTypePool::iterator ii = server->shipTypes.find(shipTypeName);
	if (ii == server->shipTypes.end())
	{
		printfToClient(who, "Ship type not found!");
		return;
	}

	delete ii->second;
	server->shipTypes.erase(ii);
}

SERVER_CONSOLE_COMMAND(add_weapon_to_shiptype)
{
    if (argc < 3)
    {
        printfToClient(who, "Not enough arguments to add_weapon_to_shiptype (should be 3)");
        printfToClient(who, "Usage:  add_weapon_to_shiptype shiptype weaponname \
                            weapontype [ammo]/[laser args]");

        return;
    }

    //make sure that the ship type is declared!
    std::string shipTypeName = argv[0];
    std::string weaponName   = argv[1];

    if (!server->isShipTypeDeclared(shipTypeName))
    {
        printfToClient(who, "That ship type is not declared!");
        return;
    }

    //determine if the user specified a primary or secondary weapon
    enum
    {
        weapon_primary   = 0, 
        weapon_secondary = 1, 
    };

    const int weaponType = atoi(argv[2]);

    ShipType *currentShipType = server->shipTypes[shipTypeName];
    if (weaponName == "laser")
    {
        int numShots;
        int shotLevel;
        float shotCooldown;
        float shotBaseSpeed;
        if (argc >= 7)
        {
            numShots      = atoi(argv[3]);
            shotLevel     = atoi(argv[4]);
            shotCooldown  = atof(argv[5]);
            shotBaseSpeed = atof(argv[6]);
        }
        else
        {
            numShots      = 1;
            shotLevel     = 1;
            shotCooldown  = shotDelay;
            shotBaseSpeed = shotSpeed;
        }

		//now have the ship use boss mounts if it's a boss
		if (argc >= 8 && !strcasecmp(argv[7], "bossmount"))
			currentShipType->useBossMounts(true);

        //laser can only be a primary weapon!
        ShipType::WeaponAmmoInfo primaryInfo;
        primaryInfo.ammoType   = ammo_unlimited;
        primaryInfo.ammoAmount = 0;
        primaryInfo.weapon     = new RedPeaShooter(
            numShots,
            shotLevel, 
            shotCooldown,
            shotBaseSpeed);

        currentShipType->setPrimaryWeapon(primaryInfo);
    }
    else if (weaponName == "missiles" || 
        weaponName == "homing" ||
        weaponName == "slowmissiles" ||
		weaponName == "slowhoming")
    {
        //if the user specified missiles, make sure he also gave an ammo amount
        if (argc < 4)
        {
            printfToClient(who, "You must also specify an ammo amount for missiles!");
            return;
        }

        int ammoAmount = atoi(argv[3]);

        ProjectileWeapon *launcher = 0;
        AmmoType missileType;
        if (weaponName == "missiles")
        {
            launcher    = new MissileLauncher();
            missileType = ammo_missile;
        }
        else if (weaponName == "homing")
        {
            launcher    = new HomingMissileLauncher();
            missileType = ammo_homing_missile;
        }
		else if (weaponName == "slowhoming")
		{
			launcher = new SlowHomingMissileLauncher();
			missileType = ammo_homing_missile;
		}
        else
        {
            launcher    = new SlowMissileLauncher();
            missileType = ammo_missile;
        }

        ShipType::WeaponAmmoInfo weaponInfo;
        weaponInfo.weapon     = launcher;
        weaponInfo.ammoType   = missileType;
        weaponInfo.ammoAmount = ammoAmount;

        if (weaponType == weapon_primary)
        {            
            currentShipType->setPrimaryWeapon(weaponInfo);            
        }
        else
        {
            currentShipType->addSecondaryWeapon(weaponInfo);
        }
    }
    else
    {
        printfToClient(who, "Invalid default weapon!  (Can only give the laser, \
                            missiles, homing missiles, or slow missiles)");
        return;
    }
}

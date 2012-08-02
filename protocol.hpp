#ifndef PROTOCOL_HPP
#define PROTOCOL_HPP

// Message types
enum
{
	// Client->server: Here I am! Now give me a ship &c.
	message_connect = 0,
	
	// Client->server: My name is...
	//   name: string
	message_name = 1,
	
	// Client->server: I want to run this tokenized command.
	//   numTokens: short
	//   repeat numTokens
	//     token: string
	message_command = 2,
	
	// Server->client: A new entity has appeared in the game or entered your
	// view.
	//   entid: int
	//   image: string
	//   drawscale: float
	//   pos: Position
	//   type: int
	//   label: string
	//   radius: float
	//   effect: short

	message_add_entity = 3,
	
	// Server->client: An entity has moved since I last told you about it.
	//   entid: int
	//   pos: Position
	message_update_entity = 4,
	
	// Server->client: An entity has been destroyed or otherwise deleted.
	//   entid: int
	message_drop_entity = 5,
	
	// Server->client: Center your camera on this point.
	//   pos: Position
	//   left_bound: float
	//   right_bound: float
	//   top_bound: float
	//   bottom_bound: float
	message_camera = 6,
	
	// Server->client: I have some text I want you to put on your screen
	//   text: string
	//   where: short        A bitmask of locations
	//       Bit 0 = console
	//       Bit 1 = chat area (bottom-left)
	message_print = 7,

	// Server->client:  play a sound at this location
	//	soundName:  string
	//	x		 :  float
	//  y		 :  float
	message_sound_3D = 8,

    // Server->client:  play a sound (non-directional)
    //  soundName:  string
    message_sound = 9, 

	// Server->client:	send information about which engines are on/off
	//	where: int entID
	//	size: float (radius)
	//	which: short		Bitmask of engine types
	//		Bit 0:	forward thruster
	//		Bit 1:	backward thruster
	//		Bit 2:  strafe thruster left
	//		Bit 3:	strafe thruster right
	//		Bit 4:	rotation thruster left
	//		Bit 5:	rotation thruster right
	message_engine = 10,
	
	// Server->client
	//   x: float
	//   y: float
	//   radius: float
	//   type: int
	message_explode = 11,
	
	// Server->client: This is how much health/etc you have.
	//   health: float
	message_updatehealth = 12,

	// Server->client:  Turn this color to represent your state
	//   entID: int
	//   red:   float
	//   green: float
	//   blue:  float
	message_change_color = 13,

	// Server->client:  Return to your normal coloring
	//	 entID: int
	message_restore_color = 14,
	
	// *->self (internal use): This client has disconnected.
	message_disconnect = 15,
	
	// Server->client: Update the contents of the scoreboard and display it
	//   content: string
	message_scoreboard = 16,
	
	// Server->client: Draw a box to show that this entity is selected.
	//   entid: int
	message_selection_box = 17,
	
	// Server->client: This is your ship's entID.
	//   entid: int
	message_ship_entid = 18,

	// Server->client: Draw a line between these points (represented by entID's)
	// to show an edge
	//   ent1ID:  int
	//   ent2ID:  int
	message_add_edge = 19, 

	// Server->client: Remove (hide) the edge between the two entID's
	//	 ent1ID:  int
	//	 ent2ID:  int
	message_drop_edge = 20, 

	// Server->client:  show all of your edges
	message_show_all_edges = 21, 

	// Server->client:  hide all of your edges
	message_hide_all_edges = 22,

	// Server->client:  new map, so clear out your edge pool
	message_clear_edges = 23, 
	
	// Server->client: Add a rectangular region for display
	// (This representation is rediculously inefficient, but it's normally
	// only used locally so it doesn't matter.)
	//   num_rects: int
	// repeat num_rects:
	//   string: name
	//   min_x: float
	//   max_x: float
	//   min_y: float
	//   max_y: float
	message_set_rects = 24,

	// What weapon(s) to disply on the HUD
	//	total: int
	//	current: int
	// foreach weapon:
	//  ammo: int   (-1 for unlimited)
	//	name: string
	message_weapons = 25,

	// Server->client:  draw a line of lightning between these two points.
	message_lightning = 26,

	// Show some shield effects
	//	source:	int (entID)
	//	type:	short (bitmask)
	//	timer:	float
	//	radius:	float
	message_shield = 27,

	// Server->client:  Draw an icon over your ally
	//   entID:  int
	message_mark_ally = 28,
	
	// Server->client: Update your engine diamond
	//   forward: float
	//   left: float
	//   right: float
	//   down: float
	//   warmup: float
	message_update_engines = 29,

	// Server->client: Set the avatarID of a client on client-side
	//   int: entID of avatar
	message_avatar = 30,

	// Server->client: Change the draw effect of an entity
	//	entID: int
	//  drawEffect: short
	message_update_effect = 31,
	
	// Server->client: Sync your clock to this time
	//  time: float
	message_sync_time = 32,
};

// Entity types
enum
{
	ent_player_ship    = 1, // player-controlled ships
	ent_ai_ally_ship   = 2, // ai-controlled ships
    ent_ai_enemy_ship  = 3, // enemy bots
	ent_projectile     = 4, // bullets, missles, asteroids, moons - moving things we don't want on radar
	ent_large_obstacle = 5, // large, stationary objects to draw on radar
	ent_powerup        = 6, // A grabbable item
	ent_invisible      = 7, // A dummy entity that shouldn't be drawn
	ent_rectangular	   = 8, // Rectangular objects that must display differently on radar
	ent_flag           = 9,
};

enum team_type
{
	team_free  = 0,	//free-for-all team; you can attack anyone and vice versa
	team_human = 1, //single player: you and your allies.
	team_enemy = 2, //single player: enemy bots
	team_red   = 3,
	team_blue  = 4,
};

// Draw effects
enum
{
	effect_none = 0,
	effect_sparking,
	effect_trail,
	effect_missile_trail,
	effect_shot,
	effect_shot2,
	effect_shot3,
	effect_shot4,
	effect_spawnshield,
	effect_hitshield,
	effect_extrashield,
	effect_generatorshield,
	effect_money,
};

// Wire format
struct MessageHeader
{
	unsigned short size;
	unsigned short type;
	// Message data
};

#endif

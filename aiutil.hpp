#ifndef AIUTIL_HPP
#define AIUTIL_HPP

enum ai_state
{
	state_roaming 	   = 0x0020, 
	state_chasing 	   = 0x0021, 
	state_evading      = 0x0022, 
	state_defending    = 0x0023,
	state_stopping     = 0x0024, 
};

enum ai_role
{
	role_chaser		   = 0x0030, 
	role_interceptor   = 0x0031, 
	role_defender	   = 0x0032, 
};

//represents the bot's state while it is transitioning between waypoints
enum ai_steering_state
{
	steering_state_at_new_dest	  = 0x0050, 
	steering_state_thrust_to_dest = 0x0051, 
	steering_state_face_target	  = 0x0052, 
	steering_state_return_to_dest = 0x0053, 
	steering_state_dest_too_close = 0x0054, 
};

class WaypointMarker;
enum visit_status;

//comparison function for waypoints
template<typename T>
class NearerWaypoint
{
public:
	bool operator() (const T &waypoint1, const T &waypoint2) const;
};

//a waypoint with a smaller weight has a higher priority, hence the > sign
template<typename T>
bool NearerWaypoint<T>::operator() (
	const T &waypoint1, 
	const T &waypoint2) const
{
	return (waypoint1->getWeight() > waypoint2->getWeight());
}

#endif	//AIUTIL_HPP
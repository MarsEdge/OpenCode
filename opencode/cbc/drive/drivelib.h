#ifndef __DRIVELIB_H__
#define __DRIVELIB_H__

#include <stdio.h>
#include <math.h>

#include "/usr/include/kovan/kovan.h"
#include "../sensor/sensorlib.h"

#define PI 3.14159
#define DEG_TO_RAD (PI / 180.0)

void build_left_wheel(int port, long ticks_cycle, float speed_proportion, float wheel_diameter, float radial_distance);
// 0 <--> 3 (port), ~1100 (ticks), 1.0 + | - 0.25 (unitless), + in mm, + in mm
//Shortcut to build the left wheel structure to be utilized by various functions

void build_right_wheel(int port, long ticks_cycle, float speed_proportion, float wheel_diameter, float radial_distance);
// 0 <--> 3 (port), ~1100 (ticks), 1.0 + | - 0.25 (unitless), + in mm, + in mm
//Shortcut to build the right wheel structure to be utilized by various functions

int cbc_straight(int speed, float distance);
// 0 <--> 1000, + | - in mm
//Moves the center of the CBC drivetrain in a straight line
//Returns 1 if executed, -1 if error is detected

int cbc_arc(int speed, float radius, float theta);
// 0 <--> 1000 (unitless), + | - in mm, + | - in degrees
//Moves the center of the CBC drivetrain in a constant radial arc
//Returns 1 if executed, -1 if error is detected

int cbc_spin(int speed, float theta);
// 0 <--> 1000, + | - in degrees
//Rotates the center of the CBC drivetrain
//Returns 1 if executed, -1 if error is detected

struct cbc_side
{
	struct wheel_properties
	{
		int port;
		int last_requested_speed;
		long ticks_cycle;
		float speed_proportion;
		float wheel_diameter;
		float radial_distance;
		float ticks_per_mm;
	}wheel;
	struct lincolns_tophat
	{
		int port;
		int black;
		int white;
		int error;
		long timeout;
	}tophat;
	struct touch_me
	{
		int port;
		long timeout;
	}touch;
}left, right;
struct cbc_accel
{
	float x_knaught[3];
	long timeout;
}acceleramator;
void build_left_wheel(int port, long ticks_cycle, float speed_proportion, float wheel_diameter, float radial_distance)
{
	left.wheel.port = port;
	left.wheel.ticks_cycle = ticks_cycle;
	left.wheel.speed_proportion = speed_proportion;
	left.wheel.wheel_diameter = wheel_diameter;
	left.wheel.radial_distance = radial_distance;
	left.wheel.ticks_per_mm = (float)ticks_cycle/(wheel_diameter*PI);
}
void build_right_wheel(int port, long ticks_cycle, float speed_proportion, float wheel_diameter, float radial_distance)
{
	right.wheel.port = port;
	right.wheel.ticks_cycle = ticks_cycle;
	right.wheel.speed_proportion = speed_proportion;
	right.wheel.wheel_diameter = wheel_diameter;
	right.wheel.radial_distance = radial_distance;
	right.wheel.ticks_per_mm = (float)ticks_cycle/(wheel_diameter*PI);
}
void build_left_tophat(int port, int white, int black, int error, long timeout)
{
	analog_sensor left_drive_tophat = build_analog_sensor(port, 0, 1, 0);
	left.tophat.port = port;
	left.tophat.white = white;
	left.tophat.black = black;
	left.tophat.error = error;
	left.tophat.timeout = timeout;
}
void build_right_tophat(int port, int white, int black, int error, long timeout)
{
	analog_sensor right_drive_tophat = build_analog_sensor(port, 0, 1, 0);
	right.tophat.port = port;
	right.tophat.white = white;
	right.tophat.black = black;
	right.tophat.error = error;
	right.tophat.timeout = timeout;
}
void build_left_touch(int port, long timeout)
{
	right.touch.port = port;
	right.touch.timeout = timeout;
}
void build_right_touch(int port, long timeout)
{
	right.touch.port = port;
	right.touch.timeout = timeout;
}

int cbc_calc_wait(int distance)
{
	if(left.wheel.last_requested_speed != 0 && distance != 0 && left.wheel.ticks_per_mm != 0)
	{
		return (int)fabs((1000.0*(float)distance*left.wheel.ticks_per_mm)/(float)left.wheel.last_requested_speed);
	}
	else
	{
		return 0;
	}
}
void cbc_wait(int distance)
{
	//use a timer for the left wheel so we don't use bmd() . It is slow and inconsistent
	msleep( cbc_calc_wait(distance) );
}
void cbc_halt()
{
	// freeze wheels for active stop (cutting power ist inaccurate)
	set_auto_publish(0); // won't send every command one by one to kmod
	publish(); // send every enqueued package to kmod
	freeze(left.wheel.port);
	freeze(right.wheel.port);
	set_auto_publish(1); // send every command one by one to kmod
	publish(); // send enqueued driving commands at once to kmod
	
	msleep(100); // wait for bot to stop 
	
	// cut motor's power (for saving battery and fitting the rules)
	set_auto_publish(0); // won't send every command one by one to kmod
	publish(); // send every enqueued package to kmod
	off(left.wheel.port);
	off(right.wheel.port);
	set_auto_publish(1); // send every command one by one to kmod
	publish(); // send enqueued driving commands at once to kmod
}
int cbc_direct(int lspeed, int rspeed)
{
	set_auto_publish(0); // won't send every command one by one to kmod
	publish(); // send every enqueued package to kmod
	if(lspeed!=0)
	{
		mav(left.wheel.port, lspeed); // enqueue driving commands
	}else{
		freeze(left.wheel.port);
	}
	if(rspeed!=0)
	{
		mav(right.wheel.port, rspeed); // enqueue driving commands
	}else{
		freeze(right.wheel.port);
	}
	set_auto_publish(1); // send every command one by one to kmod
	publish(); // send enqueued driving commands at once to kmod
	left.wheel.last_requested_speed = lspeed;
	right.wheel.last_requested_speed = rspeed;
	return 1;
}
int cbc_straight(int speed, float distance)
{
	float lticks = ((distance * left.wheel.ticks_cycle) / (left.wheel.wheel_diameter * PI)) * left.wheel.speed_proportion;
	float rticks = ((distance * right.wheel.ticks_cycle) / (right.wheel.wheel_diameter * PI)) * right.wheel.speed_proportion;
	float lspeed = (float)speed * left.wheel.speed_proportion;
	float rspeed = (float)speed * right.wheel.speed_proportion;

	if(rspeed > 1500 || lspeed > 1500 || rspeed < -1500 || lspeed < -1500)
	{
		printf("\nWarning! Invalid CBC Speed\n");
		return -1;
	}
	else
	{
		cbc_direct(copysignf(lspeed, distance), copysignf(rspeed, distance));
		cbc_wait(distance);
		return 1;
	}
}
int cbc_arc(int speed, float radius, float theta) // 0 <--> 1000 (unitless), + | - in mm, + | - in degrees
{
	float arc_length = radius * theta * DEG_TO_RAD;
	float ldistance = (radius - left.wheel.radial_distance) * theta * DEG_TO_RAD;
	float rdistance = (radius + right.wheel.radial_distance) * theta * DEG_TO_RAD;
	float lticks = left.wheel.speed_proportion * ((ldistance * left.wheel.ticks_cycle) / (left.wheel.wheel_diameter * PI));
	float rticks = right.wheel.speed_proportion * ((rdistance * right.wheel.ticks_cycle) / (right.wheel.wheel_diameter * PI));
	float lspeed = (float)speed * left.wheel.speed_proportion * ldistance / arc_length;
	float rspeed = (float)speed * right.wheel.speed_proportion * rdistance / arc_length;

	if(rspeed > 1500 || lspeed > 1500 || rspeed < -1500 || lspeed < -1500)
	{
		printf("\nWarning! Invalid CBC Speed\n");
		return -1;
	}
	else
	{
		cbc_direct(copysignf(lspeed, ldistance), copysignf(rspeed, rdistance));
		cbc_wait(ldistance);
		return 1;
	}
}
int cbc_spin(int speed, float theta)
{
	float ldistance = -1.0 * left.wheel.radial_distance * theta * DEG_TO_RAD;
	float rdistance = right.wheel.radial_distance * theta * DEG_TO_RAD;
	float lticks = left.wheel.speed_proportion * ((ldistance * left.wheel.ticks_cycle) / (left.wheel.wheel_diameter * PI));
	float rticks = right.wheel.speed_proportion * ((rdistance * right.wheel.ticks_cycle) / (right.wheel.wheel_diameter * PI));
	float lspeed = (float)speed * left.wheel.speed_proportion * left.wheel.radial_distance / (left.wheel.radial_distance + left.wheel.radial_distance);
	float rspeed = (float)speed * right.wheel.speed_proportion * right.wheel.radial_distance / (right.wheel.radial_distance + right.wheel.radial_distance);

	if(rspeed > 1500 || lspeed > 1500 || rspeed < -1500 || lspeed < -1500)
	{
		printf("\nWarning! Invalid CBC Speed\n");
		return -1;
	}
	else
	{
		cbc_direct(-1*copysignf(lspeed, theta), copysignf(rspeed, theta));
		cbc_wait(ldistance);
		return 1;
	}

}
int cbc_align_touch()
{
	long ltimeout = left.touch.timeout;
	long rtimeout = right.touch.timeout;
	cbc_direct(left.wheel.last_requested_speed, right.wheel.last_requested_speed);
	while(ltimeout > 0 && rtimeout > 0 && !digital(left.tophat.port) && !digital(right.tophat.port))
	{

		ltimeout -= 10L;
		rtimeout -= 10L;
	}
	cbc_halt();
}
int cbc_align_black()
{
	long ltimeout = left.tophat.timeout;
	long rtimeout = right.tophat.timeout;
	cbc_direct(left.wheel.last_requested_speed, right.wheel.last_requested_speed);
	while((ltimeout > 0 || rtimeout > 0) && (analog10(left.tophat.port) < (left.tophat.white + left.tophat.error) || analog10(right.tophat.port) < (right.tophat.white + right.tophat.error)))
	{
		if(analog10(left.tophat.port) > (left.tophat.black - left.tophat.error))
		{
			off(left.wheel.port);
		}
		if(analog10(right.tophat.port) > (right.tophat.black - right.tophat.error))
		{
			off(right.wheel.port);
		}
		msleep(10L);
		ltimeout -= 10L;
		rtimeout -= 10L;
	}
}
int cbc_align_white()
{
	long ltimeout = left.tophat.timeout;
	long rtimeout = right.tophat.timeout;
	cbc_direct(left.wheel.last_requested_speed, right.wheel.last_requested_speed);
	while((ltimeout > 0 || rtimeout > 0) && (analog10(left.tophat.port) > (left.tophat.black - left.tophat.error) || analog10(right.tophat.port) > (right.tophat.black - right.tophat.error)))
	{
		if(analog10(left.tophat.port) < (left.tophat.white + left.tophat.error))
		{
			off(left.wheel.port);
		}
		if(analog10(right.tophat.port) < (right.tophat.white + right.tophat.error))
		{
			off(right.wheel.port);
		}
		msleep(10L);
		ltimeout -= 10L;
		rtimeout -= 10L;
	}
}
void *accel_bump(void *this_accel)
{
	struct cbc_accel *accel = (struct cbc_accel *)this_accel;
	while(accel->timeout > 0)
	{
		if(accel->x_knaught[0] > (accel_x() + 0.1) || accel->x_knaught[0] < (accel_x() - 0.1))
		{
			cbc_halt();
			break;
		}
		else if(accel->x_knaught[1] > (accel_y() + 0.1) || accel->x_knaught[2] < (accel_y() - 0.1))
		{
			cbc_halt();
			break;
		}
		else if(accel->x_knaught[2] > (accel_z() + 0.1) || accel->x_knaught[2] < (accel_z() - 0.1))
		{
			cbc_halt();
			break;
		}
	}
}
void cbc_sense_accel()
{
	acceleramator.x_knaught[1] = accel_x();
	acceleramator.x_knaught[2] = accel_y();
	acceleramator.x_knaught[3] = accel_z();
	struct cbc_accel *here = &acceleramator;
}
#endif


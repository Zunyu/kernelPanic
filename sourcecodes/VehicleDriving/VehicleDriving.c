/*
    535_Project: VehicleDriving.c

    Version 1.0

    Engineer: Jiao Yang

    &&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&   
    &&  Attention: This is a .c code for vehicle IC processor P8X32A, not for gumstix!  &&
    &&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
*/

#include "simpletools.h"
#include "abdrive.h"
#include "ping.h"

// Variables for Ultrasound Detector
int secureDistance;				// the secure distance between obstacle and vehicle
int measuredDistance;				// the measured distance between obstacle and vehicle

// Angle and Direction info from gumstix
int d0, d1, d2, d3, d4;
int s;

// Assembled Angle and Direction info for invoking functions
int angle[256];
int direction[256];

int main()
{
	secureDistance = 25;			// "25cm"

	int i = 0;
	int j = 0;
	int k = 0;
	int temp;

	low(10);

	pause(1000);				// wait 1s

	while(1) {
		while (input(5) == 0);		// wait for ready notification of route info from gumstix

		pause(15);			// wait for angle and direction

		low(10);			// clear signal flag for obstacle interrupt

		for (i = 0; ; i++) {
			d0 = input(0);
			d1 = input(1);
			d2 = input(2);
			d3 = input(3);
			d4 = input(4);
			s = input(5);

			if(d0 == 1 && d1 == 1 && d2 == 1 && d3 == 1 && d4 == 1) break;		// Transmit is over!

			angle[i] = (16 * d4 + 8 * d3 + 4 * d2 + 2 * d1 + 1 * d0) * 10;
			direction[i] = s;

			pause(10);
		}

		for (j = 0; j < i; j++) {
			if (direction[j] == 0) {
				temp = (angle[j] * 284 / 1000) + 0.5;
				drive_goto(temp, (0 - temp));
			}
			else if (direction[j] == 1) {
				temp = (angle[j] * 284 / 1000) + 0.5;
				drive_goto((0 - temp), temp);
			}
			pause(50);

			drive_goto(25, 25);

			for(k = 0; k < 10; k++) {
				measuredDistance = ping_cm(8);					// SIG from P8

				if (measuredDistance > secureDistance) pause(5);
				else {
					drive_speed(0, 0);
					high(10);						// set flag
					break;
				}
			}
			if (k < 10) break;
		}

		if (k == 10) {
			drive_speed(64, 64);

			while(1) {
				measuredDistance = ping_cm(8);
				if (measuredDistance < secureDistance) {
					drive_speed(0, 0);
					high(10);						// set flag
					break;
				}          
				pause(5);
			}
		}

		i = 0;
		j = 0;
		k = 0;
	}
}

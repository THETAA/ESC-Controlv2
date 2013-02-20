#include <iostream>
#include <wiringPi.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>

#define BUFLEN 512	// Buffer Length
#define PORT 777	// UDP Port

using namespace std;

// Motors
int M01 = 0;
int M02 = 1;
int M03 = 2;
int M04 = 3;

// Navigation Lights
int LightRed01 = 4;
int LightRed02 = 6;

int LightGreen01 = 5;
int LightGreen02 = 7;

// Motor Settings
int Neutral = 800;			// µSeconds
int Full_Throttle = 1900;	// µSeconds

// Timings
int PulseLenght = 2500;

// Motor Values
int Motor0_Amount = 0; // 0% Speed
int Motor1_Amount = 0; // 0% Speed
int Motor2_Amount = 0; // 0% Speed
int Motor3_Amount = 0; // 0% Speed

void setup() {
	printf("[!] Setting up Motors\n");
	pinMode(0, OUTPUT);
	pinMode(1, OUTPUT);
	pinMode(2, OUTPUT);
	pinMode(3, OUTPUT);
}

void MotorDrive(int Motor) {
	int motor_amount;
	switch (Motor) {
	case 0:
		motor_amount = Motor0_Amount;
		break;
	case 1:
		motor_amount = Motor1_Amount;
		break;
	case 2:
		motor_amount = Motor2_Amount;
		break;
	case 3:
		motor_amount = Motor3_Amount;
		break;
	}
	int calc_delay = Neutral + ((Full_Throttle - Neutral) / 100 * motor_amount);
	digitalWrite(Motor, 1);
	usleep(calc_delay);
	digitalWrite(Motor, 0);
	usleep(PulseLenght - calc_delay);
}

void* MotorRun(void *) {
	cout << "[!] Motor Thread is running." << endl;

	while (1) {
		MotorDrive(M01);
		MotorDrive(M02);
		MotorDrive(M03);
		MotorDrive(M04);
	}
	return 0;
}

void MotorControl(int M1, int M2, int M3, int M4) {
	Motor0_Amount = M1;
	Motor1_Amount = M2;
	Motor2_Amount = M3;
	Motor3_Amount = M4;
}

void ESCcalibrate() {
	cout << "[!] Calibrating..." << endl;
	MotorControl(0, 0, 0, 0);
	sleep(30);

	/* Alternative (4 seconds)
	 *
	 * cout << "[!] Calibrating..." << endl;
	 * MotorControl(100,100,100,100);
	 * sleep(3);
	 * MotorControl(0,0,0,0);
	 * sleep(1);
	 * cout << "[!] Calibrated!" << endl;
	 *
	 */
}

void* ESCcontrol(void *) {
	cout << "[!] Standby" << endl;
	ESCcalibrate();
	cout << "[!] ESC working" << endl;

	// UDP Setup
	struct sockaddr_in my_addr, cli_addr;
	int sockfd;
	socklen_t slen = sizeof(cli_addr);
	char buf[BUFLEN];

	if (!((sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1))
		printf("[~] Socket created successful\n");

	bzero(&my_addr, sizeof(my_addr));
	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(PORT);
	my_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	if (!(bind(sockfd, (struct sockaddr*) &my_addr, sizeof(my_addr)) == -1))
		printf("[~] Socket binded successful\n");

	while (1) {
		recvfrom(sockfd, buf, BUFLEN, 0, (struct sockaddr*) &cli_addr, &slen);

		switch (buf[0]) {
		case 48:
			MotorControl(0, 0, 0, 0);
			break;
		case 49:
			MotorControl(25, 25, 25, 25);
			break;
		case 50:
			MotorControl(50, 50, 50, 50);
			break;
		case 51:
			MotorControl(75, 75, 75, 75);
			break;
		case 52:
			MotorControl(100, 100, 100, 100);
			break;
		}
	}

	return 0;
}

void* NavLights(void*) {
	while (1) {
		digitalWrite(LightRed01, 1);
		digitalWrite(LightRed02, 1);
		digitalWrite(LightGreen01, 1);
		digitalWrite(LightGreen02, 1);

		usleep(200000); // 200ms on

		digitalWrite(LightRed01, 0);
		digitalWrite(LightRed02, 0);
		digitalWrite(LightGreen01, 0);
		digitalWrite(LightGreen02, 0);

		usleep(100000); // 100ms off
	}
	return 0;
}

int main(void) {
	if (wiringPiSetup() == -1)
		return 1;

	setup();

	pthread_t t1; // Motors
	pthread_t t2; // NavLights
	pthread_t t3; // ESC

	pthread_create(&t1, NULL, &MotorRun, NULL);
	pthread_create(&t2, NULL, &NavLights, NULL);
	pthread_create(&t3, NULL, &ESCcontrol, NULL);

	pthread_join(t1, NULL);
	pthread_join(t2, NULL);
	pthread_join(t3, NULL);

	return 0;
}

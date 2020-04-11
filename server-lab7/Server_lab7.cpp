/*
Name: Ruby Anne Bautista
BTN415-Lab 5
*/
#include <windows.networking.sockets.h>
#pragma comment(lib, "Ws2_32.lib")

#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <algorithm>
#include "glm\glm.hpp"
#include "Serialize.h"



#define MAX_CLIENTS 3

using namespace std;



struct Actor
{
	int id;	//Unique ID of this actor
	float x_pos, y_pos;	//Position of a Actor
	bool isAlive;	//Still alive?
	bool isBullet;	//Is this actor a bullet or a player?
	bool touchedActor;	//If this Actor is collided with other Actor
	glm::vec2 direction;	//Motion direction
};



std::vector<Actor> sceneGraph;
void bulletMove(Actor &v) {
	if (v.isBullet) {
		float Actor_velocity = 4.0f; //Bullet speed
		glm::vec2 moving_dir = glm::normalize(v.direction);
		v.x_pos += 0.001 * Actor_velocity * moving_dir.x;
		v.y_pos += 0.001 * Actor_velocity * moving_dir.y;


	}
}
void makeGameScene(Actor& a) {

	//find if it exists already-the actor does not exist and is a bullet
	auto it = std::find_if(sceneGraph.begin(), sceneGraph.end(), [&](Actor c) {return c.id == a.id && !c.isBullet; });

	
	//if !exists or is a bullet
	if (it == sceneGraph.end()) {
		sceneGraph.push_back(a);
	}
	else {//replace/update existing instance
		*(it) = a;
	}

	for_each(sceneGraph.begin(), sceneGraph.end(), [](Actor& c) {bulletMove(c);});



	for (Actor a : sceneGraph) {
		cout << a.id << endl;
	}

	

}


bool Collided(Actor v1, Actor v2)
{
	bool result = false;
	if (v1.isBullet && v2.isBullet) {}
	else if (v1.isAlive && v2.isAlive)
	{
		float x_Dist = abs(v1.x_pos - v2.x_pos);
		float y_Dist = abs(v1.y_pos - v2.y_pos);
		if (x_Dist <= 0.9 && y_Dist <= 0.9) { result = true; }
	}
	return result;
}


void checkCollision()
{
	for (int i = 0; i < sceneGraph.size(); i++)
	{
		for (int j = 0; j < sceneGraph.size(); j++)
		{
			if (i != j)
			{
				Actor v = sceneGraph[i];
				Actor u = sceneGraph[j];

				if ((v.isAlive && u.isAlive) && ((v.isBullet && !u.isBullet) || (!v.isBullet && u.isBullet)) && Collided(v, u))
				{
					//Increasescore = true;
				}
				if (Collided(v, u))
				{
					cout << "THEY COLLiDEDDED" << endl;
					v.isAlive = false;
					u.isAlive = false;
					sceneGraph[i] = v;
					sceneGraph[j] = u;
				}
			}
		}
	}
}





void deleteActor(int id) {

	sceneGraph.erase(std::remove_if(sceneGraph.begin(), sceneGraph.end(), [&](Actor a) {return a.id == id; }));

}

void handle_Client(SOCKET s, int id) {

	int flag = 0;
	while (flag != -1) {

		Actor x;
		char* rvbuff = (char*)malloc(sizeof(Actor));

		//receive from client
		int n = recv(s, rvbuff, sizeof(Actor), 0);
		if (n > 0) {

			memcpy(&x, rvbuff, sizeof(Actor));
			makeGameScene(x);
			



		}
		//Should I make a deleteActor func to handle clients that leave
		else if (n == 0) {
			cout << id << (" Connection closed\n");
			flag = -1;
			deleteActor(x.id);
		}
		else {
			cout << "This is the device" << id << " recv failed: " << WSAGetLastError();
			flag = -1;
			deleteActor(x.id);

		}


		//Serialize
		

		checkCollision();
		char* t = convert(sceneGraph);
		int y = sizeof(Actor) * sceneGraph.size();

		send(s, t, y, 0);



	}


}





int main()
{

	struct sockaddr_in SvrAddr;				//structure to hold servers IP address-holds param- port num, ip address
	SOCKET WelcomeSocket, ConnectionSocket;	//socket descriptors
	WSADATA wsaData;

	//Data buffers


	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)///What is this doing?
		return -1;

	//create welcoming socket at port and bind local address
	if ((WelcomeSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCKET)
		return -1;


	SvrAddr.sin_family = AF_INET;			//set family to internet

	SvrAddr.sin_addr.s_addr = INADDR_ANY;   //inet_addr("127.0.0.1");	//set the local IP address
	//To be completed by students
	//What does 127.0.0.1 represent?
	///what is htons?
	SvrAddr.sin_port = htons(27000);							//set the port number- 
	//To be complete by students:
	//Now, try to communicate through port number 27001

	if ((bind(WelcomeSocket, (struct sockaddr*) & SvrAddr, sizeof(SvrAddr))) == SOCKET_ERROR)
	{
		closesocket(WelcomeSocket);
		WSACleanup();
		return -1;
	}

	//Specify the maximum number of clients that can be queued
	if (listen(WelcomeSocket, MAX_CLIENTS) == SOCKET_ERROR)
	{
		closesocket(WelcomeSocket);
		std::cout << "Connection failed- Socket Error!" << std::endl;

		return -1;
	}

	//Main server loop - accept and handle requests from clients
	std::cout << "Waiting for client connection" << std::endl;



	std::thread t[MAX_CLIENTS];

	for (int i = 0; i < MAX_CLIENTS; ++i) {
		//wait for an incoming connection from a client
		if ((ConnectionSocket = (accept(WelcomeSocket, NULL, NULL))) == SOCKET_ERROR)///accept will wait for connection
		{

			closesocket(WelcomeSocket);
			std::cout << "Sorry bad connection it was rejected" << std::endl;

			return -1;
		}
		else
		{
			t[i] = std::thread(handle_Client, ConnectionSocket, i + 1);
			cout << "Thread Created for Client: " << 1 + i << endl;

		}
	}


	for (int i = 0; i < MAX_CLIENTS; ++i) {
		t[i].join();
	}

	closesocket(ConnectionSocket);



	WSACleanup();

	std::cout << "Successful Run\n";
}
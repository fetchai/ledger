#pragma once

// Server side C/C++ program to demonstrate Socket programming
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include <queue>
#include <math.h>
#include <cmath> 
#define earthRadiusKm 6371.0

#define PORT 8080

// code example from here: https://www.geeksforgeeks.org/socket-programming-cc/

void pipeResults(std::queue<std::string> &stringsToSend){

  int server_fd, new_socket;
  struct sockaddr_in address;
  int opt = 1;
  int addrlen = sizeof(address);

  while(1){

    //////////////////////////////////////////////////////////////////////////////////////
    // Create server connection

    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        //perror("socket failed");
        continue; //exit(EXIT_FAILURE);
    }

    // Forcefully attaching socket to the port 8080
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
    {
        //perror("setsockopt");
        continue; //exit(EXIT_FAILURE);
    }
    address.sin_family      = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port        = htons( PORT );

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address))<0)
    {
        //perror("bind failed");
        continue; //exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0)
    {
        //perror("listen");
        continue; //exit(EXIT_FAILURE);
    }
    if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0)
    {
        //perror("accept");
        continue; //exit(EXIT_FAILURE);
    }

    // End create server connection
    //////////////////////////////////////////////////////////////////////////////////////

    while(1)
    {
      std::this_thread::sleep_for(std::chrono::microseconds{10});

      if(!stringsToSend.empty()){
        send(new_socket , stringsToSend.front().c_str() , stringsToSend.front().length(), 0 );
        stringsToSend.pop();
      }
      //send(new_socket , hello , strlen(hello) , 0 );
    }
  }
}

int getAEAStrings(std::queue<std::string> &stringsToReceive){

  while(1)
  {
    sleep(3);
    int sock = 0, valread;
    struct sockaddr_in serv_addr;
    char buffer[1024] = {0};

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("\n Socket creation error \n");
        continue;
        //return -1;
    }

    memset(&serv_addr, '0', sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // Convert IPv4 and IPv6 addresses from text to binary form
    if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr)<=0) 
    {
        printf("\nInvalid address/ Address not supported \n");
        continue;
        //return -1;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        printf("\nConnection Failed \n");
        continue;
        //return -1;
    }

    //send(sock , hello , strlen(hello) , 0 );

    // TODO: (`HUT`) : this should actually parse by newline
    while(1){
      valread = read( sock , buffer, 1024);
      //std::cout << ">" << valread << std::endl;
      //std::cout << ">" << buffer << std::endl;
      stringsToReceive.push(std::string{buffer, static_cast<long unsigned int>(valread)});
    }

  }
  return 0;
}

// This function converts decimal degrees to radians
double deg2rad(double deg)
{
  return (deg * M_PI / 180);
}

//  This function converts radians to decimal degrees
double rad2deg(double rad)
{
  return (rad * 180 / M_PI);
}

/**
 * Returns the distance between two points on the Earth.
 * Direct translation from http://en.wikipedia.org/wiki/Haversine_formula
 * @param lat1d Latitude of the first point in degrees
 * @param lon1d Longitude of the first point in degrees
 * @param lat2d Latitude of the second point in degrees
 * @param lon2d Longitude of the second point in degrees
 * @return The distance between the two points in kilometers
 */
double distanceEarthKm(double lat1d, double lon1d, double lat2d, double lon2d)
{
  std::cout << "vals: " << lat1d << " " << lon1d << " " << lat2d << " " << lon2d << std::endl << std::endl;

  double lat1r, lon1r, lat2r, lon2r, u, v;
  lat1r = deg2rad(lat1d);
  lon1r = deg2rad(lon1d);
  lat2r = deg2rad(lat2d);
  lon2r = deg2rad(lon2d);
  u = sin((lat2r - lat1r)/2);
  v = sin((lon2r - lon1r)/2);
  return 2.0 * earthRadiusKm * asin(sqrt(u * u + cos(lat1r) * cos(lat2r) * v * v));
}

double distanceEarthKm(const std::string &lat1d, const std::string &lon1d, const std::string &lat2d, const std::string &lon2d)
{
  using namespace std;
  std::cout << "Vals: " << lat1d << " " << lon1d << " " << lat2d << " " << lon2d << std::endl;
  return distanceEarthKm(atof(lat1d.c_str()), atof(lon1d.c_str()), atof(lat2d.c_str()), atof(lon2d.c_str()));
}


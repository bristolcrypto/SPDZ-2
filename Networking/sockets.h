// (C) 2018 University of Bristol. See License.txt

#ifndef _sockets
#define _sockets

#include "Networking/data.h"

#include <errno.h>      /* Errors */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

#include <arpa/inet.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>   /* Wait for Process Termination */

#include <iostream>
using namespace std;


void error(const char *str1,const char *str2);
void error(const char *str);

void set_up_server_socket(sockaddr_in& dest,int& consocket,int& main_socket,int Portnum);
void close_server_socket(int consocket,int main_socket);

void set_up_client_socket(int& mysocket,const char* hostname,int Portnum);
void close_client_socket(int socket);

/* Send and receive 8 bit integers */
void send(int socket,int a);
void receive(int socket,int& a);

// same for words
void send(int socket, size_t a, size_t len);
void receive(int socket, size_t& a, size_t len);

void send_ack(int socket);
int get_ack(int socket);


extern unsigned long long sent_amount, sent_counter;

inline void send(int socket,octet *msg,size_t len)
{
  size_t i = 0;
  while (i < len)
    {
      int j = send(socket,msg+i,len-i,0);
      i += j;
      if (j < 0 and errno != EINTR)
        { error("Send error - 1 ");  }
    }

  sent_amount += len;
  sent_counter++;
}

inline void receive(int socket,octet *msg,size_t len)
{
  size_t i=0;
  int fail = 0;
  while (len-i>0)
    { int j=recv(socket,msg+i,len-i,0);
      if (j<0)
        {
          if (errno == EAGAIN or errno == EINTR)
            {
              if (++fail > 100)
                error("Unavailable too many times");
              else
                {
                  cout << "Unavailable, trying again in 1 ms" << endl;
                  usleep(1000);
                }
            }
          else
            { error("Receiving error - 1"); }
        }
      else
        i=i+j;
    }
}

#endif

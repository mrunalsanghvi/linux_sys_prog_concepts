#include<sys/socket.h>
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<pthread.h>
#include<string.h>
#include<sys/epoll.h>
#include <arpa/inet.h>
#include <getopt.h>
#include<stdbool.h>
#define IP_ADDR 1
#define PORT_NO 2
#define MAX_EVENTS 100

int main(int argc, char *argv[])
{
	const struct option longopts[] = {
				{"ip", required_argument, 0, IP_ADDR},
				{"port", required_argument, 0, PORT_NO},
				{0, 0, 0, 0}
	};
	int longindex;
	char ip[16]={0};
	uint16_t port_no;
	int opt=0;
	while((opt = getopt_long (argc, argv, "",&longopts[0],&longindex)) != -1)
	{
		switch(opt){
	
			case IP_ADDR:
				strcpy(ip,optarg);	
				break;
			case PORT_NO:
				port_no=atoi(optarg);	
				break;
			default:
				break;
		}
	}
	struct sockaddr_in addr_s;
	int cfd;	
	cfd=socket(AF_INET, SOCK_STREAM, 0);
	if(cfd < 0)	
	{
		perror("socket:"); 
		exit(EXIT_FAILURE);
	}
	int one=1;
	if(setsockopt(cfd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, (void *)&one, sizeof(one))<0)
	{
		perror("setsocketopt");
		exit(EXIT_FAILURE);
	}

	addr_s.sin_family=PF_INET;
	addr_s.sin_port=htons(port_no);
	addr_s.sin_addr.s_addr=inet_addr(ip);
	if(addr_s.sin_addr.s_addr==INADDR_NONE)
	{
		perror("inet_addr");
		printf("invalid IP address, Please provide address in ipv4 format: usage:0.0.0.0\n");
		exit(EXIT_FAILURE);
		
	}	
	if(connect(cfd,(struct sockaddr*)&addr_s, sizeof(addr_s))<0)
	{
		perror("connect");
		exit(EXIT_FAILURE);
	}
//	while(1)
	{	
			char buf[256]={'\0'};
			if(send(cfd, "ls", strlen("ls"), 0)<0)
			{
				perror("recv");
				exit(EXIT_FAILURE);
			}
			if(recv(cfd, buf, 256, 0)<0)
			{
				perror("send");
				exit(EXIT_FAILURE);
			}
	//		printf("received:%s\n",buf);	
	}
	//while(1);
	close(cfd);
exit(EXIT_SUCCESS);	
}

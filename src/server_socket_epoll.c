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

int event_queue[MAX_EVENTS];

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
	int sfd;	
	sfd=socket(AF_INET, SOCK_STREAM, 0);
	if(sfd < 0)	
	{
		perror("socket:"); 
		exit(EXIT_FAILURE);
	}
	int one=1;
	if(setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, (void *)&one, sizeof(one))<0)
	{
		perror("setsocketopt");
		exit(EXIT_FAILURE);
	}
	struct sockaddr_in addr_s;
	addr_s.sin_family=PF_INET;
	addr_s.sin_port=htons(port_no);
	addr_s.sin_addr.s_addr=inet_addr(ip);
	if(addr_s.sin_addr.s_addr==INADDR_NONE)
	{
		perror("inet_addr");
		printf("invalid IP address, Please provide address in ipv4 format: usage:0.0.0.0\n");
		exit(EXIT_FAILURE);
		
	}	
	if(bind(sfd,(struct sockaddr*)&addr_s, sizeof(addr_s))<0)
	{
		perror("bind");
		exit(EXIT_FAILURE);
	}

	puts("binding success");
        
        puts("listening....");
	
	spawn_handlers(2);
	
	if(listen(sfd,10000)<0)
	{
		perror("listen");
		exit(EXIT_FAILURE);
	}
	int pollfd;
	if((pollfd=epoll_create(10))<0)
	{
		perror("epoll_create1");
		exit(EXIT_FAILURE);
	}
	struct epoll_event ev;
	ev.events=EPOLLIN;
	ev.data.fd=sfd;
	if(epoll_ctl(pollfd,EPOLL_CTL_ADD,sfd,&ev)<0)
	{
		perror("epoll_ctl");
		exit(EXIT_FAILURE);
	}
	struct sockaddr caddr_s;
	socklen_t addrlen=0;
	struct epoll_event clev[MAX_EVENTS];
	int nfds=0;
	int cfd;
	while(1)
	{	
		// puts("waiting for client.......");
		if((nfds=epoll_wait(pollfd,clev,MAX_EVENTS,-1))<0)
		{
			perror("epoll_wait");
			exit(EXIT_FAILURE);
		}
		//if(nfds==0)
		//continue;
		for(int i=0;i<nfds;i++)
		{
			if((clev[i].events & EPOLLIN) == EPOLLIN)
			{	
				memset(&caddr_s,0,sizeof(struct sockaddr_in));
				addrlen=0;
				if(clev[i].data.fd == sfd)
				{
					if((cfd=accept(sfd, &caddr_s, &addrlen))<0)
					{	
						perror("epoll_wait");
						exit(EXIT_FAILURE);
					}
					ev.events=EPOLLIN|EPOLLET;
					ev.data.fd=cfd;	
					printf("connection established to peer:\nip:%s\nport:%u\n",inet_ntoa(addr_s.sin_addr),ntohs(addr_s.sin_port));
					if(epoll_ctl(pollfd,EPOLL_CTL_ADD,cfd,&ev)<0)
					{	
						perror("epoll_ctl");
						exit(EXIT_FAILURE);
					}
				}
				else
				{
					char buf[256]={'\0'};
					int rc;
					if((rc=recv(clev[i].data.fd, buf, 256, 0))<0)
					{
						perror("recv");
						exit(EXIT_FAILURE);
					}
					if(rc == 0)
					{
						printf("client closed the connection abruptly\n");
						if(epoll_ctl(pollfd,EPOLL_CTL_DEL,cfd,NULL)<0)
						{	
							perror("epoll_ctl");
							exit(EXIT_FAILURE);
						}
						close(clev[i].data.fd);
					}
					else{
						printf("received:%s\n",buf);	
						callthread(fd);
						char sendbuf[1024]={0};
						FILE* fp; 
						fp=popen(buf,"r");
						//fscanf(fp,"%79[^ \n]%s",sendbuf);
						fread(sendbuf,1,1024,fp);
						printf("sendbuf:%s\n",sendbuf);	
						if(send(clev[i].data.fd, sendbuf, strlen(sendbuf), 0)<0)
						{
							perror("send");
							exit(EXIT_FAILURE);
						}
						/*
							if(epoll_ctl(pollfd,EPOLL_CTL_DEL,cfd,NULL)<0)
							{	
								perror("epoll_ctl");
								exit(EXIT_FAILURE);
							}
							close(cfd);

							printf("closed the client\n");*/
					}
				}
			}
		}	
	}
exit(EXIT_SUCCESS);	
}

int serve(int clientid)
{
		

}

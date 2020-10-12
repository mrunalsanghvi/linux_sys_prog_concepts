#include"types.h"
#define IP_ADDR 1
#define PORT_NO 2
#define MAX_EVENTS 100
#define MAX_THREAD 50
pthread_mutex_t mutex;
pthread_cond_t condition=PTHREAD_COND_INITIALIZER;

int pollfd;
typedef struct msgbuf{
	
	long mtype;
	struct epoll_event event;

}mbuf_s;
int msgq_init(char*);
int msgq_init(char *filename)
{
	key_t ipc_key;
	int id;
	struct msqid_ds mqattr;
	if((ipc_key=ftok(filename, 0777|IPC_CREAT))==-1)
	{
		perror("ftok");
		return ipc_key;
	}
	if((id=msgget(ipc_key, IPC_CREAT))<0)	
	{
		perror("msgget");
		return id;
	}
	mqattr.msg_perm.mode=0777;
	if(msgctl(id,IPC_SET,&mqattr)<0)
	{
		perror("msgctl");
		exit(EXIT_FAILURE);
	}
	printf("mqid:%d\n",id);
	return id;
}
void *request_handler(void*args)
{
	int mqid = *(int*)args;
	struct msgbuf msg;
	printf("mqid:%d\n",mqid);
	while(1)
	{
		//while(pthread_mutex_trylock(&mutex)==0);
		pthread_mutex_lock(&mutex);
		if((msgrcv(mqid,&msg,sizeof(msg),0,IPC_NOWAIT)<0)&&(errno==ENOMSG))
		{
			//printf("nomessage on queue\n");
			//continue;
			pthread_cond_wait(&condition,&mutex);
			msgrcv(mqid,&msg,sizeof(msg),0,IPC_NOWAIT);
		}
		pthread_mutex_unlock(&mutex);
		char buf[256]={'\0'};
		int rc;
		if((rc=recv(msg.event.data.fd, buf, 256, 0))<0)
		{
			perror("recv");
			exit(EXIT_FAILURE);
		}
		if(rc == 0)
		{
			printf("client closed the connection abruptly\n");
			if(epoll_ctl(pollfd,EPOLL_CTL_DEL,msg.event.data.fd,NULL)<0)
			{	
				perror("epoll_ctl");
				exit(EXIT_FAILURE);
			}
			close(msg.event.data.fd);
		}
		else
		{
			printf("received:%s\n",buf);	
			char sendbuf[256]={0};
			FILE* fp; 
			fp=popen(buf,"r");
			//fscanf(fp,"%79[^ \n]%s",sendbuf);
			fread(sendbuf,1,256,fp);
			printf("sendbuf:%s\n",sendbuf);
			//sleep(1);	
			if(send(msg.event.data.fd, sendbuf, strlen(sendbuf), 0)<0)
			{
				if(errno==EBADF)
				{
					printf("Bad file descriptor, connections timedout client closed abruptly\n");
					if(epoll_ctl(pollfd,EPOLL_CTL_DEL,msg.event.data.fd,NULL)<0)
					{	
						perror("epoll_ctl");
						exit(EXIT_FAILURE);
					}
					close(msg.event.data.fd);
				}
				else
				{
					perror("send");
					exit(EXIT_FAILURE);
				}
			}
			if(epoll_ctl(pollfd,EPOLL_CTL_DEL,msg.event.data.fd,NULL)<0)
					{	
						perror("epoll_ctl");
						exit(EXIT_FAILURE);
					}
					close(msg.event.data.fd);

		}
	}	
}
int threadpool_init(int mqid)
{	
	pthread_t tid[MAX_THREAD];
	pthread_mutex_init(&mutex, NULL);
	for(int i=0; i<MAX_THREAD ; i++)
	{
		pthread_create(&tid[i], NULL, request_handler, (void*)&mqid);
	}
	return 0;
}

int add_event_to_q(int mqid,struct epoll_event *event)
{
	struct msgbuf msg;
	msg.mtype=1;
	memcpy(&msg.event, event ,sizeof(struct epoll_event));
	if(msgsnd(mqid, &msg, sizeof(msg),0)<0)
	return -1;
	else
	return 0;
}

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
	
	int mqid;
	mqid=msgq_init("/home/mrunal/sockets/data/ARB123.txt");
	printf("mqid:%d\n",mqid);
	if(mqid < 0)
	{
		perror("msgq_init");
		exit(EXIT_FAILURE);
	}
	printf("message_queue init success...\n");
	if(threadpool_init(mqid)<0)	
	{		
		printf("threadpool_init failed");
		exit(EXIT_FAILURE);
	}
	if(listen(sfd,10000)<0)
	{
		perror("listen");
		exit(EXIT_FAILURE);
	}
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
					pthread_mutex_lock(&mutex);
					if(add_event_to_q(mqid, clev+i)<0)
					{
						perror("add_event_to_q");
					}
					pthread_cond_signal(&condition);
					pthread_mutex_unlock(&mutex);
					/*
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


					}*/
				}
			}
		}	
	}
return 0;	
}


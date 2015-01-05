#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/epoll.h>

#include <time.h>
#include <unistd.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAXSIZE 1024
#define IPADDRESS "127.0.0.1"
#define SERV_PORT 9999
#define FDSIZE 1024
#define EPOLLEVENTS 20

static void handle_connection(int sockfd);
static void handle_events(	int epollfd,
							struct epoll_event* events,
							int maxevents,
							int sockfd,
							char* buf);
static void do_read(int epollfd, int fd, int sockfd, char* buf);
static void do_write(int epollfd, int fd, int sockfd, char* buf);
static void add_event(int epollfd, int fd, int event);
static void delete_event(int epollfd, int fd, int event);
static void modify_event(int epollfd, int fd, int event);

int main(int argc, char *argv[])
{
	int sockfd;
	struct sockaddr_in servaddr;
	bzero(&servaddr,sizeof(struct sockaddr_in));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(SERV_PORT);
	inet_pton(AF_INET,IPADDRESS,&servaddr.sin_addr);
	sockfd = socket(AF_INET, SOCK_STREAM,0);
	if( connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr) )== -1){
		perror("connect error:");
		return -1;
	}
	handle_connection(sockfd);
	close(sockfd);

	return 0;
}

static void handle_connection(int sockfd)
{
	int epollfd;
	struct epoll_event events[EPOLLEVENTS];
	char buf[MAXSIZE];
	int ret;
	epollfd = epoll_create(FDSIZE); 
	add_event(epollfd,STDIN_FILENO,EPOLLIN);
	for(;;){
		ret = epoll_wait(epollfd, events,EPOLLEVENTS,-1);
		handle_events(epollfd,events,ret,sockfd,buf);
	}
	close(sockfd);
}
static void handle_events(int epollfd,struct epoll_event* events,int maxevents,int sockfd,char* buf)
{
	int fd;
	int i;
	for(i = 0; i < maxevents; i++){
		fd = events[i].data.fd;
		if( events[i].events & EPOLLIN)
			do_read(epollfd,fd,sockfd,buf);
		else if(events[i].events & EPOLLOUT)
			do_write(epollfd,fd,sockfd,buf);
	}
}

static void do_read(int epollfd,int fd, int sockfd, char* buf)
{
	int nread;
	nread = read(fd, buf ,MAXSIZE);
	if(nread == -1){
		perror("read error:");
		close(fd);
	}
	else if(nread == 0){
		fprintf(stderr,"server close.\n");
		close(fd);
	}
	else{
		if( fd == STDIN_FILENO)
			add_event(epollfd,sockfd,EPOLLOUT);
		else{
			delete_event(epollfd,sockfd,EPOLLIN);
			add_event(epollfd,STDOUT_FILENO,EPOLLOUT);
		}
	}
}
static void do_write(int epollfd, int fd,int sockfd,char* buf)
{
	int nwrite;
	nwrite = write(fd, buf,strlen(buf));
	if(nwrite == -1)
	{
		perror("write error:");
		close(fd);
	}
	else{
		if( fd == STDOUT_FILENO)
			delete_event(epollfd,fd, EPOLLOUT);
		else
			modify_event(epollfd,fd,EPOLLIN);
	}
	memset(buf,0,MAXSIZE);
}
static void add_event(int epollfd, int fd, int event)
{
	struct epoll_event ev;
	ev.events = event;
	ev.data.fd = fd;
	epoll_ctl(epollfd, EPOLL_CTL_ADD,fd,&ev);
}
static void delete_event(int epollfd, int fd, int event)
{
	struct epoll_event ev;
	ev.events = event;
	ev.data.fd = fd;
	epoll_ctl(epollfd, EPOLL_CTL_DEL,fd,&ev);
}
static void modify_event(int epollfd, int fd, int event)
{
	struct epoll_event ev;
	ev.events = event;
	ev.data.fd = fd;
	epoll_ctl(epollfd, EPOLL_CTL_MOD,fd,&ev);
}
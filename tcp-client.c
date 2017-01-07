#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#define BUFFER_LENGTH 640*512*3+1+40+14
#define BUFFER_RECV 40000

int main(int argc, char *argv[]) {
	struct sockaddr_in server_addr;
	int connfd;
	char buf[BUFFER_LENGTH],sbuf[BUFFER_RECV];
	int n=0,m=0;
    FILE *fp;
    int i;
    char *filename = "2new.bmp ";
	if (argc != 3){
		fputs("usage: ./udpserver serverIP serverPort\n", stderr);
		exit(1);
	}
	connfd = socket(AF_INET, SOCK_STREAM, 0);//建立套接字
	bzero(&server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(atoi(argv[2]));
	if (inet_aton(argv[1], &server_addr.sin_addr) == 0){
		perror("inet_aton");
		exit(1);
	}
	if (connect(connfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0){
		perror("connect");
		exit(1);
	}//发送连接请求
	fp=fopen( filename, "wb");
        if(fp == NULL)
        {
            printf( "File open error!\n ");
            exit(1);
        }

	while(1){
		n = recv(connfd, sbuf, BUFFER_RECV, 0);
		m += n;
		printf("%d\n", n);
		if (n == -1){
			perror("fail to receive");
			exit(1);
		}
		else if(n == 0){
			break;
		}
		else if(m<BUFFER_LENGTH){//建立连接创建新的套接字
			memcpy(buf+m-n,sbuf,n);
			printf("%s\n", buf); 
		}
		else break;
	}
	fwrite(buf,1,BUFFER_LENGTH,fp);
	if (close(connfd) == -1){//关闭连接套接字
		perror("fail to close");
		exit(1);
	}
	fclose(fp);
	puts("TCP client is closed!\n");
	return 0;
	}

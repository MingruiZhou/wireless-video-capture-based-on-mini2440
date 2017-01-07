#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>
#include <netdb.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/sendfile.h>
#include <fcntl.h>
#define BUFFER_LENGTH 640*512*2
#define BUFFER_LENGTH_BMP 640*512*3+40+14
#define BUFFER_SEND 40000

//length of buf
#define SERV_PORT 3002

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#pragma pack(1)                           //½á¹¹ÌåÒª°´×Ö½Ú¶ÔÆë
typedef struct __BITMAPFILEHEADER__
{
    u_int16_t bfType;
    u_int32_t bfSize;
    u_int16_t bfReserved1;
    u_int16_t bfReserved2;
    u_int32_t bfOffBits;
}BITMAPFILEHEADER; 

typedef struct __BITMAPINFOHEADER 
{ 
    u_int32_t biSize; 
    u_int32_t biWidth; 
    u_int32_t biHeight; 
    u_int16_t biPlanes; 
    u_int16_t biBitCount; 
    u_int32_t biCompression; 
    u_int32_t biSizeImage; 
    u_int32_t biXPelsPerMeter; 
    u_int32_t biYPelsPerMeter; 
    u_int32_t biClrUsed; 
    u_int32_t biClrImportant; 
}BITMAPINFOHEADER; 

#define BYTE_PER_PIX 2
#define BYTE_PER_PIX_BMP 3

int create_bmp_header(BITMAPFILEHEADER* bmphead, BITMAPINFOHEADER* infohead, int w, int h)
{
    bmphead->bfType = 0x4D42; //must be 0x4D42='BM'
    bmphead->bfSize= w*h*BYTE_PER_PIX_BMP+14+40;
    bmphead->bfReserved1= 0x00;
    bmphead->bfReserved2= 0x00;
    bmphead->bfOffBits = 14+40;

    infohead->biSize = sizeof(BITMAPINFOHEADER);
    infohead->biWidth = w; 
    infohead->biHeight = -h;    //ÌØ±ð×¢Òâ£ºÒý×Ô²Î¿¼ÎÄÕÂ"BMPÍ¼Æ¬´Ó×îºóÒ»¸öµã¿ªÊ¼É¨Ãè£¬ÏÔÊ¾Ê±Í¼Æ¬ÊÇµ¹×ÅµÄ£¬ËùÒÔÓÃ-height£¬ÕâÑùÍ¼Æ¬¾ÍÕýÁË"
    infohead->biPlanes = 1;
    infohead->biBitCount = BYTE_PER_PIX_BMP*8;
    infohead->biCompression= 0;
    infohead->biSizeImage= w*h*BYTE_PER_PIX_BMP;//640 * 480 * 3;
    infohead->biXPelsPerMeter= 0x0;
    infohead->biYPelsPerMeter= 0x0;
    infohead->biClrUsed = 0;
    infohead->biClrImportant = 0;
    return 0;
}

int rawTobmp(char *pbuf, char *pRGBbuf)
{
    int w = 640;
    int h = 512;
    
    int i,k,x=0;

    BITMAPFILEHEADER bmphead;
    BITMAPINFOHEADER infohead;
    
    //pRGBbuf = (char*)malloc(w*h*BYTE_PER_PIX_BMP*sizeof(char));
	//prepare for bmp write 

    create_bmp_header(&bmphead, &infohead, w, h);
	memcpy(pRGBbuf,&bmphead,14);
	memcpy(pRGBbuf+14,&infohead,40);
    
    for(i=0,k=54; i<w*h*BYTE_PER_PIX && k<w*h*BYTE_PER_PIX_BMP+54; i+=BYTE_PER_PIX,k+=BYTE_PER_PIX_BMP)  //ÌØ±ð×¢Òâ£¬Õâ¶ùµÄBGR×ªRGB
    {
		pRGBbuf[k] = pbuf[i]<<3;
		pRGBbuf[k+1] = (pbuf[i+1]<<5)|((pbuf[i]&0xe0)>>3);
		pRGBbuf[k+2] = pbuf[i+1]&0xf8;

		pRGBbuf[k+2] = pRGBbuf[k+2]|((pRGBbuf[k+2]&0x38)>>3);
		pRGBbuf[k+1] = pRGBbuf[k+1]|((pRGBbuf[k+1]&0x0c)>>2);
		pRGBbuf[k] = pRGBbuf[k]|((pRGBbuf[k]&0x38)>>3);
		x++;
		
    }
    printf("%d",x);
    return EXIT_SUCCESS;
}

int main(int argc,char* argv[]){
	struct sockaddr_in server_addr,client_addr;
	int listenfd,connfd;
	socklen_t client_addr_len = sizeof(struct sockaddr);
	char buf[BUFFER_LENGTH] = {};
	char buf2[BUFFER_LENGTH_BMP] = {};
	char *filename = "1.raw";
	int n=0,m=0;
        FILE* pFile, *pFile2;  
        pFile = fopen("1.raw" , "rb"); // 重新打开文件读操作
        fread(buf , 1 , BUFFER_LENGTH , pFile); // 从文件中读数据  
        rawTobmp(buf,buf2);
        printf("%s\n", buf); 
        printf("%s\n",buf2); 
        pFile2 = fopen("2.bmp" , "wb"); 
        fwrite(buf2,1,BUFFER_LENGTH_BMP,pFile2);
       	fclose(pFile2);
        fclose(pFile); // 关闭文件   
	listenfd = socket(AF_INET,SOCK_STREAM,0);
	//create ipv4 listen socket
	if(listenfd == -1){
		perror("created TCP socket");
		exit(1);
	}
	bzero(&server_addr,sizeof(server_addr));
	//init server_addr
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_port = htons(SERV_PORT);
	if(bind(listenfd,(struct sockaddr*)&server_addr,sizeof(server_addr)) == -1){
		perror("call to bind");
		exit(1);
	}//绑定监听套接字和服务器地址结构
	listen(listenfd,2);//监听
	connfd = accept(listenfd,(struct sockaddr*)&client_addr,&client_addr_len);//建立连接创建新的套接字
	if(connfd<0){
		perror("accept");
		exit(1);
		}
	
	// fq = open(filename , O_RDONLY);

	while(1){
		printf("%s",buf2);
		n = send(connfd,buf2+m,BUFFER_SEND,0);
		m += n;
		if(n == -1){
			perror("fail to reply");
			exit(1);
		}
		if(m>=BUFFER_LENGTH_BMP) break;
		// sleep(10);
		// if( fq < 0 )
  //   	{
  //       perror("file error");
  //       exit(1);
  //   	}
  //   	stat(filename,&st);//获取文件大小
  //   	len = st.st_size;

  //   	if(sendfile(connfd,fq,0,len) < 0)
  //   	{
  //       perror("send error");
  //       exit(1);
  //   	}
	}
	if(close(listenfd)==-1){//关闭监听
		perror("fail to close");
		exit(1);
	}
	// close(fq);
	puts("TCP server is closed!\n");
	return 0;
}

//encoding = "utf-8"
#include <iostream>
#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <linux/types.h>
#include <linux/fb.h>
#include <fstream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>
#include <math.h>
#pragma pack(1) 

#define SERV_PORT 3002
#define BUFFER_LENGTH_BMP 640*512*3+40+14
#define BUFFER_SEND 40000

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

class TError {
public:
	TError(const char *msg) {
		this->msg = msg;
	}
	TError(const TError & e) {
		msg = e.msg;
	}
	void Output() {
		std::cerr << msg << std::endl;
	}
	virtual ~TError() {}
protected:
	TError & operator=(const TError &);
private:
	const char *msg;
};

// Linear memory based image
class TRect {
public:
	TRect():  Addr(0), Size(0), Width(0), Height(0), LineLen(0), BPP(16) {
	}
	virtual ~TRect() {
	}
	bool DrawRect(const TRect & SrcRect, int x, int y) const {
		if (BPP != 16 or SrcRect.BPP != 16) {
			// don't support that yet
			throw TError("does not support other than 16 BPP yet");
		}

		// clip
		int x0, y0, x1, y1;
		x0 = x;
		y0 = y;
		x1 = x0 + SrcRect.Width - 1;
		y1 = y0 + SrcRect.Height - 1;
		if (x0 < 0) {
			x0 = 0;
		}
		if (x0 > Width - 1) {
			return true;
		}
		if( x1 < 0) {
			return true;
		}
		if (x1 > Width - 1) {
			x1 = Width - 1;
		}
		if (y0 < 0) {
			y0 = 0;
		}
		if (y0 > Height - 1) {
			return true;
		}
		if (y1 < 0) {
			return true;
		}
		if (y1 > Height - 1) {
			y1 = Height -1;
		}

		//copy
		int copyLineLen = (x1 + 1 - x0) * BPP / 8;
		unsigned char *DstPtr = Addr + LineLen * y0 + x0 * BPP / 8;
		const unsigned char *SrcPtr = SrcRect.Addr + SrcRect.LineLen *(y0 - y) + (x0 - x) * SrcRect.BPP / 8;

		for (int i = y0; i <= y1; i++) {
			memcpy(DstPtr, SrcPtr, copyLineLen);
			DstPtr += LineLen;
			SrcPtr += SrcRect.LineLen;
		}
		
		
		return true;
	}

	bool DrawRect(const TRect & rect) const { // default is Center
		return DrawRect(rect, (Width - rect.Width) / 2, (Height - rect.Height) / 2);
	}

	bool Clear() const {
		int i;
		unsigned char *ptr;
		for (i = 0, ptr = Addr; i < Height; i++, ptr += LineLen) {
			memset(ptr, 0, Width * BPP / 8);
		}
		return true;
	}

protected:
	TRect(const TRect &);
	TRect & operator=( const TRect &);

protected:
	int Size;
	unsigned char *Addr;
	int Width, Height, LineLen;
	unsigned BPP;
};



class TFrameBuffer: public TRect {
public:
	TFrameBuffer(const char *DeviceName = "/dev/fb0"): TRect(), fd(-1) {
		Addr = (unsigned char *)MAP_FAILED;

        	fd = open(DeviceName, O_RDWR);
		if (fd < 0) {
			throw TError("cannot open frame buffer");
		}

        	struct fb_fix_screeninfo Fix;
        	struct fb_var_screeninfo Var;
		if (ioctl(fd, FBIOGET_FSCREENINFO, & Fix) < 0 or ioctl(fd, FBIOGET_VSCREENINFO, & Var) < 0) {
			throw TError("cannot get frame buffer information");
		}

		BPP = Var.bits_per_pixel;
	        if (BPP != 16) {
			throw TError("support 16BPP frame buffer only");
		}

        	Width  = Var.xres;
        	Height = Var.yres;
        	LineLen = Fix.line_length;
      		Size = LineLen * Height;

		int PageSize = getpagesize();
		Size = (Size + PageSize - 1) / PageSize * PageSize ;
	        Addr = (unsigned char *)mmap(NULL, Size, PROT_READ|PROT_WRITE,MAP_SHARED, fd, 0);
		if (Addr == (unsigned char *)MAP_FAILED) {
			throw TError("map frame buffer failed");
			return;
		}
		::close(fd);
		fd = -1;

		Clear();
	}

	virtual ~TFrameBuffer() {
		::munmap(Addr, Size);
		Addr = (unsigned char *)MAP_FAILED;

		::close(fd);
		fd = -1;
	}

protected:
	TFrameBuffer(const TFrameBuffer &);
	TFrameBuffer & operator=( const TFrameBuffer &);
private:
	int fd;
};

class TVideo : public TRect {
public:
	TVideo(const char *DeviceName = "/dev/camera"): TRect(), fd(-1) {
		Addr = 0;
		fd = ::open(DeviceName, O_RDONLY);
		if (fd < 0) {
			throw TError("cannot open video device");
		}
		Width = 640;
		Height = 512;
		BPP = 16;
		LineLen = Width * BPP / 8;
		Size = LineLen * Height;
		Addr = new unsigned char<:Size:>;

		Clear();
	}

	bool FetchPicture() const {
		int count = ::read(fd, Addr, Size);
		if (count != Size) {
			throw TError("error in fetching picture from video");
		}
		// char *buffer[10]; 
		// std::cout << Size << std::endl;
		// FILE* fp;
		// char *fname = "/1.raw";
		// fp = fopen(fname,"wb+");
		// //std::cout << sizeof(Addr) << std::endl;
		// if(fp == NULL)
		// {
		// 	return false;
		// }
		// fwrite(Addr,sizeof(char),Size,fp);
		// fclose(fp);
		printf("%s\n", Video.Addr);

		//change raw to bmp
		if(rawTobmp((char*)Video.Addr, buf)==1){
			perror("raw to bmp");
		}
		while(1){
			n = send(connfd,buf+m,BUFFER_SEND,0);
			m += n;
			if(n == -1){
				perror("fail to reply");
				exit(1);
			}
			if(m>=BUFFER_LENGTH_BMP) break;
		}
		return true;
	}

	virtual ~TVideo() {
		::close(fd);
		fd = -1;
		delete<::> Addr;
		Addr = 0;
	}

protected:
	TVideo(const TVideo &);
	TVideo & operator=(const TVideo &);

private:
	int fd;
};

int main(int argc, char **argv)
{
	struct sockaddr_in server_addr,client_addr;
	int listenfd,connfd;
	int n=0,m=0;
	socklen_t client_addr_len = sizeof(struct sockaddr);
	char buf[BUFFER_LENGTH_BMP] = {};

	//create listenfd
	listenfd = socket(AF_INET,SOCK_STREAM,0);
	if(listenfd == -1){
		perror("created TCP socket");
		exit(1);
	}
	//server_addr modify
	bzero(&server_addr,sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_port = htons(SERV_PORT);

	//bind
	if(bind(listenfd,(struct sockaddr*)&server_addr,sizeof(server_addr)) == -1){
		perror("call to bind");
		exit(1);
	}

	//listen
	listen(listenfd,2);//监听

	//accept
	connfd = accept(listenfd,(struct sockaddr*)&client_addr,&client_addr_len);//建立连接创建新的套接字
	if(connfd<0){
		perror("accept");
		exit(1);
	}

	//do capture
	try {
		TFrameBuffer FrameBuffer;
		TVideo Video;
		// for (;;) {
		Video.FetchPicture();
		FrameBuffer.DrawRect(Video);
		// }
		// printf("%s\n", Video.Addr);

		// //change raw to bmp
		// if(rawTobmp((char*)Video.Addr, buf)==1){
		// 	perror("raw to bmp");
		// }
		// while(1){
		// 	n = send(connfd,buf+m,BUFFER_SEND,0);
		// 	m += n;
		// 	if(n == -1){
		// 		perror("fail to reply");
		// 		exit(1);
		// 	}
		// 	if(m>=BUFFER_LENGTH_BMP) break;
		// }

		if(close(listenfd)==-1 || close(connfd)==-1){//关闭监听
			perror("fail to close");
			exit(1);
		}
	} catch (TError & e) {
		e.Output();
		return 1;
	}
	return 0;
}

#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/types.h>
#include <math.h>
#include <fcntl.h>
#include <signal.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <sys/time.h>
#include <time.h>

#define SYSTEM_EYE_DEV_NAME  "/dev/st7789v"
#define SYSTEM_BOOTIMG1_PATH "/home/pi/booting1"
#define SYSTEM_BOOTIMG2_PATH "/home/pi/booting2"
#define SYSTEM_TESTBMP_PATH  "/home/pi/test.bmp"

#define LCD_PIXS 240*240
unsigned char gLogoBuf[LCD_PIXS*2 + 1] = {0};

typedef int64_t nsecs_t;

static  inline nsecs_t seconds_to_nanoseconds(nsecs_t secs)
{
    return secs*1000000000;
}

static  inline nsecs_t milliseconds_to_nanoseconds(nsecs_t secs)
{
    return secs*1000000;
}

static  inline nsecs_t microseconds_to_nanoseconds(nsecs_t secs)
{
    return secs*1000;
}

static  inline nsecs_t nanoseconds_to_seconds(nsecs_t secs)
{
    return secs/1000000000;
}

static  inline nsecs_t nanoseconds_to_milliseconds(nsecs_t secs)
{
    return secs/1000000;
}

static  inline nsecs_t nanoseconds_to_microseconds(nsecs_t secs)
{
    return secs/1000;
}

static  inline nsecs_t s2ns(nsecs_t v)  {return seconds_to_nanoseconds(v);}
static  inline nsecs_t ms2ns(nsecs_t v) {return milliseconds_to_nanoseconds(v);}
static  inline nsecs_t us2ns(nsecs_t v) {return microseconds_to_nanoseconds(v);}
static  inline nsecs_t ns2s(nsecs_t v)  {return nanoseconds_to_seconds(v);}
static  inline nsecs_t ns2ms(nsecs_t v) {return nanoseconds_to_milliseconds(v);}
static  inline nsecs_t ns2us(nsecs_t v) {return nanoseconds_to_microseconds(v);}
nsecs_t systemTime(){
    static const clockid_t clocks[] = {
        CLOCK_REALTIME,
        CLOCK_MONOTONIC,
        CLOCK_PROCESS_CPUTIME_ID,
        CLOCK_THREAD_CPUTIME_ID,
        CLOCK_BOOTTIME
    };
    struct timespec t;
    t.tv_sec = t.tv_nsec = 0;
    clock_gettime(clocks[CLOCK_BOOTTIME], &t);
    return (nsecs_t)(t.tv_sec)*1000000000LL + t.tv_nsec;
}


//24位深的bmp图片转换为16位深RGB565格式的bmp图片
struct RGB_Data{
    unsigned short B_data:5;
    unsigned short G_data:6;
    unsigned short R_data:5;
}lcd_img[LCD_PIXS]; //img size must less than 1024*768

typedef struct{
	unsigned char  bfType[2];				    //文件类型
	unsigned long  bfSize;					    //位图大小
	unsigned short bfReserved1;				    //位0
	unsigned short bfReserved2;				    //位0
	unsigned long  bfOffBits;				    //到数据偏移量
} __attribute__((packed)) BitMapFileHeader;     //使编译器不优化，其大小为14字节
//信息头结构体
typedef struct{
	unsigned long biSize;						//BitMapFileHeader 字节数
	long biWidth;								//位图宽度
	long biHeight;								//位图高度，正位正向，反之为倒图
	unsigned short biPlanes;					//为目标设备说明位面数，其值将总是被设为1
	unsigned short biBitCount;					//说明比特数/象素，为1、4、8、16、24、或32。
	unsigned long biCompression;				//图象数据压缩的类型没有压缩的类型：BI_RGB
	unsigned long biSizeImage;					//说明图象的大小，以字节为单位
	long biXPelsPerMeter;						//说明水平分辨率
	long biYPelsPerMeter;						//说明垂直分辨率
	unsigned long biClrUsed;					//说明位图实际使用的彩色表中的颜色索引数
	unsigned long biClrImportant;				//对图象显示有重要影响的索引数，0都重要。
} __attribute__((packed)) BitMapInfoHeader;

//像素点结构体
typedef struct{
	unsigned char Blue;			//该颜色的蓝色分量
	unsigned char Green;		//该颜色的绿色分量
	unsigned char Red;			//该颜色的红色分量
	unsigned char Reserved;		//保留值（亮度）
} __attribute__((packed)) RgbQuad;

int show_photo(const char *bmpname){
	BitMapFileHeader FileHead;
	BitMapInfoHeader InfoHead;
	RgbQuad rgb;
	char* pdata = NULL;
    char* pTemp = NULL;
	int fd_dev  = 0;
	int fb = NULL;
	int len = 0;
    int i = 0, j = 0;
	
    if(NULL == bmpname){
        return -1;
    }
    fd_dev = open(SYSTEM_EYE_DEV_NAME,O_WRONLY,0777);
    if (fd_dev < 0){
        printf("open st7789v error \n");
		goto EXIT1;
    }

	//打开.bmp文件
	fb = open(bmpname, O_RDONLY);
	if( fb <= 0 ){
		printf("open bmp error\r\n");
		goto EXIT1;
	}

	//读文件信息
	if(sizeof(BitMapFileHeader) != read(fb, &FileHead, sizeof(BitMapFileHeader))){
		printf("read BitMapFileHeader error!\n");
		goto EXIT1;
	}else{
        printf("################################################################################\n");
        printf("bmp header info:\n");
        printf("type:%c%c\n", FileHead.bfType[0], FileHead.bfType[1]);
        printf("bfOffBits:%lu\n", FileHead.bfOffBits);
        printf("################################################################################\n");
    }
	if( memcmp(FileHead.bfType, "BM", 2) != 0){
		printf("it's not a BMP file\n");
		goto EXIT1;
	}
	printf("################################################################################\n");
    printf("headsize: FileHeader: %d, InfoHeader: %d\n", sizeof(BitMapFileHeader), sizeof(BitMapInfoHeader));
    printf("################################################################################\n");
	//读位图信息
	if( sizeof(BitMapInfoHeader) != read(fb, (char *)&InfoHead, sizeof(BitMapInfoHeader))){
		printf("read BitMapInfoHeader error!\n");
		goto EXIT1;
	}else{
        printf("################################################################################\n");
        printf("InfoHeader:biSize:%lu,w:%lu,h:%lu, biBitCount:%d\n",InfoHead.biSize, InfoHead.biWidth,InfoHead.biHeight,InfoHead.biBitCount);
        printf("InfoHeader:biSizeImage:%lu,biX:%lu,biY:%lu\n",InfoHead.biSizeImage, InfoHead.biXPelsPerMeter,InfoHead.biYPelsPerMeter);
        printf("################################################################################\n");
    }
	
	//跳转至数据区
	lseek(fb, FileHead.bfOffBits, SEEK_SET);
	len = InfoHead.biBitCount / 8;			//原图一个像素占几字节
    pdata = malloc(InfoHead.biSizeImage);
    if(pdata == NULL){
        printf("no memory\n");
        goto EXIT1;
    }
	if (InfoHead.biSizeImage != read(fb, pdata, InfoHead.biSizeImage)){
		printf("read BitMapInfoHeader error!\n");
		goto EXIT1;
	}
    #if 0
    for(i=0;i<InfoHead.biSizeImage/len;i++){
        lcd_img[i].B_data = pdata[0]>>2;
        lcd_img[i].G_data = pdata[1]>>2;
        lcd_img[i].R_data = pdata[2]>>2;
        pdata+=len;
	}
    #else
    pTemp = pdata + InfoHead.biSizeImage-1-2;
	j=0;
    for(i=0;i<LCD_PIXS;i++){
        //lcd_img[LCD_PIXS-1-i].B_data = pdata[InfoHead.biWidth-3-i%InfoHead.biWidth]>>2;
        //lcd_img[LCD_PIXS-1-i].G_data = pdata[InfoHead.biWidth-2-i%InfoHead.biWidth]>>2;
        //lcd_img[LCD_PIXS-1-i].R_data = pdata[InfoHead.biWidth-1-i%InfoHead.biWidth]>>2;
        lcd_img[j+239-i%240].B_data = pTemp[0]>>3;
        lcd_img[j+239-i%240].G_data = pTemp[1]>>2;
        lcd_img[j+239-i%240].R_data = pTemp[2]>>3;
        pTemp-=len;
		j=i/240*240;
	}
    #endif
	//close(fb);
    memcpy(gLogoBuf,(char*)lcd_img, 240*240*2);
    gLogoBuf[240*240*2] = 2;
    write(fd_dev, gLogoBuf, sizeof(gLogoBuf));
EXIT1:
    if(fb > 0){
        close(fb);
    }
    if(pdata != NULL){
        free(pdata);
    }
    if(fd_dev != 0){
        close(fd_dev);
	}
	return 0;
}

int do_test(void){
	int fd_dev  = 0;
    int fd_booting1 = 0, fd_booting2 = 0;
    int i = 0,j = 0;
	nsecs_t sleepTime = 0;
	nsecs_t startTime = 0;
	nsecs_t duration = 0;
	printf("boot hello0 start at %lldus",ns2us(systemTime()));

	fd_booting1 = open(SYSTEM_BOOTIMG1_PATH,O_RDONLY,0777);
	if(fd_booting1 < 0){
		printf("open logo1 error \n");
		goto EXIT;
	}
   	fd_booting2 = open(SYSTEM_BOOTIMG2_PATH,O_RDONLY,0777);
    if(fd_booting2 < 0){
		printf("open logo2 error \n");
		goto EXIT;
	}
	fd_dev = open(SYSTEM_EYE_DEV_NAME,O_WRONLY,0777);
    if (fd_dev < 0){
        printf("open st7789v error \n");
		goto EXIT;
    }
	printf("open fddev = %d, fd_booting1 = %d, fd_booting2 = %d \n",fd_dev, fd_booting1, fd_booting2);
        printf("start bootimg2 at %lldus",ns2us(systemTime()));
	for(i = 0; i < 44; i++){
        startTime = systemTime();    
        read(fd_booting1, gLogoBuf, sizeof(gLogoBuf));
        write(fd_dev, gLogoBuf, sizeof(gLogoBuf));
		//write(fd_dev, gLogoBuf, sizeof(gLogoBuf));
        duration = ns2us(systemTime()-startTime);

		//25fps: don't animate too fast to preserve CPU
    	sleepTime = 40000 - duration;
        if (sleepTime > 0){
    		usleep(sleepTime);
    	}
        printf("count:%d time:%lldus, sleepTime:%lldus\n", i, duration, sleepTime);
    }
    lseek(fd_booting1, 0, SEEK_SET);   
    printf("start bootimg2 at %lldus\n",ns2us(systemTime()));
    do {
        for(i = 0; i < 50; i++){
            startTime = systemTime();
	        read(fd_booting2, gLogoBuf, sizeof(gLogoBuf));
	        write(fd_dev, gLogoBuf, sizeof(gLogoBuf));
			//write(fd_dev, gLogoBuf, sizeof(gLogoBuf));
            duration = ns2us(systemTime()-startTime);
    	    printf("count:%d time:%lldus\n", i, duration);
            sleepTime = 40000 - duration;
            if (sleepTime > 0){
    			usleep(sleepTime);
    		}
        }
		lseek(fd_booting2, 0, SEEK_SET);
        printf("start bootimg2 circle %d at %lldus\n",j,ns2us(systemTime()));
        j++;
    } while (j < 5);
EXIT:
	if(fd_dev)close(fd_dev);
    if(fd_booting1)close(fd_booting1);
    if(fd_booting2)close(fd_booting2);    
    printf("bootaudio stop at %lldus",ns2us(systemTime()));
}


int main(int argc, char** argv){
    //do_test();
    show_photo(SYSTEM_TESTBMP_PATH);
    return 0;
}

/**
 * gif2lcd
 * hongbing.luo
 * 3/2/2021
 *
 * Program to convert a gif file to a raspberry device spi LCD
*
 * Thanks to: 
 *            
 */
#include <stdio.h>
#include <stdlib.h>
#include <png.h>
#include <string.h>

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
#include "gifdec.h"

#define RGB888toRGB565(r, g, b) ((r >> 3) << 11)| \
	((g >> 2) << 5)| ((b >> 3) << 0)

#define LCD_PIXS 240*240

#define SYSTEM_EYE_DEV_NAME  "/dev/st7789v"

struct RGB_Data{
    unsigned short B_data:5;
    unsigned short G_data:6;
    unsigned short R_data:5;
}lcd_img[LCD_PIXS]; //img size must less than 1024*768

int main(int argc, char* argv[]) {
    int fd_dev  = 0;
	
    // check parameter
    if (argc != 2 ) {
        printf("Usage: bin [GIF file]\n");
        return -1;
    }
    fd_dev = open(SYSTEM_EYE_DEV_NAME,O_WRONLY,0777);
    if (fd_dev < 0){
        printf("open st7789v error \n");
		return -1;
    }
    
    // open file
    printf("Opening file %s...\n", argv[1]);
    // char header[8];
    // int img_fp = open(argv[1], O_RDWR);
    // if (img_fp == -1) {
    //     printf("Could not open file %s.\n", argv[1]);
    //     return -1;
    // }

    gd_GIF *gif = gd_open_gif(argv[1]);
    if (!gif) {
        printf("gd_open_gif error.\n");
        return -1;
    }

    int res = 0;
    int size = gif->width * gif->height;

    uint8_t* screen = (uint8_t*)malloc(size*3);
    // unsigned char _gLogoBuf[LCD_PIXS*2 + 1] = {0};
    uint8_t* _gLogoBuf = (uint8_t*)malloc(LCD_PIXS*2+1);
    memset(_gLogoBuf, 0xff, LCD_PIXS*2+1);
    int i, j;
    printf("h=%d, w=%d\n", gif->height, gif->width);
    while(true){
        usleep(gif->gce.delay * 1000);
        res = gd_get_frame(gif);
        if (res < 0) {
            printf("failur\n");
        } else if (res == 0) {
            gd_rewind(gif);
            continue;
        }

        gd_render_frame(gif, screen);
        for (int y=0; y<gif->height; y++) {
            uint8_t* row = &screen[y*gif->width*3];
            for (int x=0; x<gif->width; x++) {
                uint8_t* pixel = &(row[x*3]);
                // unsigned short convPixel = RGB888toRGB565(pixel[0], pixel[1], pixel[2]);
                // printf("%04x ", convPixel);
                lcd_img[y*gif->width+x].R_data = pixel[0]>>3;
                lcd_img[y*gif->width+x].G_data = pixel[1]>>2;
                lcd_img[y*gif->width+x].B_data = pixel[2]>>3;
            }
        }
        // write(fd_dev, screen, size);
        // memcpy(_gLogoBuf,(unsigned char*)screen, size*2);
        memcpy(_gLogoBuf,(char*)lcd_img, LCD_PIXS*2);
        _gLogoBuf[LCD_PIXS*2] = 2;
        write(fd_dev, _gLogoBuf, LCD_PIXS*2+1);
        printf("------------> LCD, %d\n", gif->gce.delay);
    }

    free(screen);
}

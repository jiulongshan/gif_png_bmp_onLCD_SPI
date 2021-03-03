/**
 * png2c
 * Oleg Vaskevich, Northeastern University
 * 4/17/2013
 *
 * Program to convert a PNG file to an C header as an array
 * in either RGB565 or RGB5A1 format. This is useful for
 * embedding pixmaps to display with a PAL board or Arduino.
 *
 * Thanks to: http://zarb.org/~gc/html/libpng.html
 *            http://stackoverflow.com/a/2736821/832776
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

#define SYSTEM_EYE_DEV_NAME  "/dev/st7789v"

#define RGB888toRGB565(r, g, b) ((r >> 3) << 11)| \
	((g >> 2) << 5)| ((b >> 3) << 0)

#define RGB8888toRGB5A1(r, g, b, a) ((r >> 3) << 11)| \
	((g >> 3) << 6)| ((b >> 3) << 1) | (a == 0x0)


FILE *file;

#define LCD_PIXS 240*240

int x, y;

int width, height;
png_byte colorType;
png_byte bitDepth;

png_structp pPng;
png_infop pInfo;
int numPasses;
png_bytep *rows;
int generate5A1 = 0;

//24位深的bmp图片转换为16位深RGB565格式的bmp图片
struct RGB_Data{
    unsigned short B_data:5;
    unsigned short G_data:6;
    unsigned short R_data:5;
}lcd_img[LCD_PIXS]; //img size must less than 1024*768

int main(int argc, char* argv[]) {
    int fd_dev  = 0;
	
    // check parameter
    if (argc != 2 && argc != 3) {
        printf("Usage: png2c [PNG file] [-a]\n"
                "A header file will be generated in the same directory\n"
                "If -a is provided then the output will be RGB5A1 instead of RGB565.\n");
        return -1;
    }
    fd_dev = open(SYSTEM_EYE_DEV_NAME,O_WRONLY,0777);
    if (fd_dev < 0){
        printf("open st7789v error \n");
		return -1;
    }
    
    // open file
    printf("Opening file %s...\n", argv[1]);
    char header[8];
    file = fopen(argv[1], "rb");
    if (!file) {
        printf("Could not open file %s.\n", argv[1]);
        return -1;
    }
    
    // check if RGB5A1
    if (argc == 3 && 0 == strcmp(argv[2], "-a"))
        generate5A1 = 1;

    // read the header
    printf("Reading header...\n");
    if (fread(header, 1, 8, file) <= 0) {
        printf("Could not read PNG header.\n");
        return -1;
    }
    
    // make sure it's a PNG file
    if (png_sig_cmp(header, 0, 8)) {
        printf("File is not a valid PNG file.\n");
        return -1;
    }
    printf("Valid PNG file found. Reading more...\n");
    
    // create the read struct
    pPng = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!pPng) {
        printf("Error: couldn't read PNG read struct.\n");
        return -1;
    }

    // create the info struct
    png_infop pInfo = png_create_info_struct(pPng);
    if (!pInfo) {
        printf("Error: couldn't read PNG info struct.\n");
        png_destroy_read_struct(&pPng, (png_infopp)0, (png_infopp)0);
        return -1;
    }
    
    // basic error handling
    if (setjmp(png_jmpbuf(pPng))) {
        printf("Error reading PNG file properties.\n");
        return -1;
    }
    
    // read PNG properties
    png_init_io(pPng, file);
    png_set_sig_bytes(pPng, 8);
    png_read_info(pPng, pInfo);
    width = png_get_image_width(pPng, pInfo);
    height = png_get_image_height(pPng, pInfo);
    colorType = png_get_color_type(pPng, pInfo);
    bitDepth = png_get_bit_depth(pPng, pInfo);
    numPasses = png_set_interlace_handling(pPng);
    png_read_update_info(pPng, pInfo);
    printf("Metadata: w: %i, h: %i, color type: %i, bit depth: %i, num passes: %i\n", width, height, colorType, bitDepth, numPasses);

    // read the file
    if (setjmp(png_jmpbuf(pPng))) {
        printf("Error reading PNG file data.\n");
        return -1;
    }
    rows = (png_bytep*) malloc(sizeof(png_bytep) * height);
    printf("rows = %d, png_bytep=%d, png_byte =%d, height=%d\n", sizeof(rows), sizeof(png_bytep), sizeof(png_byte), height);
    for (y = 0; y < height; y++)
        rows[y] = (png_byte*) malloc(png_get_rowbytes(pPng,pInfo));
    png_read_image(pPng, rows);
    fclose(file);
    
    // verify that the PNG image is RGBA
    if (png_get_color_type(pPng, pInfo) != PNG_COLOR_TYPE_RGBA) {
        printf("Error: Input PNG file must be RGBA (%d), but is %d.", PNG_COLOR_TYPE_RGBA, png_get_color_type(pPng, pInfo));
        return -1;
    }
    
    // get the name of the image, without the extension (assume it ends with .png)
    size_t len = strlen(argv[1]);
    char *imageName = malloc(len - 3);
    memcpy(imageName, argv[1], len - 4);
    imageName[len - 4] = 0;
    printf("Creating output file %s.h %s.c...\n", imageName,imageName);
                       
    // create output file
    FILE *outputHeader, *outputSource;
    char outputHeaderName[100], outputSourceName[100];
    sprintf(outputHeaderName, "%s.h", imageName);
    sprintf(outputSourceName, "%s.c", imageName);
    outputHeader = fopen(outputHeaderName, "w"); // overwrite/create empty file
    outputSource = fopen(outputSourceName, "w"); // overwrite/create empty file
    
    // write the header
    fprintf(outputHeader,
        "// Generated by png2c [(c) Oleg Vaskevich 2013]\n\n"
        "#ifndef _%s_H_\n"
        "#define _%s_H_\n\n"
        "extern const unsigned int %s_width;\n"
        "extern const unsigned int %s_height;\n"
        "extern const int %s_size;\n"
        "extern const unsigned short %s[];\n\n"
        "#endif\n\n",
        imageName, imageName, imageName, imageName, imageName, imageName);
    fclose(outputHeader);

    // write the source
    fprintf(outputSource,
        "// Generated by png2c [(c) Oleg Vaskevich 2013]\n\n"
        "#include \"%s.h\"\n\n"
        "const unsigned int %s_width = %u;\n"
        "const unsigned int %s_height = %u;\n"
        "const int %s_size = %u;\n"
        "const unsigned short %s[] = {\n",
        imageName, imageName, width, imageName, height, imageName, width*height, imageName);

    // go through each pixel
    printf("Processing pixels...\n");
    unsigned short* gLogoBuf = (unsigned short*)malloc(sizeof(unsigned short)*height*width);
    memset(gLogoBuf, 0x0, sizeof(unsigned short)*height*width);
    unsigned char _gLogoBuf[LCD_PIXS*2 + 1] = {0};
    memset(_gLogoBuf, 0xff, LCD_PIXS*2+1);
    printf("---------------%d...\n", sizeof(unsigned short));
    for (y=0; y<height; y++) {
        png_byte* row = rows[y];
        for (x=0; x<width; x++) {
            png_byte* pixel = &(row[x*4]);
            unsigned short convPixel = generate5A1 ? RGB8888toRGB5A1(pixel[0], pixel[1], pixel[2], \
                pixel[3]) : RGB888toRGB565(pixel[0], pixel[1], pixel[2]);
            // printf("%04x ", convPixel);
            lcd_img[y*width+x].R_data = pixel[0]>>3;
            lcd_img[y*width+x].G_data = pixel[1]>>2;
            lcd_img[y*width+x].B_data = pixel[2]>>3;
            printf("%d ", y*width+x);
            // gLogoBuf[y*width+x] = convPixel;
            // the last pixel shouldn't have a comma
            if (y == height-1 && x == width-1)
                fprintf(outputSource, "0x%x\n};\n", convPixel);
            else
                fprintf(outputSource, "0x%x, ", convPixel);
        }
        printf("\n");
        fprintf(outputSource, "\n");
    }
    
    // write(fd_dev, gLogoBuf, sizeof(unsigned short)*height*width);
    memcpy(_gLogoBuf,(char*)lcd_img, 240*240*2);
    _gLogoBuf[240*240*2] = 2;
    write(fd_dev, _gLogoBuf, LCD_PIXS*2+1);
    fclose(outputSource);
    if (generate5A1)
        printf("Output format is RGB5A1.\n");
    else
        printf("Output format is RGB565.\n");
    printf("Done! Generated %s.h and %s.c.\n", imageName, imageName);
    close(fd_dev);
    free(gLogoBuf);
    return 0;
}





#define STB_IMAGE_IMPLEMENTATION

#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>


#define getByte(value, n) (value >> (n*8) & 0xFF)

typedef uint32_t DWORD;   // DWORD = unsigned 32 bit value
typedef uint16_t WORD;    // WORD = unsigned 16 bit value
typedef uint8_t BYTE;     // BYTE = unsigned 8 bit value

#pragma pack(push, 1)

typedef struct tagRGB {
    uint8_t blue;
    uint8_t green;
    uint8_t red;
} RGB;
#pragma pack(pop)


typedef struct tagARGB {
    uint8_t blue;
    uint8_t green;
    uint8_t red;
    uint8_t alpha;
} ARGB;

#pragma pack(push, 1)

typedef struct tagBITMAPFILEHEADER {
    WORD bfType;  //specifies the file type
    DWORD bfSize;  //specifies the size in bytes of the bitmap file
    WORD bfReserved1;  //reserved; must be 0
    WORD bfReserved2;  //reserved; must be 0
    DWORD bfOffBits;  //species the offset in bytes from the bitmapfileheader to the bitmap bits
} BITMAPFILEHEADER;

#pragma pack(pop)

#pragma pack(push, 1)

typedef struct tagBITMAPINFOHEADER {
    DWORD biSize;  //specifies the number of bytes required by the struct
    DWORD biWidth;  //specifies width in pixels
    DWORD biHeight;  //species height in pixels
    WORD biPlanes; //specifies the number of color planes, must be 1
    WORD biBitCount; //specifies the number of bit per pixel
    DWORD biCompression;//spcifies the type of compression
    DWORD biSizeImage;  //size of image in bytes
    DWORD biXPelsPerMeter;  //number of pixels per meter in x axis
    DWORD biYPelsPerMeter;  //number of pixels per meter in y axis
    DWORD biClrUsed;  //number of colors used by th ebitmap
    DWORD biClrImportant;  //number of colors that are important
} BITMAPINFOHEADER;

#pragma pack(pop)

unsigned char *LoadBitmapFile(char *filename, BITMAPFILEHEADER *bitMapFileHeader, BITMAPINFOHEADER *bitmapInfoHeader) {
    FILE *filePtr; //our file pointer
    BITMAPFILEHEADER bitmapFileHeader; //our bitmap file header
    unsigned char *bitmapImage;  //store image data
    int imageIdx = 0;  //image index counter
    unsigned char tempRGB;  //our swap variable

    //open filename in read binary mode
    filePtr = fopen(filename, "rb");
    if (filePtr == NULL)
        return NULL;

    //read the bitmap file header
    fread(&bitmapFileHeader, sizeof(BITMAPFILEHEADER), 1, filePtr);

    //verify that this is a bmp file by check bitmap id
    if (bitmapFileHeader.bfType != 0x4D42) {
        fclose(filePtr);
        return NULL;
    }
    bitMapFileHeader->bfOffBits = bitmapFileHeader.bfOffBits;
    bitMapFileHeader->bfReserved1 = bitmapFileHeader.bfReserved1;
    bitMapFileHeader->bfReserved2 = bitmapFileHeader.bfReserved2;
    bitMapFileHeader->bfSize = bitmapFileHeader.bfSize;
    bitMapFileHeader->bfType = bitmapFileHeader.bfType;
    //read the bitmap info header
    fread(bitmapInfoHeader, sizeof(BITMAPINFOHEADER), 1, filePtr);

    //move file point to the begging of bitmap data
    fseek(filePtr, bitmapFileHeader.bfOffBits, SEEK_SET);

    //allocate enough memory for the bitmap image data
    bitmapImage = (unsigned char *) malloc(bitmapInfoHeader->biSizeImage);

    //verify memory allocation
    if (!bitmapImage) {
        free(bitmapImage);
        fclose(filePtr);
        return NULL;
    }

    //read in the bitmap image data
    fread(bitmapImage, bitmapInfoHeader->biSizeImage, bitmapInfoHeader->biSizeImage, filePtr);

    //make sure bitmap image data was read
    if (bitmapImage == NULL) {
        fclose(filePtr);
        return NULL;
    }

    //swap the r and b values to get RGB (bitmap is BGR)
    for (imageIdx = 0; imageIdx < bitmapInfoHeader->biSizeImage; imageIdx += 3) // fixed semicolon
    {
        tempRGB = bitmapImage[imageIdx];
        bitmapImage[imageIdx] = bitmapImage[imageIdx + 2];
        bitmapImage[imageIdx + 2] = tempRGB;
    }

    //close file and return bitmap iamge data
    fclose(filePtr);
    return bitmapImage;
}

typedef struct {
    uint32_t *pixels;
    unsigned int w;
    unsigned int h;
} image_t;

uint32_t getpixel(image_t *image, unsigned int x, unsigned int y) {
    return image->pixels[(y * image->w) + x];
}

float lerp(float s, float e, float t) { return s + (e - s) * t; }

float blerp(float c00, float c10, float c01, float c11, float tx, float ty) {
    return lerp(lerp(c00, c10, tx), lerp(c01, c11, tx), ty);
}

void putpixel(image_t *image, unsigned int x, unsigned int y, uint32_t color) {
    image->pixels[(y * image->w) + x] = color;
}

void scale(image_t *src, image_t *dst, float scalex, float scaley) {
    int newWidth = (int) src->w * scalex;
    int newHeight = (int) src->h * scaley;
    int newSize = newWidth * newHeight;
    dst->pixels = malloc(sizeof(uint32_t) * newSize);
    dst->h = newHeight;
    dst->w = newWidth;
    int x, y;
    for (x = 0, y = 0; y < newHeight; x++) {
        if (x > newWidth) {
            x = 0;
            y++;
        }
        float gx = x / (float) (newWidth) * (src->w - 1);
        float gy = y / (float) (newHeight) * (src->h - 1);
        int gxi = (int) gx;
        int gyi = (int) gy;
        uint32_t result = 0;
        uint32_t c00 = getpixel(src, gxi, gyi);
        uint32_t c10 = getpixel(src, gxi + 1, gyi);
        uint32_t c01 = getpixel(src, gxi, gyi + 1);
        uint32_t c11 = getpixel(src, gxi + 1, gyi + 1);
        uint8_t i;
        for (i = 0; i < 3; i++) {
            result |= (uint8_t) blerp(getByte(c00, i), getByte(c10, i), getByte(c01, i), getByte(c11, i), gx - gxi,gy - gyi) << (8 * i);
        }
        putpixel(dst, x, y, result);
    }
}

int main(int argc, char **argv) {
    BITMAPINFOHEADER bitmapInfoHeader;
    BITMAPFILEHEADER bitmapfileheader;
    unsigned char *bitmapData;

    bitmapData = LoadBitmapFile("../input_image.bmp", &bitmapfileheader, &bitmapInfoHeader);
    int pixelCount = bitmapInfoHeader.biWidth * bitmapInfoHeader.biHeight;
    printf("Number of Pixels: %d Image Width: %d Image Height: %d Bits per Pixel: %d\n", (bitmapInfoHeader.biWidth * bitmapInfoHeader.biHeight), bitmapInfoHeader.biWidth, bitmapInfoHeader.biHeight, bitmapInfoHeader.biBitCount);

    RGB* rgb = (RGB*) bitmapData;
    ARGB* argb = malloc(sizeof(ARGB) * pixelCount);

    printf("B: %d B: %d\n",rgb[0].blue, bitmapData[0]);
    printf("G: %d G: %d\n",rgb[0].green, bitmapData[1]);
    printf("R: %d  R: %d\n",rgb[0].red, bitmapData[2]);

    int i;
    for(i = 0; i < pixelCount; i++){
        argb[i].alpha = 0;
        argb[i].blue = rgb[i].blue;
        argb[i].green = rgb[i].green;
        argb[i].red = rgb[i].red;
    }

    printf("B: %d B: %d\n",argb[0].blue, bitmapData[0]);
    printf("G: %d G: %d\n",argb[0].green, bitmapData[1]);
    printf("R: %d  R: %d\n",argb[0].red, bitmapData[2]);

    printf("Scaling setup\n");
    uint32_t* image_buffer = (uint32_t*) argb;
    image_t* image_source = malloc(sizeof(image_t));
    image_source->pixels = image_buffer;
    image_source->h = bitmapInfoHeader.biHeight;
    image_source->w = bitmapInfoHeader.biWidth;
    image_t* image_dest = malloc(sizeof(image_t));;
    printf("Starting scaling\n");
    scale(image_source, image_dest, 1, 1);
    printf("Done Scaling\n");

    free(argb);
    argb = (ARGB*) image_dest->pixels;
    int newImageSize = image_dest->h * image_dest->w;
    RGB* newRGB = malloc(sizeof(RGB) * newImageSize);
    for(i = 0; i < newImageSize; i++){
        newRGB[i].blue = argb[i].blue;
        newRGB[i].green = argb[i].green;
        newRGB[i].red = argb[i].red;
    }

    printf("Write BMP\n");
    FILE *outputFile = fopen("../output_image.bmp","wb");
    int byte_change = (newImageSize * 3) - bitmapInfoHeader.biSizeImage;
    bitmapfileheader.bfSize += byte_change;
    fwrite(&bitmapfileheader, sizeof(BITMAPFILEHEADER), 1, outputFile);
    bitmapInfoHeader.biHeight = image_dest->h;
    bitmapInfoHeader.biWidth = image_dest->w;
    bitmapInfoHeader.biSizeImage = newImageSize * 24;
    fwrite(&bitmapInfoHeader, sizeof(BITMAPINFOHEADER), 1, outputFile);
    fseek(outputFile, bitmapfileheader.bfOffBits, SEEK_SET);
    uint8_t* data = (uint8_t*) newRGB;
    fwrite(data, sizeof(uint8_t), newImageSize * 3, outputFile);
    fclose(outputFile);

    bitmapData = LoadBitmapFile("../input_image.bmp", &bitmapfileheader, &bitmapInfoHeader);
    printf("Number of Pixels: %d Image Width: %d Image Height: %d Bits per Pixel: %d\n", (bitmapInfoHeader.biWidth * bitmapInfoHeader.biHeight), bitmapInfoHeader.biWidth, bitmapInfoHeader.biHeight, bitmapInfoHeader.biBitCount);

    return 0;
}
#include <stdio.h>
#include <stdlib.h>


 Rgb **arr_buf = malloc((y1 - y0 + 1)*sizeof(Rgb*)); 
    unsigned int offset = ((x1 - x0 + 1) * sizeof(Rgb)) % 4; 
    offset = (offset ? 4-offset : 0); 
 
    for(int i = 0; i < y1-y0+1; i++){ 
        arr_buf[i] = malloc((x1 - x0 + 1) * sizeof(Rgb) + offset); 
        for(int j = 0; j < x1-x0+1; j++){ 
            arr_buf[i][j] = arr[y0 + i][x0 + j]; 
        } 
    } 
 
    int lim_x = (x2+x1-x0+1 > W) ? W : x2+x1-x0+1; 
    int lim_y = (y2+y1-y0+1 > H) ? H : y2+y1-y0+1; 
 
    for(int i = y2; i < (lim_y); i++){ 
        for(int j = x2; j < (lim_x); j++){ 
            arr[i][j] = arr_buf[i - y2][j - x2]; 
        } 
 
    } 
    fileCompletion(bmfh, bmif, H, W, arr, file); 
    freeRgbArr(arr_buf, y1 - y0 + 1); 
    return 0; 
} 




/*
unsigned int headerSize;
    unsigned int width;
    unsigned int height;
    unsigned short planes;
    unsigned short bitsPerPixel;
    unsigned int compression;
    unsigned int imageSize;
    unsigned int xPixelsPerMeter;
    unsigned int yPixelsPerMeter;
    unsigned int colorsInColorTable;
    unsigned int importantColorCount;
*/
#pragma pack(push, 1)  // Отключаем выравнивание структур
typedef struct {
    unsigned short bfType;      // 'BM' (0x4D42) signature
    unsigned int bfSize;        // Размер файла
    unsigned short bfReserved1; // 0 reserved1
    unsigned short bfReserved2; // 0 reserved2
    unsigned int bfOffBits;     // Смещение до пиксельных данных pixelArrOffset
} BITMAPFILEHEADER;

typedef struct {
    unsigned int biSize;         // Размер структуры (40)
    int biWidth;                 // Ширина изображения
    int biHeight;                // Высота изображения
    unsigned short biPlanes;     // 1
    unsigned short biBitCount;   // Бит на пиксель (1, 4, 8, 16, 24, 32) bitsPerPixel
    unsigned int biCompression;  // 0 (без сжатия)
    unsigned int biSizeImage;    // Размер пиксельных данных
    int biXPelsPerMeter;         // Горизонтальное разрешение (пикс/м)
    int biYPelsPerMeter;         // Вертикальное разрешение (пикс/м)
    unsigned int biClrUsed;      // Количество используемых цветов
    unsigned int biClrImportant; // Количество важных цветов
} BITMAPINFOHEADER;
#pragma pack(pop)  // Восстанавливаем выравнивание

int main() {
    const char* inputFile = "mk2.bmp";
    const char* outputFile = "output.bmp";

    FILE* in = fopen(inputFile, "rb");
    if (!in) {
        perror("Не удалось открыть файл");
        return 1;
    }

    BITMAPFILEHEADER fileHeader;
    BITMAPINFOHEADER infoHeader;

    // Чтение заголовков
    fread(&fileHeader, sizeof(BITMAPFILEHEADER), 1, in);
    fread(&infoHeader, sizeof(BITMAPINFOHEADER), 1, in);

    // Проверка, что это BMP-файл
    if (fileHeader.bfType != 0x4D42) {
        printf("Это не BMP-файл!\n");
        fclose(in);
        return 1;
    }

    // Проверка, что битность 24 бита (3 байта на пиксель)
    if (infoHeader.biBitCount != 24) {
        printf("Поддерживаются только 24-битные BMP-файлы\n");
        fclose(in);
        return 1;
    }

    // Вычисляем размер строки с учетом выравнивания (должно делиться на 4)
    int rowSize = ((infoHeader.biWidth * 3 + 3) / 4) * 4;
    unsigned char* pixels = malloc(rowSize * infoHeader.biHeight);

    // Чтение пикселей
    fseek(in, fileHeader.bfOffBits, SEEK_SET);
    fread(pixels, 1, rowSize * infoHeader.biHeight, in);
    fclose(in);

    // Инвертируем цвета (BGR)
    for (int i = 0; i < rowSize * infoHeader.biHeight; i++) {
        pixels[i] = 255 - pixels[i];  // Инверсия каждого байта
    }

    // Запись в новый файл
    FILE* out = fopen(outputFile, "wb");
    if (!out) {
        perror("Не удалось создать файл");
        free(pixels);
        return 1;
    }

    fwrite(&fileHeader, sizeof(BITMAPFILEHEADER), 1, out);
    fwrite(&infoHeader, sizeof(BITMAPINFOHEADER), 1, out);
    fwrite(pixels, 1, rowSize * infoHeader.biHeight, out);
    fclose(out);
    free(pixels);

    printf("Изображение успешно обработано!\n");
    return 0;
}

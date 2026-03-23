#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <math.h>
#include <sys/stat.h> 


#pragma pack (push, 1)

typedef struct {
    unsigned short signature;
    unsigned int filesize;
    unsigned short reserved1;
    unsigned short reserved2;
    unsigned int pixelArrOffset;

} BitmapFileHeader;


typedef struct 
{
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
    
}BitmapInfoHeader;

typedef struct 
{
    unsigned char b;
    unsigned char g;
    unsigned char r;

}Rgb;

typedef struct {

    float h;
    float s;
    float v;

}Hsv;

#pragma pack (pop)


typedef enum 
{   
    op_none,
    op_mirror,
    op_copy,
    op_circle,
    op_bitwise_threshold,
    op_contrast,
    op_bin,
    op_square_rhombus,
    op_hsv,
    op_info

}operation_struct;

typedef struct 
{
    int x;
    int y;

}Point;

typedef struct
{
    operation_struct operation;
    Point left_up;
    Point right_down;
    Point dleft_up;
    char axis;
    Point center;
    int radius;
    int thickness;
    Rgb color;
    int fill;
    Rgb fill_color;

    int thres_red;
    int thres_green;
    int thres_blue;

    float alfa;
    int beta;

    int threshold;

    Point upper_vertex;
    int size;

}image_operation;

void print_file_header(BitmapFileHeader header);
void print_info_header(BitmapInfoHeader header);
void Error_Handling(const char* ErrorMessage);
void TransformInColor(char *str, Rgb* color);
void transform_digital(char *str, Point *p);
unsigned int padding(unsigned int w);
unsigned int row_len(unsigned int w);
Rgb **read_bmp (const char* file_name, BitmapFileHeader* bmfh, BitmapInfoHeader* bmif);
void write_bmp (const char* file_name, Rgb **arr, BitmapFileHeader bmfh, BitmapInfoHeader bmif);
void free_bmp(Rgb** arr, unsigned int height);
Rgb **copy_image(Rgb **arr, BitmapInfoHeader *bmif, int left, int up, int right, int down, int dleft, int dup);
Rgb **mirror_image(Rgb **arr, BitmapInfoHeader * bmif, char axis, int left, int up, int right, int down);
Rgb **draw_circle(Rgb **arr, BitmapInfoHeader* bmif, Point center, int radius, int thickness, Rgb color, int fill, Rgb fill_color);
void print_help();

Rgb **contrast_image(Rgb **arr, BitmapInfoHeader* bmif, float alpha, int beta);
Rgb **binarization(Rgb **arr, BitmapInfoHeader* bmif, int threshold);

void print_file_header(BitmapFileHeader header){
    printf("signature:\t%x (%hu)\n", header.signature, header.signature);
    printf("filesize:\t%x (%u)\n", header.filesize, header.filesize);
    printf("reserved1:\t%x (%hu)\n", header.reserved1, header.reserved1);
    printf("reserved2:\t%x (%hu)\n", header.reserved2, header.reserved2);
    printf("pixelArrOffset:\t%x (%u)\n", header.pixelArrOffset, header.pixelArrOffset);
} 

void print_info_header(BitmapInfoHeader header){
    printf("headerSize:\t%x (%u)\n", header.headerSize,header.headerSize);
    printf("width: \t%x (%u)\n", header.width, header.width);
    printf("height: \t%x (%u)\n", header.height, header.height);
    printf("planes: \t%x (%hu)\n", header.planes, header.planes);
    printf("bitsPerPixel:\t%x (%hu)\n", header.bitsPerPixel, header.bitsPerPixel);
    printf("compression:\t%x (%u)\n", header.compression, header.compression);
    printf("imageSize:\t%x (%u)\n", header.imageSize, header.imageSize);
    printf("xPixelsPerMeter:\t%x (%u)\n", header.xPixelsPerMeter, header.xPixelsPerMeter);
    printf("yPixelsPerMeter:\t%x (%u)\n", header.yPixelsPerMeter, header.yPixelsPerMeter);
    printf("colorsInColorTable:\t%x (%u)\n", header.colorsInColorTable, header.colorsInColorTable);
    printf("importantColorCount:\t%x (%u)\n", header.importantColorCount, header.importantColorCount);
}

void Error_Handling(const char* ErrorMessage){
    fprintf(stderr, "Error: %s\n", ErrorMessage);
	return;
}

//Преобразовываем строку в номера цвета
void TransformInColor(char *str, Rgb* color)
{
    char *dot1 = strchr(str, '.');
    char *dot2 = strchr(dot1 + 1, '.');

    if(!dot1 || !dot2){
        Error_Handling("Incorrect data entry format. Use: 'rrr.ggg.bbb");
    }
    
    color->r = atoi(str);
    color->g = atoi(dot1 + 1);
    color->b = atoi(dot2 + 1);

    if(color->r > 255 || color->r < 0 ||
       color->b > 255 || color->b < 0 ||
       color->g > 255 || color->g < 0){
        Error_Handling("Color values must to be between 0 and 255");
    }

}
//Записываем координаты в виде чисел
void transform_digital(char *str, Point *p)
{
    char *dot = strchr(str, '.');

    if(!dot){
        Error_Handling("Incorrect data entry format. Use: 'x.y");
    }
    
    p->x = atoi(str);
    p->y = atoi(dot + 1);
}

//Вычисляем выравнивание
unsigned int padding(unsigned int w){
    return (4 - (w * sizeof(Rgb)) % 4) % 4;
}
//Вычисляем длину строки
unsigned int row_len(unsigned int w){
    return w * sizeof(Rgb) + padding(w);
}

//Читаем файл
Rgb **read_bmp (const char* file_name, BitmapFileHeader* bmfh, BitmapInfoHeader* bmif){
    FILE *f = fopen(file_name, "rb");
    if (!f){
        Error_Handling("File openings");
    }
    
    fread(bmfh, 1, sizeof(BitmapFileHeader), f);
    fread(bmif, 1, sizeof(BitmapInfoHeader), f);
    
    if (bmfh->signature != 0x4D42) {
        fclose(f);
        Error_Handling("This is not BMP file");
    }

    if (bmif->bitsPerPixel != 24) {
        Error_Handling("Only 24-bit BMP is supported");
    }

    if (bmfh->pixelArrOffset != sizeof(BitmapFileHeader) + sizeof(BitmapInfoHeader)) {
        Error_Handling("Incorrect pixelArrOffset");
    }
    unsigned int H = bmif->height;
    unsigned int W = bmif->width;

    Rgb **arr = malloc(H * sizeof(Rgb*));
    for (int i = 0; i < H; i++){
        arr[i] = malloc(row_len(W));
        fread(arr[i], 1, row_len(W), f);
    }
    fclose (f);
    return arr;
}

//Записываем файл в массив
void write_bmp (const char* file_name, Rgb **arr, BitmapFileHeader bmfh, BitmapInfoHeader bmif){
    
    if (!arr || bmif.width == 0 || bmif.height == 0) {
        Error_Handling("Incorrect image parameters");
    }

    FILE * ff = fopen(file_name, "wb");
    unsigned int H = bmif.height;
    unsigned int W = bmif.width;
    
    
    fwrite(&bmfh, 1, sizeof(BitmapFileHeader), ff);
    fwrite(&bmif, 1, sizeof(BitmapInfoHeader), ff);

    int padding = (4 - (W * 3) % 4) % 4;
    unsigned char paddingBytes[3] = {0};
    
    for (int y = 0; y < H; y++){
        if (!arr[y]) {
            exit(41);
        }

        for (int x = 0; x < W; x++){
            if (x >= W || y >= H) {
                exit(41);
            }
            fwrite(&arr[y][x].b, 1, 1, ff);
            fwrite(&arr[y][x].g, 1, 1, ff);
            fwrite(&arr[y][x].r, 1, 1, ff);
        }
        fwrite(paddingBytes, 1, padding, ff);
    }
    fclose(ff);
}

//Освобождаем массив
void free_bmp(Rgb** arr, unsigned int height) {
	for (unsigned int i = 0; i < height; i++) {
		free(arr[i]);
	}
	free(arr);
}

Rgb **copy_image(Rgb **arr, BitmapInfoHeader *bmif, int left, int up, int right, int down, int dleft, int dup) {
    int height = (int)bmif->height;
    int width = (int)bmif->width;
    
    left = (left < 0) ? 0 : (left >= width) ? width - 1 : left;
    up = (up < 0) ? 0 : (up >= height) ? height - 1 : up;
    right = (right < 0) ? 0 : (right >= width) ? width - 1 : right;
    down = (down < 0) ? 0 : (down >= height) ? height - 1 : down;

    dleft = (dleft < 0) ? 0 : (dleft >= width) ? width - 1 : dleft;
    dup = (dup < 0) ? 0 : (dup >= height) ? height - 1 : dup;

    if (left > right) {
        int temp = left;
        left = right;
        right = temp;
    }
    if (up > down) {
        int temp = up;
        up = down;
        down = temp;
    }

    int H = down - up; 
    int W = right - left;

    Rgb **temp = malloc(H * sizeof(Rgb *));
    if (!temp) return arr;

    for (int i = 0; i < H; i++) {
        temp[i] = malloc(W * sizeof(Rgb));
    }

    for (int y = 0; y < H; y++) {
        for (int x = 0; x < W; x++) {
            int src_y = height - 1 - (up + y);
            temp[y][x] = arr[src_y][left + x];
        }
    }

    for (int y = 0; y < H; y++) {
        for (int x = 0; x < W; x++) {
            int dest_y = height - 1 - (dup + y);
            int dest_x = dleft + x;

            if (dest_y >= 0 && dest_y < height && dest_x >= 0 && dest_x < width) {
                arr[dest_y][dest_x] = temp[y][x];
            }
        }
    }

    for (int i = 0; i < H; i++) {
        free(temp[i]);
    }
    free(temp);
    
    return arr;
}

//Отражение заданой области по определённой оси
Rgb **mirror_image(Rgb **arr, BitmapInfoHeader * bmif, char axis, int left, int up, int right, int down){
    int height = (int)bmif->height;
    int width = (int)bmif->width;

    left = (left < 0) ? 0 : left >= width ? width - 1 : left;
    up = (up < 0) ? 0 : up >= height ? height - 1 : up;
    right = (right < 0) ? 0 : right >= width ? width - 1 : right;
    down = (down < 0) ? 0 : down >= height ? height - 1 : down;

    if (left > right) {
        int temp = left;
        left = right;
        right = temp;
    }
    if (up > down) {
        int temp = up;
        up = down;
        down = temp;
    }
    down -= 2;
	up += 2;
    int H = down - up ; 
    int W = right - left;
    
    int up_arr = height - down;
    int down_arr = height - up;

    if (axis == 'y') {
        for (int y = -2; y < H / 2; y++) {
            int row1 = up_arr + y - 1;
            int row2 = down_arr - y - 1;
            for (int x = 0; x < W; x++) {
                Rgb temp = arr[row1][left + x];
                arr[row1][left + x] = arr[row2][left + x];
                arr[row2][left + x] = temp;
            }
       }
    } 
    else if (axis == 'x') {
        for (int y = 0; y < H; y++) {
           int row = up_arr + y;
            for (int x = 0; x < W / 2; x++) {
                int col1 = left + x;
                int col2 = right - x;
                Rgb temp = arr[row][col1];
                arr[row][col1] = arr[row][col2];
                arr[row][col2] = temp;
            }
        }
    }
    return arr;
}

Rgb **bitwise_threshold(Rgb **arr, BitmapInfoHeader* bmif, const char *op, int thres_red, int thres_blue,int thres_green){
    int height = (int)bmif->height;
    int width = (int)bmif->width;

    for (int y = 0; y < height; y++){
        for (int x = 0; x < width; x++){
            Rgb pixel = arr[y][x];
            unsigned char r,g,b;

            if (strcmp(op, "and") == 0){
                unsigned char result = pixel.r & pixel.g & pixel.b;
                r = (result > thres_red) ? result : 0;
                g = (result > thres_green) ? result : 0;
                b = (result > thres_blue) ? result : 0;
            }else{
                unsigned char result = pixel.r ^ pixel.g ^ pixel.b;
                r = (result > thres_red) ? result : 0;
                g = (result > thres_green) ? result : 0;
                b = (result > thres_blue) ? result : 0;
            }

            arr[y][x].r = r;
            arr[y][x].g = g;
            arr[y][x].b = b;
        }
    }
    return arr;
}

//Рисуем круг по координатам
Rgb **draw_circle(Rgb **arr, BitmapInfoHeader* bmif, Point center, int radius, int thickness, Rgb color, int fill, Rgb fill_color){
    
    for (int y = 0; y < bmif->height; y++){
        int actual_y = bmif->height - 1 - y;
        
        for (int x = 0; x < bmif->width; x++){
            double distance = sqrt(pow(x - center.x, 2) + pow(actual_y - center.y, 2));
            
            if (distance >= radius - (thickness/2) && distance <= radius + thickness/2){
                arr[y][x] = color;
            }else if (fill && distance <= radius){
                arr[y][x] = fill_color;
            }
        }
    }
    return arr;
}

Rgb **contrast_image(Rgb **arr, BitmapInfoHeader* bmif, float alfa, int beta){
    int height = (int)bmif-> height;
    int width = (int)bmif->width;

    for (int y = 0; y < height; y++){
        for (int x = 0; x < width; x++){

            int new_r = (int)(alfa * arr[y][x].r + beta);
            int new_g = (int)(alfa * arr[y][x].g + beta);
            int new_b = (int)(alfa * arr[y][x].b + beta);

            arr[y][x].r = (new_r < 0) ? 0 : (new_r > 255) ? 255 : new_r;
            arr[y][x].g = (new_g < 0) ? 0 : (new_g > 255) ? 255 : new_g;
            arr[y][x].b = (new_b < 0) ? 0 : (new_b > 255) ? 255 : new_b;
        }
    }
    return arr;
}

Rgb **binarization(Rgb **arr, BitmapInfoHeader* bmif, int threshold){
    int height = (int)bmif-> height;
    int width = (int)bmif->width;

    if (threshold < 0 || threshold > 765){
        Error_Handling("Threshold must be between 0 and 765");
    }

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
             int sum = arr[y][x].r + arr[y][x].g + arr[y][x].b;

             if (sum >= threshold){
                arr[y][x].r = 255;
                arr[y][x].g = 255;
                arr[y][x].b = 255;
            }else{
                arr[y][x].r = 0;
                arr[y][x].g = 0;
                arr[y][x].b = 0;
            }
        }
    }
    return arr;
}

Rgb **square_rhombus(Rgb **arr, BitmapInfoHeader* bmif, Point upper_vertex, int size, Rgb fill_color) {
    int height = (int)bmif->height;
    int width = (int)bmif->width;

    if (size <= 0) {
        Error_Handling("Size must be greater than 0");
        return arr;
    }

    int half_diag = (int)(size/sqrt(2.0));

    int center_x = upper_vertex.x;
    int center_y = upper_vertex.y + half_diag;

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            // Преобразуем координаты в систему с центром в заданной точке
            int dx = abs(x - center_x);
            int dy = abs(y - center_y);
            
            // Уравнение ромба: |x|/a + |y|/b <= 1 (где a и b - полудиагонали)
            // Для равностороннего ромба a = b = size
            if (dx + dy <= half_diag) {
                arr[y][x] = fill_color;
            }
        }
    }
    return arr;
}


Hsv switch_to_hsv(Rgb color){
    Hsv hsv;

    float r = color.r / 255.0;
    float g = color.g / 255.0;
    float b = color.b / 255.0;

    float max = (r > g) ? (r > b ? r : b) : (g > b ? g : b );
    float min = (r < g) ? (r < b ? r : b) : (g < b ? g : b );
    float delta = max - min;

    hsv.v = max;

    hsv.s = (max == 0) ? 0 : (delta/max);

    if (delta == 0){
        hsv.h = 0;
    }else{
        if(max == r){
            hsv.h = 60.0 * fmod((g - b) / delta, 6.0);
        }else if (max == g) {
            hsv.h = 60.0 * ((b - r) / delta + 2.0);
        } else {
            hsv.h = 60.0 * ((r - g) / delta + 4.0);
        }

        if (hsv.h < 0){
            hsv.h += 360.0;
        }
    }
    return hsv;
}

Rgb **convert_to_hsv(Rgb **arr, BitmapInfoHeader* bmif){
    int height = (int)bmif->height;
    int width = (int)bmif->width;

    for (int y = 0; y < height; y++){
        for (int x = 0; x < width; x++){
            Hsv hsv = switch_to_hsv(arr[y][x]);

            arr[y][x].b = (unsigned char)(hsv.h * 179 / 360);
            arr[y][x].g = (unsigned char)(hsv.s * 255);
            arr[y][x].r = (unsigned char)(hsv.v * 255);
        }
    }
    return arr;
}
*/

void print_help(){
    printf("Course work for option 4.1, created by Popova Elizaveta\n");
    printf("\nOptions\n");
    printf("\nFunction: Copy_image:"); 
    printf("\n    --copy -c: Copy the specified area by entering coordinates.");
    printf("\n    --left_up -l: Upper-right corner of the source area (used in copy and mirror)");
    printf("\n    --right_down -r: Lower-right corner of the source area (used in copy and mirror)");
    printf("\n    --dest_left_up -d: Coordinate of the upper-left corner of the destination area.\n");
    printf("\nFunction: mirror_image");
    printf("\n    --mirror -m: Reflection of a given area based on a given Y or X orientation");
    printf("\n    --axis -a: Reflection orientation");
    printf("\n    --left_up -l: The right upper corner of the source area (used in copy and mirror)");
    printf("\n    --right_down -r: Lower-right corner of the source area (used in copy and mirror)\n");
    printf("\nFunction: draw_circle");
    printf("\n    --circle -C: Drawing a circle of a certain size at the specified coordinates of a point");
    printf("\n    --center -cn: The center of the circle");
    printf("\n    --radius -R: The radius of the circle");
    printf("\n    --thickness -t: the thickness of the circle line");
    printf("\n    --color n: the color of the circle line");
    printf("\n    --fill -f: fill the circle\n    --fill_color -F: circle fill color\n");
    printf("\nOther  functions:");
    printf("\n    --input -i: the name of the input file");
    printf("\n    --output -o: the name of the output file (default: out.bmp)");
    printf("\n    --info -D: Information about file. It only works with --input");
    printf("\n    --help -h: Help");
    printf("\nAn example of an input command:\n");
    printf("    ./cw -c -l 200.100 -r 250.200 -d 200.200 -i input_name.bmp -o copied.bmp\n");
    printf("    ./cw --mirror --axis y --left_up 50.50 --right_down 20.20 --input input_file.bmp --output output.bmp\n");
    printf("    ./cw --circle --center 150.150 --radius 50 --thickness 10 --color 000.045.023 --fill --fill_color 230.220.255 --input together.bmp --output circle.bmp\n");
    printf("    ./cw --info --input input_file.bmp\n");
    printf("\nWhen compiling, please use the -lm flag.");

}

int main(int argc, char *argv[]) {
    
    image_operation operation = {0};
    int coordinates = 0;
    const char* inputFile = NULL;
    const char* outputFile = "out.bmp";
    const char* op = NULL;

    BitmapFileHeader bmfh;
    BitmapInfoHeader bmif;

    operation.thickness = 1;
    operation.fill = 0;

    const struct option long_options[] = {
        {"copy", no_argument, 0, 'c'},
        {"mirror", no_argument, 0, 'm'},
        {"circle", no_argument, 0, 'C'},

        {"bitwise_threshold", no_argument, 0, 'b'},
        {"op", required_argument, 0, 'P'},

        {"threshold_red", required_argument, 0, 'T'},
        {"threshold_blue", required_argument, 0, 'G'},
        {"threshold_green", required_argument, 0, 'B'},

        {"contrast_image", no_argument, 0, 'I'},
        {"alfa", required_argument, 0, 'A'},
        {"beta", required_argument, 0, 'E'},

        {"binarization", no_argument, 0, 'Z'},
        {"threshold", required_argument, 0, 'U'},

        {"square_rhombus", no_argument, 0, 'Q'},
        {"upper_vertex", required_argument, 0, 'V'},
        {"size", required_argument, 0, 'S'},

        {"axis", required_argument, 0, 'a'},
        {"left_up", required_argument, 0, 'l'},
        {"right_down", required_argument, 0, 'r'},
        {"dest_left_up", required_argument, 0, 'd'},
        {"center", required_argument, 0, 'n'},
        {"radius", required_argument, 0, 'R'},
        {"thickness", required_argument, 0, 't'},
        {"color", required_argument, 0, 'L' },
        {"fill", no_argument, 0, 'f'},
        {"fill_color", required_argument, 0, 'F'},
        {"input", required_argument, 0, 'i'},
        {"output", required_argument, 0, 'o'},
        {"help", no_argument, 0, 'h'},
        {"info", no_argument, 0, 'D'},
        {0, 0, 0, 0}
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "cmCbIZQV:S:A:U:E:P:T:G:B:a:l:r:d:n:R:t:L:fF:i:o:hD", long_options, NULL)) != -1) {
        switch (opt) {

            // copy
            case 'c':
                if (optarg != NULL){
                    Error_Handling("Flag --copy does not accept arguments");
                }

                if (operation.operation != op_none){
                    Error_Handling("Only one operation is available at a time");
                }
                operation.operation = op_copy;
                break;
            //mirror
            case 'm':
                if (optarg != NULL){
                    Error_Handling("Flag --mirror does not accept arguments");
                }

                if (operation.operation != op_none){                    
                    Error_Handling("Only one operation is available at a time");
                }
                operation.operation = op_mirror;
                break;
            
            //circle
            case 'C':
                if (optarg != NULL){
                    Error_Handling("Flag --circle does not accept arguments");
                }
                
                if (operation.operation != op_none) {
                    Error_Handling("Only one operation is available at a time");
                }
                operation.operation = op_circle;
                break;

    //binarization - 3
            case 'Z':
                if (optarg != NULL){
                    Error_Handling("Flag --mirror does not accept arguments");
                }
                if (operation.operation != op_none) {
                    Error_Handling("Only one operation is available at a time");
                }
                operation.operation = op_bin;
                break;

            case 'U':
                operation.threshold = atoi(optarg);
                if (operation.threshold < 0 || operation.threshold > 765) {
                    Error_Handling("Threshold must be between 0 and 765");
                }
                break;

        //contrast - 3?
            case 'I':
                if (operation.operation != op_none) {
                    Error_Handling("Only one operation is available at a time");
                }
                operation.operation = op_contrast;
                break;    
            
            case 'A':
                operation.alfa = atof(optarg);
                if (operation.alfa <= 0){
                    Error_Handling("Alfa must be greater than 0");
                }
                break;            

            case 'E':
                operation.beta = atoi(optarg);
                break;


        //Square_romb - 3
            case 'Q':
                if (optarg != NULL) {
                    Error_Handling("Flag --square_rhombus does not accept arguments");
                }
                if (operation.operation != op_none) {
                    Error_Handling("Only one operation is available at a time");
                }
                operation.operation = op_square_rhombus;
                break;

            case 'V':
                if (operation.operation != op_square_rhombus) {
                    Error_Handling("This parameter is only available for the --square_rhombus operation");
                }
                transform_digital(optarg, &operation.upper_vertex);
                break;

            case 'S':
                if (operation.operation != op_square_rhombus) {
                    Error_Handling("This parameter is only available for the --square_rhombus operation");
                }
                operation.size = atoi(optarg);
                if (operation.size <= 0) {
                    Error_Handling("Size must be greater than 0");
                }
                break;


            case 'b':
                if (optarg != NULL){
                    Error_Handling("Flag --bitwise_threshold does not accept arguments");
                }
                
                if (operation.operation != op_none) {
                    Error_Handling("Only one operation is available at a time");
                }
                operation.operation = op_bitwise_threshold;
                break;
            
            case 'P':
				op = optarg;
                break;

            case 'T':
                operation.thres_red = atoi(optarg);
                break;
            
            
            case 'G':
                operation.thres_green = atoi(optarg);
                break;

                
            case 'B':
                operation.thres_blue = atoi(optarg);
                break;


            case 'a':
				operation.axis = optarg [0];
                if (strcmp(optarg, "x") != 0 && strcmp(optarg, "y")!= 0){
                    Error_Handling("Axis must be either 'x' or 'y'");
                }
                
                break;
                
            case 'l':
                coordinates += 1;
                transform_digital(optarg, &operation.left_up);
                break;
                
            case 'r':
                coordinates += 1;
                transform_digital(optarg, &operation.right_down);
                break;
                
            case 'd':
                coordinates += 1;
                transform_digital(optarg, &operation.dleft_up);
                break;
            
            case 'n':
                if (operation.operation != op_circle) {
                    Error_Handling("This parameter is only available for the --circle operation");
                }
                transform_digital(optarg, &operation.center);
                break;
            
            case 'R':
                if (operation.operation != op_circle) {
                    Error_Handling("This parameter is only available for the --circle operation");
                }
                operation.radius = atoi(optarg);
                break;

            case 't':
                if (operation.operation != op_circle) {
                    Error_Handling("This parameter is only available for the --circle operation");
                }
                operation.thickness = atoi(optarg);
                break;
                
            case 'L':
                if (operation.operation != op_circle) {
                    Error_Handling("This parameter is only available for the --circle operation");
                }
                TransformInColor(optarg, &operation.color);
                break;
            
            case 'f':
                if (optarg != NULL){
                    Error_Handling("Flag --fill does not accept arguments");
                }
                if (operation.operation != op_circle) {
                    Error_Handling("This parameter is only available for the --circle operation");
                }
                operation.fill = 1;
                break;

            case 'F':
                TransformInColor(optarg, &operation.fill_color);
                break;

            case 'i':
                inputFile = optarg;
                break;
                
            case 'o':
                outputFile = optarg;
                break;
                
            case 'h':
                if (optarg != NULL){
                    Error_Handling("Flag --help does not accept arguments");
                }
                print_help();
                return 0;
            
            case 'D':
                if (optarg != NULL){
                    Error_Handling("Flag --info does not accept arguments");
                }

                if (operation.operation != op_none) {
                    Error_Handling("--info cannot be used with other operations");
                }
                operation.operation = op_info;
                break;
                
            case '?':
                Error_Handling("Unknown option or missing argument");
        }
    }

    //Проверка обязательных аргументов
    if (inputFile == NULL) {
        Error_Handling("Input file is required");
    }
    
    if(strcmp(inputFile, outputFile) == 0){
        Error_Handling("Input and Output file cannot be the same");
    }

    if (operation.operation == op_none) {
        Error_Handling("Operation not selected");
    }

    if((coordinates < 3 && operation.operation == op_copy) || (coordinates < 2 && operation.operation == op_mirror)){
        Error_Handling("The necessary coordinate parameters are missing");
    }

    if (operation.operation == op_mirror && operation.axis == '\0') {
        Error_Handling("The required --axis parameter is missing");
    }

    if (operation.operation == op_circle) {
        if (operation.radius == 0){
            Error_Handling("The required --radius parameter is missing");
        }
        if (operation.thickness <= 0){
            Error_Handling("The --thickness parameter cannot be less than or equal to 0");
        }
        if (operation.radius <= 0){
            Error_Handling("The --radius cannot be less than or equal to 0");
        }
        if (operation.fill && operation.fill_color.r == 0 && operation.fill_color.g == 0 && operation.fill_color.b == 0){
            Error_Handling("To fill the shape, you must specify --fill_color");
        }
    }

	if (operation.operation != op_mirror && operation.axis != 0){
        Error_Handling("Axis is only available with the --mirror flag");
    }

	if (operation.operation != op_copy && coordinates == 3) {
        Error_Handling("This parameter is only available for the --copy operation");
    }

    if (operation.operation == op_info){
        FILE *info_file = fopen(inputFile, "rb");
        
        if (!info_file) {
            Error_Handling("File openings");
        }

        size_t file_header_read = fread(&bmfh, 1, sizeof(BitmapFileHeader), info_file);
        size_t info_header_read = fread(&bmif, 1, sizeof(BitmapInfoHeader), info_file);
    
        fclose(info_file);

        if (file_header_read != sizeof(BitmapFileHeader) || 
            info_header_read != sizeof(BitmapInfoHeader)) {
                Error_Handling("Could not read BMP headers");
        }

        if (bmfh.signature != 0x4D42) {
            fclose(info_file);
            Error_Handling("This is not BMP file");
        }

        print_file_header(bmfh);
        print_info_header(bmif);
        return 0;
    }


    Rgb **arr = read_bmp(inputFile, &bmfh, &bmif);

     switch (operation.operation) {
        case op_copy:
            write_bmp(outputFile, copy_image(arr, &bmif, operation.left_up.x, operation.left_up.y, operation.right_down.x, operation.right_down.y, operation.dleft_up.x, operation.dleft_up.y), bmfh, bmif);
            break;
        case op_mirror:
            write_bmp(outputFile, mirror_image(arr, &bmif, operation.axis, operation.left_up.x, operation.left_up.y, operation.right_down.x, operation.right_down.y), bmfh, bmif);
            break;
        case op_circle:
            write_bmp(outputFile, draw_circle(arr, &bmif, operation.center, operation.radius, operation.thickness, operation.color, operation.fill, operation.fill_color), bmfh, bmif);
            break;
        case op_bitwise_threshold:
            write_bmp(outputFile, bitwise_threshold(arr, &bmif, op, operation.thres_red, operation.thres_green, operation.thres_blue), bmfh, bmif);
            break;
        case op_contrast:
            write_bmp(outputFile, contrast_image(arr, &bmif, operation.alfa, operation.beta), bmfh, bmif);
            break;
        case op_bin:
            write_bmp(outputFile, binarization(arr, &bmif, operation.threshold), bmfh, bmif);
            break;
        case op_square_rhombus:
            write_bmp(outputFile, square_rhombus(arr, &bmif, operation.upper_vertex, operation.size, operation.fill_color), bmfh, bmif);
            break;
        default:
            break;
    }

    free_bmp(arr, bmif.height);
    return 0;
}

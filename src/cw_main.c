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

#pragma pack (pop)


typedef enum 
{   
    op_none,
    op_mirror,
    op_copy,
    op_circle,
    op_bit,
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

    const char *op;
    int threshold_red;
    int threshold_blue;
    int threshold_green;

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

Rgb **bitwise_threshold(Rgb **arr, BitmapInfoHeader* bmif, int threshold_red, int threshold_green, int threshold_blue, const char *op){
    int height = (int)bmif->height;
    int width = (int)bmif->width;

    for (int y = 0; y < height; y++){
        for (int x = 0; x < width; x++){
            Rgb pixel = arr[y][x];
            unsigned char r, g, b;

            if (strcmp(op, "and")){
                unsigned char result = pixel.r & pixel.g & pixel.b;
                r = (result > threshold_red) ? result : 0;
                g = (result > threshold_green) ? result : 0;
                b = (result > threshold_blue) ? result : 0;
            }else{
                unsigned char result = pixel.r ^ pixel.g ^ pixel.b;
                r = (result > threshold_red) ? result : 0;
                g = (result > threshold_green) ? result : 0;
                b = (result > threshold_blue) ? result : 0;
            }
        arr[y][x].r = r;
        arr[y][x].g = g;
        arr[y][x].b = b;
        }
    }
    return arr;
}



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
    printf("Course work for option 4.1, created by Popova Elizaveta\n");
    
    image_operation operation = {0};
    int coordinates = 0;
    const char* inputFile = NULL;
    const char* outputFile = "out.bmp";

    BitmapFileHeader bmfh;
    BitmapInfoHeader bmif;

    operation.thickness = 1;
    operation.fill = 0;

    const struct option long_options[] = {
        {"copy", no_argument, 0, 'c'},
        {"mirror", no_argument, 0, 'm'},
        {"circle", no_argument, 0, 'C'},

        {"bitwise", no_argument, 0, 'B'},
        {"op", required_argument, 0, 'O'},
        {"red", required_argument, 0, 'e'},
        {"green", required_argument, 0, 'g'},
        {"blue", required_argument, 0, 'U'},


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
    while ((opt = getopt_long(argc, argv, "cmCBO:e:g:U:a:l:r:d:n:R:t:L:fF:i:o:hD", long_options, NULL)) != -1) {
        switch (opt) {
            case 'c':
                if (optarg != NULL){
                    Error_Handling("Flag --copy does not accept arguments");
                }

                if (operation.operation != op_none){
                    Error_Handling("Only one operation is available at a time");
                }
                operation.operation = op_copy;
                break;
            
            case 'm':
                if (optarg != NULL){
                    Error_Handling("Flag --mirror does not accept arguments");
                }

                if (operation.operation != op_none){                    
                    Error_Handling("Only one operation is available at a time");
                }
                operation.operation = op_mirror;
                break;
            
            case 'C':
                if (optarg != NULL){
                    Error_Handling("Flag --circle does not accept arguments");
                }
                
                if (operation.operation != op_none) {
                    Error_Handling("Only one operation is available at a time");
                }
                operation.operation = op_circle;
                break;
            
                //op_bit
            case 'B':
                if (operation.operation != op_none) {
                    Error_Handling("Only one operation is available at a time");
                }
                operation.operation = op_bit;
                break;

            case 'O':
                operation.op = optarg;
                break;

            case 'e':
                operation.threshold_red = atoi(optarg);
                break;

            case 'g':
                operation.threshold_green = atoi(optarg);
                break;

            case 'U':
                operation.threshold_blue = atoi(optarg);
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
                if (operation.operation != op_circle) {
                    Error_Handling("This parameter is only available for the --circle operation");
                }
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
        case op_bit:
            write_bmp(outputFile, bitwise_threshold(arr, &bmif, operation.threshold_red, operation.threshold_green, operation.threshold_blue, operation.op), bmfh, bmif);
            break;
        default:
            break;
    }

    free_bmp(arr, bmif.height);
    return 0;
}

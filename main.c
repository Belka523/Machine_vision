/*
 * main.c — консольное приложение imgproc
 * Использование:
 *   ./imgproc <name.расширение> <операция> [параметры] <name.расширение>
 * Примеры:
 *   ./imgproc photo.png -median 2 out.png
 */

#include <stdio.h>   
#include <stdlib.h>  
#include <string.h>

#include "imgproc.h"


// Повышение резкости: центральный пиксель усиливается, соседи вычитаются
static const float KERNEL_SHARPEN[9] = {
     0.0f, -1.0f,  0.0f,
    -1.0f,  5.0f, -1.0f,
     0.0f, -1.0f,  0.0f
};

//(emboss): создаёт эффект выпуклой поверхности
static const float KERNEL_EMBOSS[9] = {
    -2.0f, -1.0f,  0.0f,
    -1.0f,  1.0f,  1.0f,
     0.0f,  1.0f,  2.0f
};

// Простое размытие: среднее всех соседей
static const float KERNEL_BLUR[9] = {
    1.0f/9, 1.0f/9, 1.0f/9,
    1.0f/9, 1.0f/9, 1.0f/9,
    1.0f/9, 1.0f/9, 1.0f/9
};

static void print_usage()
{
    printf("Usage:\n");
    printf(" ./imgproc <name.file extension> <operation> [параметры] <name.file extension>\n");
    printf("\noperations:\n");
    printf("  -median  <radius> median filter (radius: 1, 2, 3...)\n");
    printf("  -gauss   <size> [sigma]    Gaussian filter (size: 3,5,7; sigma: 0.5..3.0)\n");
    printf("  -edges   [threshold]       Sobel boundary detection (threshold: 0..255)\n");
    printf("  -convolve <preset>         Convolution: sharpen | emboss | blur\n");
    printf("  -bc      <bright> <contr>  brightness (-255..255) and contrast (0.5..2.0)\n");
    printf("\nExamples:\n");
    printf(" ./imgproc photo.png -median 2 out.png\n");
    printf(" ./imgproc photo.png -gauss 5 1.4 out.png\n");
    printf(" ./imgproc photo.png -edges 60 out.png\n");
    printf(" ./imgproc photo.png -convolve sharpen out.png\n");
    printf(" ./imgproc photo.png -bc 30 1.5 out.png\n");
}


int main(int argc, char *argv[])
{

    if (argc == 2 && (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "-help") == 0)) {
        print_usage(argv[0]);
        return 0; 
    }
    // argc — количество аргументов 
    // argv — массив строк: argv[0]="./imgproc", argv[1]="photo.png", ...
    if (argc < 4) {
        print_usage(argv[0]);
        return 1;   
    }

    const char *input_path  = argv[1];           // первый аргумент — входной файл 
    const char *operation   = argv[2];           // второй — операция
    const char *output_path = argv[argc - 1];    // последний — выходной файл

    //загрузка изображения
    printf("loading: %s\n", input_path);
    Image *src = img_load(input_path);
    if (!src) return 1;
    printf("Size: %d x %d, image channel: %d\n", src->width, src->height, src->channels);

    Image *result = NULL;   

    //выбор операции по флагу

    if (strcmp(operation, "-median") == 0) {
        // -median <radius>
        // argv[3] — радиус окна. atoi() конвертирует строку в int.
        int radius = (argc >= 5) ? atoi(argv[3]) : 1;  //по умолчанию radius=1
        printf("Медианный фильтр, radius=%d\n", radius);
        result = img_median_filter(src, radius);

    } else if (strcmp(operation, "-gauss") == 0) 
    {
        //-gauss <size> [sigma]
        //argc >= 5: есть хотя бы размер ядра
        //argc >= 6: есть и sigma
        if (argc < 5) {
            fprintf(stderr, "Mistake: specify the size of the core, for example: -gauss 5\n");
            img_free(src);
            return 1;
        }
        int   ksize = atoi(argv[3]);
        float sigma = (argc >= 6) ? (float)atof(argv[4]) : 0.0f;
        printf("Gaussian filter, size=%d, sigma=%.2f\n", ksize, sigma);
        result = img_gaussian_filter(src, ksize, sigma);

    } else if (strcmp(operation, "-edges") == 0) {
        //-edges [threshold]
        //threshold=0 означает без бинаризации — оставление градиента как есть
        int threshold = (argc >= 5) ? atoi(argv[3]) : 0;
        printf("Sobel Boundary detection, threshold=%d\n", threshold);
        result = img_edge_detect(src, threshold);

    } else if (strcmp(operation, "-convolve") == 0) {
        // -convolve <preset>
        // выбор одно из готовых ядер
        if (argc < 5) {
            fprintf(stderr, "Mistake: specify a preset: sharpen, emboss или blur\n");
            img_free(src);
            return 1;
        }
        const char  *preset = argv[3];
        const float *kernel = NULL;

        if      (strcmp(preset, "sharpen") == 0) { kernel = KERNEL_SHARPEN; printf("Convolution: sharpness\n"); }
        else if (strcmp(preset, "emboss")  == 0) { kernel = KERNEL_EMBOSS;  printf("Convolution: relief\n");  }
        else if (strcmp(preset, "blur")    == 0) { kernel = KERNEL_BLUR;    printf("Convolution: blurring\n"); }
        else {
            fprintf(stderr, "Error: Unknown preset '%s'\n", preset);
            img_free(src);
            return 1;
        }
        result = img_convolve(src, kernel, 3);

    } else if (strcmp(operation, "-bc") == 0) {
        // -bc <brightness> <contrast>
        // Нужны оба параметра → argc >= 6
        if (argc < 6) {
            fprintf(stderr, "Error: you need two parameters, for example: -bc 30 1.5\n");
            img_free(src);
            return 1;
        }
        int   brightness = atoi(argv[3]);
        float contrast   = (float)atof(argv[4]);
        printf("Brightness=%d, contrast=%.2f\n", brightness, contrast);
        result = img_brightness_contrast(src, brightness, contrast);

    } else {
        fprintf(stderr, "Error: unknown operation '%s'\n", operation);
        print_usage(argv[0]);
        img_free(src);
        return 1;
    }

    //сохранение результата
    if (!result) {
        fprintf(stderr, "Error: the operation failed\n");
        img_free(src);
        return 1;
    }

    printf("Saving: %s\n", output_path);
    int ok = img_save(result, output_path);

    //освобождение памяти
    img_free(src);
    img_free(result);

    if (ok) {
        printf("Готово!\n");
        return 0;   
    }
    return 1;
}

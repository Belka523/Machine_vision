#ifndef IMGPROC_H
#define IMGPROC_H

#include <stdint.h>  

/*
 * Структура изображения
 * data     — массив пикселей
 * width    — ширина в пикселях
 * height   — высота в пикселях
 * channels — 1 = grayscale, 3 = RGB, 4 = RGBA
 */
typedef struct {
    uint8_t *data;
    int width;
    int height;
    int channels;
} Image;

//Загрузка / сохранение (поддерживает PNG, JPG, BMP, TGA)
Image *img_load(const char *path);                             
int    img_save(const Image *img, const char *path);           

void   img_free(Image *img);
Image *img_clone(const Image *src);

//Медианный фильтр
Image *img_median_filter(const Image *src, int radius);

//Гауссов фильтр
Image *img_gaussian_filter(const Image *src, int kernel_size, float sigma);

//Детекция границ
Image *img_edge_detect(const Image *src, int threshold);

//Свёртка
Image *img_convolve(const Image *src, const float *kernel, int kernel_size);

//Яркость и контраст
Image *img_brightness_contrast(const Image *src, int brightness, float contrast);

#endif 

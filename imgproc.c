#define STB_IMAGE_IMPLEMENTATION

#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION

#include "stb_image_write.h"

#include "imgproc.h"
#include <stdlib.h>  
#include <string.h>  
#include <math.h>    
#include <stdio.h>   

//ограничение диапазона 0-255
#define CLAMP(v) ((v) < 0 ? 0 : (v) > 255 ? 255 : (v))

//загрузка изображения/определение формата
Image *img_load(const char *path)
{
    Image *img = malloc(sizeof(Image));
    if (!img) return NULL;

    /*
     *   &img->width — ширина
     *   &img->height — высота
     *   &img->channels — число каналов
     */
    img->data = stbi_load(path, &img->width, &img->height, &img->channels, 0);
    if (!img->data) 
    {
        fprintf(stderr, "Error: failed to upload '%s': %s\n",
                path, stbi_failure_reason());
        free(img);
        return NULL;
    }
    return img;
}

//сохранение изображения в определенном формате
int img_save(const Image *img, const char *path)
{
    //strrchr находит последнее вхождение символа — точку расширения
    const char *trg = strrchr(path, '.');
    int flag = 0;

    if (strcmp(trg, ".png") == 0 || strcmp(trg, ".PNG") == 0) 
    {
        int step = img->width * img->channels;
        flag = stbi_write_png(path, img->width, img->height,
                            img->channels, img->data, step);
    } else if (strcmp(trg, ".jpg") == 0 || strcmp(trg, ".jpeg") == 0 ||
               strcmp(trg, ".JPG") == 0 || strcmp(trg, ".JPEG") == 0) 
    {
        //качество JPEG (1..100)
        flag = stbi_write_jpg(path, img->width, img->height,
                            img->channels, img->data, 90);
    } else if (strcmp(trg, ".bmp") == 0 || strcmp(trg, ".BMP") == 0) 
    {
        flag = stbi_write_bmp(path, img->width, img->height,
                            img->channels, img->data);
    } else {
        fprintf(stderr, "Error: unknown format '%s' (needed .png/.jpg/.bmp)\n", trg);
        return 0;
    }
    return flag;
}

//освобождение памяти
void img_free(Image *img)
{
    if (!img) return;
    stbi_image_free(img->data); 
    free(img);
}

Image *img_clone(const Image *src)
{
    Image *dst = malloc(sizeof(Image));
    size_t size = (size_t)src->width * src->height * src->channels;
    
    dst->data = malloc(size);
    memcpy(dst->data, src->data, size);
    
    dst->width = src->width;
    dst->height = src->height;
    dst->channels = src->channels;
    
    return dst;
}

//перевод в оттенки серого
static Image *to_gray(const Image *src)
{
    Image *dst = malloc(sizeof(Image));
    dst->width = src->width;
    dst->height = src->height;
    dst->channels = 1;
    
    size_t n = (size_t)src->width * src->height;
    dst->data = malloc(n);
    
    for (size_t i = 0; i < n; i++) {
        const uint8_t *p = src->data + i * src->channels;
        uint8_t r = p[0];
        uint8_t g = (src->channels >= 2) ? p[1] : r;
        uint8_t b = (src->channels >= 3) ? p[2] : r;
        dst->data[i] = (uint8_t)(0.299f * r + 0.587f * g + 0.114f * b);
    }
    return dst;
}

//МЕДИАННЫЙ ФИЛЬТР
//Для каждого пикселя берётся окно (2*radius+1)×(2*radius+1)
//сортируются яркости соседей, берётся серединное значение.
static int cmp_byte(const void *a, const void *b)
{
    return (int)(*(const uint8_t *)a) - (int)(*(const uint8_t *)b);
}

Image *img_median_filter(const Image *src, int radius)
{
    if (radius < 1) radius = 1;
    Image *dst = img_clone(src);
    int w = src->width, h = src->height, c = src->channels;
    int wsize = (2 * radius + 1) * (2 * radius + 1);
    uint8_t *window = malloc((size_t)wsize);
    
    for (int y = 0; y < h; y++) 
    {
        for (int x = 0; x < w; x++) 
        {
            for (int ch = 0; ch < c; ch++) 
            {
                if (c == 4 && ch == 3) continue; 
                int n = 0;
                for (int dy = -radius; dy <= radius; dy++) 
                {
                    for (int dx = -radius; dx <= radius; dx++) 
                    {
                        int nx = x + dx, ny = y + dy;
                        if (nx < 0)  nx = -nx;
                        if (ny < 0)  ny = -ny;
                        if (nx >= w) nx = 2 * w - nx - 2;
                        if (ny >= h) ny = 2 * h - ny - 2;
                        if (nx < 0)  nx = 0;
                        if (ny < 0)  ny = 0;
                        if (nx >= w) nx = w - 1;
                        if (ny >= h) ny = h - 1;
                        window[n++] = src->data[(ny * w + nx) * c + ch];
                    }
                }
                qsort(window, (size_t)n, 1, cmp_byte);
                dst->data[(y * w + x) * c + ch] = window[n / 2];
            }
        }
    }
    free(window);
    return dst;
}

//СВЁРТКА (базовая операция, используется гауссом)
//output[x,y] = Σ kernel[kx,ky] * input[x+kx, y+ky]
Image *img_convolve(const Image *src, const float *kernel, int kernel_size)
{
    if (kernel_size % 2 == 0) kernel_size++;
    int half = kernel_size / 2;
    Image *dst = img_clone(src);
    int w = src->width, h = src->height, c = src->channels;
    
    for (int y = 0; y < h; y++) 
    {
        for (int x = 0; x < w; x++) 
        {
            for (int ch = 0; ch < c; ch++) 
            {
                if (c == 4 && ch == 3) continue;
                float res = 0.0f;
                for (int ky = -half; ky <= half; ky++) 
                {
                    for (int kx = -half; kx <= half; kx++) 
                    {
                        int nx = x + kx, ny = y + ky;
                        if (nx < 0)  nx = 0;
                        if (ny < 0)  ny = 0;
                        if (nx >= w) nx = w - 1;
                        if (ny >= h) ny = h - 1;
                        float k = kernel[(ky + half) * kernel_size + (kx + half)];
                        res += k * (float)src->data[(ny * w + nx) * c + ch];
                    }
                }
                dst->data[(y * w + x) * c + ch] = (uint8_t)CLAMP((int)res);
            }
        }
    }
    return dst;
}

//ГАУСОВ ФИЛЬТР
//Строит ядро по формуле: G(x,y) = exp(-(x²+y²) / (2·σ²))
Image *img_gaussian_filter(const Image *src, int kernel_size, float sigma)
{
    if (kernel_size % 2 == 0) kernel_size++;
    if (kernel_size < 3)      kernel_size = 3;
    if (sigma <= 0.0f) sigma = (float)(kernel_size / 2) / 3.0f;
    
    int half = kernel_size / 2;
    float *kernel = malloc((size_t)kernel_size * kernel_size * sizeof(float));
    float sum = 0.0f;
    
    for (int ky = -half; ky <= half; ky++) 
    {
        for (int kx = -half; kx <= half; kx++) 
        {
            float val = expf(-(float)(kx * kx + ky * ky) / (2.0f * sigma * sigma));
            kernel[(ky + half) * kernel_size + (kx + half)] = val;
            sum += val;
        }
    }
    for (int i = 0; i < kernel_size * kernel_size; i++)
        kernel[i] /= sum;
        
    Image *result = img_convolve(src, kernel, kernel_size);
    free(kernel);
    return result;
}

//ДЕТЕКТОР ГРАНИЦ
//
Image *img_edge_detect(const Image *src, int threshold)
{
    Image *gray = to_gray(src);
    int w = gray->width, h = gray->height;
    
    Image *dst = malloc(sizeof(Image));
    dst->width = w;
    dst->height = h;
    dst->channels = 1;
    dst->data = calloc((size_t)w * h, 1);
    
    const int Kx[3][3] = {{-1, 0, 1}, {-2, 0, 2}, {-1, 0, 1}};
    const int Ky[3][3] = {{-1, -2, -1}, {0, 0, 0}, {1, 2, 1}};
    
    for (int y = 1; y < h - 1; y++) 
    {
        for (int x = 1; x < w - 1; x++) 
        {
            int gx = 0, gy = 0;
            for (int ky = -1; ky <= 1; ky++) 
            {
                for (int kx = -1; kx <= 1; kx++) 
                {
                    int pixel = gray->data[(y + ky) * w + (x + kx)];
                    gx += Kx[ky + 1][kx + 1] * pixel;
                    gy += Ky[ky + 1][kx + 1] * pixel;
                }
            }
            int mag = (int)sqrtf((float)(gx * gx + gy * gy));
            if (threshold > 0)
                mag = (mag >= threshold) ? 255 : 0;
            dst->data[y * w + x] = (uint8_t)CLAMP(mag);
        }
    }
    img_free(gray);
    return dst;
}

//ИЗМЕНЕНИЕ ЯРКОСТИ И КОНТРАСТА
Image *img_brightness_contrast(const Image *src, int brightness, float contrast)
{
    Image *dst = img_clone(src);
    int total = src->width * src->height * src->channels;
    int skip_a = (src->channels == 4);
    
    for (int i = 0; i < total; i++) {
        if (skip_a && (i % 4 == 3)) continue;  
        float v = (float)src->data[i];
        v = contrast * (v - 128.0f) + 128.0f + (float)brightness;
        dst->data[i] = (uint8_t)CLAMP((int)v);
    }
    return dst;
}

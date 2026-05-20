@echo off
chcp 65001 >nul
echo Тесты

if not exist imgproc.exe (
    echo Файл imgproc.exe не найден! выполните mingw32-make.
    exit /b
)


if not exist test.jpg (
    echo Положите 'test.jpg' в эту папку!
    exit /b
)

:: Создаем папку для результатов, если её нет
if not exist test_results mkdir test_results

echo.
echo Медианный фильтр (-median 5)
imgproc.exe test.jpg -median 5 test_results/out_median.png >nul
if exist test_results/out_median.png (echo successfully) else (echo error)

echo Гауссов фильтр (-gauss 15 4.0) -^> сохранение в BMP
imgproc.exe test.jpg -gauss 15 4.0 test_results/out_gauss.bmp >nul
if exist test_results/out_gauss.bmp (echo successfully) else (echo error)

echo Детекция границ Собеля (-edges 100)
imgproc.exe test.jpg -edges 100 test_results/out_edges.jpg >nul
if exist test_results/out_edges.jpg (echo successfully) else (echo error)

echo Свёртка (emboss)
imgproc.exe test.jpg -convolve emboss test_results/out_emboss.png >nul
if exist test_results/out_emboss.png (echo successfully) else (echo error)

echo Яркость и контраст (-bc 80 2.0)
imgproc.exe test.jpg -bc 80 2.0 test_results/out_bc.png >nul
if exist test_results/out_bc.png (echo successfully) else (echo error)

echo.
echo Результаты в папке test_results\

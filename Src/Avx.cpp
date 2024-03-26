#include <assert.h>
#include <math.h>
#include <stddef.h>
#include <SFML/Graphics.hpp>
#include <immintrin.h>

extern "C" uint64_t GetTimeStampCounter();

void     CreateWindow           (const size_t width, const size_t height, 
                                 sf::RenderWindow* outWindow, const char* windowName);

uint64_t CalculateMandelbrotSet (sf::Uint8* pixels, const size_t width, const size_t height,
                                 const float imageXShift, const float imageYShift, 
                                 const float scale, const float dxPerPixel, const float dyPerPixel);

void     DrawPixels             (sf::RenderWindow* window, sf::Uint8* pixels, 
                                 const size_t width, const size_t height);

void     ClearWindow            (sf::RenderWindow* window);

void     PollEvents             (sf::RenderWindow* window, 
                                 float* imageXShift, float* imageYShift, float* scale,
                                 const float dxPerPixel, const float dyPerPixel);

int main()
{
    static const size_t width  = 800;
    static const size_t height = 600;
    
    static const float dxPerPixel = 1.f / (float)width;
    static const float dyPerPixel = dxPerPixel;

    sf::RenderWindow window;
    CreateWindow(width, height, &window, "Mandelbrot");
 
    sf::Uint8* pixels = (sf::Uint8*)calloc(width * height * 4, sizeof(*pixels));

    float imageXShift = 0.f;
    float imageYShift = 0.f;
    float scale       = 1.f;

    uint64_t time         = 0;
    uint64_t numberOfRuns = 0;
    while (window.isOpen())
    {
        time += CalculateMandelbrotSet(pixels, width, height, imageXShift, 
                                       imageYShift, scale, dxPerPixel, dyPerPixel);

#ifndef TIME_MEASURE
        DrawPixels(&window, pixels, width, height);
#else
        numberOfRuns++;

        if (numberOfRuns == 1000)
            window.close();
#endif

        PollEvents(&window, &imageXShift, &imageYShift, &scale, dxPerPixel, dyPerPixel);
    }

#ifdef TIME_MEASURE
    printf("Runs - %zu, Time spent on one run - %zu\n", numberOfRuns, time / numberOfRuns);
#endif 

    window.clear();
    free(pixels);
}

void CreateWindow(const size_t width, const size_t height, 
                  sf::RenderWindow* outWindow, const char* windowName)
{
    outWindow->create(sf::VideoMode(width, height), windowName);
}

uint64_t CalculateMandelbrotSet(sf::Uint8* pixels, const size_t width, const size_t height,
                                const float imageXShift, const float imageYShift, 
                                const float scale, const float dxPerPixel, const float dyPerPixel)
{   
    static const float  centerX = -1.35f;
    static const float  centerY = 0.f;
    
    static const __m256 maxRadiusSquare = _mm256_set1_ps(100.f);

    static const size_t maxNumberOfIterations = 256;
    static const __m256 colorsCalculatingDivider = 
                                _mm256_set1_ps((float)maxNumberOfIterations / 255.f);

#ifdef TIME_MEASURE
    uint64_t startTime = GetTimeStampCounter();
#endif

    const float dx = dxPerPixel / scale;
    const float dy = dyPerPixel / scale;

    const __m256 pointsDeltas = _mm256_mul_ps(_mm256_set_ps(7.f, 6.f, 5.f, 4.f,
                                                            3.f, 2.f, 1.f, 0.f), 
                                              _mm256_set1_ps(dx));
                                              
          float y0Begin = -(float)height / 2 * dy + centerY + imageYShift;
    const float x0Begin = -(float)width  / 2 * dx + centerX + imageXShift;
    
    __m256 y0Avx = _mm256_set1_ps(y0Begin);
    __m256 dyAvx = _mm256_set1_ps(dy);
    for (size_t pixelY = 0; pixelY < height; ++pixelY, y0Avx = _mm256_add_ps(y0Avx, dyAvx))
    {
        float x0 = x0Begin;

        for (size_t pixelX = 0; pixelX < width; pixelX += 8, x0 += 8 * dx)
        {
            __m256i numberOfIterations = _mm256_setzero_si256();
        #ifndef BASELINE
            __m256 x0Avx = _mm256_add_ps(_mm256_set1_ps(x0), pointsDeltas);

            __m256 x = x0Avx;
            __m256 y = y0Avx;
            
            for (size_t iterationNumber = 0; iterationNumber < maxNumberOfIterations;
                 ++iterationNumber)
            {
                __m256 xSquare = _mm256_mul_ps(x, x);
                __m256 ySquare = _mm256_mul_ps(y, y); 
                __m256 xMulY   = _mm256_mul_ps(x, y); 

                __m256 radiusSquare = _mm256_add_ps(xSquare, ySquare);

                __m256 cmpRadius = _mm256_cmp_ps(radiusSquare, maxRadiusSquare, _CMP_LT_OQ);
                int mask = _mm256_movemask_ps(cmpRadius);

                if (!mask) break;

                // calculating number of iterations per each dx shift 
                numberOfIterations = _mm256_sub_epi32(numberOfIterations, 
                                                      _mm256_castps_si256(cmpRadius));

                x = _mm256_add_ps(_mm256_sub_ps(xSquare, ySquare), x0Avx);
                y = _mm256_add_ps(_mm256_add_ps(xMulY  , xMulY),   y0Avx);
            }
        
        #endif
            __m256 colors = _mm256_div_ps(_mm256_cvtepi32_ps(numberOfIterations), 
                                          colorsCalculatingDivider);

            size_t pixelPos = (pixelX + pixelY * width) * 4;

            int*   numberOfIterationsArray = (int*)(&numberOfIterations);
            float* colorsArray             = (float*)(&colors);
            for (size_t i = 0; i < 8; ++i, pixelPos += 4)
            {
                uint8_t color = (uint8_t)colorsArray[i];

                color = numberOfIterationsArray[i] == maxNumberOfIterations ? 0 : color;
                pixels[pixelPos]     = color > 122 ? color : 0;
                pixels[pixelPos + 1] = color > 122 ? 1     : color;
                pixels[pixelPos + 2] = color > 122 ? color : 0;
                pixels[pixelPos + 3] = 255;
            }
        }
    }

#ifdef TIME_MEASURE
    return GetTimeStampCounter() - startTime;
#else
    return 0;
#endif
}

void DrawPixels (sf::RenderWindow* window, sf::Uint8* pixels, 
                 const size_t width, const size_t height)
{
    sf::Texture texture;
    texture.create(width, height);
    sf::Sprite sprite;
    
    texture.update(pixels);
    sprite.setTexture(texture);

    window->draw(sprite);

    window->display();
}

void PollEvents(sf::RenderWindow* window, 
                float* imageXShift, float* imageYShift, float* scale,
                const float dxPerPixel, const float dyPerPixel)
{
    sf::Event event;
    while (window->pollEvent(event))
    {
        switch(event.type)
        {
            case sf::Event::Closed:
                window->close();
                break;
            
            case sf::Event::KeyReleased:
            {
                switch(event.key.code)
                {
                    case sf::Keyboard::Right:
                        *imageXShift += dxPerPixel * 10.f;
                        break;
                    case sf::Keyboard::Left:
                        *imageXShift -= dxPerPixel * 10.f;
                        break;
                    case sf::Keyboard::Up:
                        *imageYShift -= dyPerPixel * 10.f;
                        break;
                    case sf::Keyboard::Down:
                        *imageYShift += dyPerPixel * 10.f;
                        break;
                    case sf::Keyboard::Hyphen: // -
                        *scale -= dxPerPixel * 10.f;
                        break;
                    case sf::Keyboard::Equal: // equal on the same pos as +
                        *scale += dxPerPixel * 10.f;
                        break;

                    default:
                        break;
                
                }
            }
        }
    }
}

void ClearWindow(sf::RenderWindow* window)
{
    window->clear();
}


// 1193243
// 52355494

// 51162251

// 2.3%
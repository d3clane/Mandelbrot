#include <assert.h>
#include <math.h>
#include <stddef.h>
#include <SFML/Graphics.hpp>

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

typedef float m256 [8];
typedef int   m256i[8];

static inline void mm256_set1_ps(m256 dst, float val);
static inline void mm256_set_ps (m256 dst, float val7, float val6, float val5, 
                                           float val4, float val3, float val2, 
                                           float val1, float val0);
                                               
static inline void mm256_add_ps   (m256 dst, m256 src1, m256 src2);
static inline void mm256_mul_ps   (m256 dst, m256 src1, m256 src2);
static inline void mm256_sub_ps   (m256 dst, m256 src1, m256 src2);
static inline void mm256_div_ps   (m256 dst, m256i src1, m256 src2);
static inline void mm256_cpy_ps   (m256 dst, m256 src);

static inline void mm256_cmplt_ps (m256 dst, m256 arr1, m256 arr2);

static inline void mm256_sub_epi32(m256i dst, m256i src1, m256 src2);

static inline int  mm256_movemask_ps  (m256 src);
static inline void mm256_setzero_si256(m256i dst);

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
    
    static const size_t maxNumberOfIterations = 256;

#ifdef TIME_MEASURE
    uint64_t startTime            = GetTimeStampCounter();
    uint64_t allIterationsCounter = 0;
#endif
    const float dx = dxPerPixel / scale;
    const float dy = dyPerPixel / scale;

    static m256   maxRadiusSquare = {};
    mm256_set1_ps(maxRadiusSquare, 100.f);

    static m256  _76543210 = {};
    mm256_set_ps(_76543210, 7.f, 6.f, 5.f, 4.f, 3.f, 2.f, 1.f, 0.f);

    static m256   dxAvx = {};
    mm256_set1_ps(dxAvx, dx);

    static m256  pointsDeltas = {};
    mm256_mul_ps(pointsDeltas, dxAvx, _76543210);

    static m256   colorsCalculatingDivider = {};
    mm256_set1_ps(colorsCalculatingDivider, (float)maxNumberOfIterations / 255.f);

          float y0Begin = -(float)height / 2 * dy + centerY + imageYShift;
    const float x0Begin = -(float)width  / 2 * dx + centerX + imageXShift;
    

    m256 y0Avx     = {};
    mm256_set1_ps(y0Avx, y0Begin);

    m256 dyAvx = {};
    mm256_set1_ps(dyAvx, dy);

    for (size_t pixelY = 0; pixelY < height; ++pixelY, mm256_add_ps(y0Avx, y0Avx, dyAvx))
    {
        float x0 = x0Begin;

        for (size_t pixelX = 0; pixelX < width; pixelX += 8, x0 += 8 * dx)
        {
            m256 x0Avx = {};
            mm256_set1_ps(x0Avx, x0);
            mm256_add_ps(x0Avx, x0Avx, pointsDeltas);

            m256 x = {};
            m256 y = {};
            mm256_cpy_ps(x, x0Avx);
            mm256_cpy_ps(y, y0Avx);
            
            m256i numberOfIterations = {};
            mm256_setzero_si256(numberOfIterations);

            size_t iterationNumber = 0;
            for (iterationNumber = 0; iterationNumber < maxNumberOfIterations;
                 ++iterationNumber)
            {
                m256 xSquare = {};
                mm256_mul_ps(xSquare, x, x);
                m256 ySquare = {};
                mm256_mul_ps(ySquare, y, y); 
                m256 xMulY   = {};
                mm256_mul_ps(xMulY, x, y); 

                m256 radiusSquare = {};
                mm256_add_ps(radiusSquare, xSquare, ySquare);

                m256 cmpRadius = {}; 
                mm256_cmplt_ps(cmpRadius, radiusSquare, maxRadiusSquare);
                int mask = mm256_movemask_ps(cmpRadius);

                if (!mask) break;

                // calculating number of iterations per each dx shift 
                mm256_sub_epi32(numberOfIterations, numberOfIterations, cmpRadius);

                mm256_sub_ps(x, xSquare, ySquare); mm256_add_ps(x, x, x0Avx);
                mm256_add_ps(y, xMulY,   xMulY);   mm256_add_ps(y, y, y0Avx);
            }

        #if defined(TIME_MEASURE_PIXELS_SETTING) || !defined(TIME_MEASURE)
            m256 colors = {};
            mm256_div_ps(colors, numberOfIterations, colorsCalculatingDivider);

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
        #endif
        #ifdef TIME_MEASURE_EXTRA_VAR
            allIterationsCounter += iterationNumber;
        #endif
        }
    }

#ifdef TIME_MEASURE
    uint64_t timeSpent = GetTimeStampCounter() - startTime;
    printf("allIterationsCounter - %llu\n", allIterationsCounter);
    return timeSpent;
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
                        *imageXShift -= dxPerPixel * 10.0f;
                        break;
                    case sf::Keyboard::Up:
                        *imageYShift -= dyPerPixel * 10.0f;
                        break;
                    case sf::Keyboard::Down:
                        *imageYShift += dyPerPixel * 10.0f;
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

static inline void mm256_set1_ps(m256 dst, float val)
{
    for (size_t i = 0; i < 8; ++i) dst[i] = val;
}
static inline void mm256_set_ps (m256 dst, float val7, float val6, float val5, 
                                           float val4, float val3, float val2, 
                                           float val1, float val0)
{
    dst[0] = val0;
    dst[1] = val1;
    dst[2] = val2;
    dst[3] = val3;
    dst[4] = val4;
    dst[5] = val5;
    dst[6] = val6;
    dst[7] = val7;
}
                                               
static inline void mm256_add_ps   (m256 dst, m256 src1, m256 src2)
{
    for (size_t i = 0; i < 8; ++i) dst[i] = src1[i] + src2[i];
}

static inline void mm256_mul_ps   (m256 dst, m256 src1, m256 src2)
{
    for (size_t i = 0; i < 8; ++i) dst[i] = src1[i] * src2[i];
}

static inline void mm256_sub_ps   (m256 dst, m256 src1, m256 src2)
{
    for (size_t i = 0; i < 8; ++i) dst[i] = src1[i] - src2[i];
}

static inline void mm256_div_ps   (m256 dst, m256i src1, m256 src2)
{
    for (size_t i = 0; i < 8; ++i) dst[i] = (float)src1[i] / src2[i];
}

static inline void mm256_cpy_ps   (m256 dst, m256 src)
{
    for (size_t i = 0; i < 8; ++i) dst[i] = src[i];
}

static inline void mm256_cmplt_ps (m256 dst, m256 arr1, m256 arr2)
{
    for (size_t i = 0; i < 8; ++i) 
    {
        if (arr1[i] < arr2[i]) dst[i] = -1;
        else                   dst[i] = 0;
    }
}

static inline void mm256_sub_epi32(m256i dst, m256i src1, m256 src2)
{
    for (size_t i = 0; i < 8; ++i) dst[i] = src1[i] - (int)src2[i];
}

static inline int  mm256_movemask_ps  (m256 src)
{
    int mask = 0;
    for (size_t i = 0; i < 8; ++i) mask |= (int)src[i];

    return mask;
}

static inline void mm256_setzero_si256(m256i dst)
{
    for (size_t i = 0; i < 8; ++i) dst[i] = 0;
}
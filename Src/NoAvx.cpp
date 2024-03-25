#include <assert.h>
#include <math.h>
#include <stddef.h>
#include <SFML/Graphics.hpp>

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

extern "C" uint64_t GetTimeStampCounter();

int main()
{
    static const size_t width  = 800;
    static const size_t height = 600;
    static const float dxPerPixel      = 1.f / (float)width;
    static const float dyPerPixel      = dxPerPixel;

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
    static const size_t maxNumberOfIterations = 256;
    static const float  maxRadiusSquare       = 10 * 10.f;
    static const float  centerX               = -1.35f;
    static const float  centerY               = 0.f;

#ifdef TIME_MEASURE
    uint64_t startTime = GetTimeStampCounter();
#endif

    const float dx = dxPerPixel / scale;
    const float dy = dyPerPixel / scale;

    float y0            = -(float)height / 2 * dy + centerY + imageYShift;
    const float x0Begin = -(float)width  / 2 * dx + centerX + imageXShift;

    for (size_t pixelY = 0; pixelY < height; ++pixelY, y0 += dy)
    {
        float x0 = x0Begin;

        for (size_t pixelX = 0; pixelX < width; ++pixelX, x0 += dx)
        {
            float x = x0;
            float y = y0;

            size_t iterationNumber = 0;
            for (iterationNumber = 0; iterationNumber < maxNumberOfIterations;
                    ++iterationNumber)
            {
                const float xSquare = x * x;
                const float ySquare = y * y;
                const float xMulY   = x * y;

                float radiusSquare = xSquare + ySquare;

                if (radiusSquare >= maxRadiusSquare) break;

                x = xSquare - ySquare + x0;
                y = xMulY   + xMulY   + y0;
            }

            float color = (float)iterationNumber / (float)maxNumberOfIterations * 255.f;

            uint8_t col = iterationNumber == maxNumberOfIterations ? 0 : (uint8_t)color;

            size_t pixelPos = (pixelX + pixelY * width) * 4;

            pixels[pixelPos]     = col > 122 ? col : 0;
            pixels[pixelPos + 1] = col > 122 ? 1   : col;
            pixels[pixelPos + 2] = col > 122 ? col : 0;
            pixels[pixelPos + 3] = 255;
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
#ifndef TIMER_HPP
#define TIMER_HPP

#include <chrono>
#include <thread>

// Object used for getting the time between events. Two ways:
//     get_delta_time(): Get the time increment (deltaTime) between two consecutive calls to this function.
//     start() & end(): Get the time increment (deltaTime) between a call to start() and end(). <<<<<<<<<<<<<<<<<<<<<<
class timer
{
    // Get date and time (all in numbers)

    // Get date and time (months and week days are strings)
};

// Class used in the render loop (OpenGL, Vulkan, etc.) for different time-related purposes (frame counting, delta time, current time, fps...)
class TimerSet
{
    std::chrono::high_resolution_clock::time_point startTime;       // From origin of time
    std::chrono::high_resolution_clock::time_point prevTime;        // From origin of time
    std::chrono::high_resolution_clock::time_point currentTime;     // From origin of time

    long double deltaTime;      // From prevTime
    long double time;           // From startTime() call

    int FPS;
    int maxFPS;
    int maxPossibleFPS;

    size_t frameCounter;

public:
    TimerSet(int maxFPS = 0);           ///< Constructor. maxFPS sets a maximum FPS (0 for setting no maximum FPS)

    // Chrono methods
    void        startTimer();           ///< Start time counting for the chronometer (startTime)
    void        computeDeltaTime();     ///< Compute frame's duration (time between two calls to this) and other time values
    long double getDeltaTime();         ///< Returns time (seconds) increment between frames (deltaTime)
    long double getTime();              ///< Returns time (seconds) since startTime when computeDeltaTime() was called

    // FPS control
    int         getFPS();               ///< Get FPS (updated in computeDeltaTime())
    void        setMaxFPS(int fps);     ///< Modify the current maximum FPS. Set it to 0 to deactivate FPS control.
    int         getMaxPossibleFPS();    ///< Get the maximum possible FPS you can get (if we haven't set max FPS, maxPossibleFPS == FPS)

    // Frame counting
    size_t      getFrameCounter();      ///< Get frame number (it is incremented each time getDeltaTime() is called)

    // Bonus methods (less used)
    long double getTimeNow();           ///< Returns time (seconds) since startTime, at the moment of calling GetTimeNow()
};

#endif
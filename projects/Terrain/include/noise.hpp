#ifndef NOISE_HPP
#define NOISE_HPP

#include <array>

#include "FastNoiseLite.h"
//#include "FastNoise/FastNoise.h"
//#include "FastSIMD/FastSIMD.h"

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

//#define DEBUG_NOISE


class Noiser;
class SimpleNoise;
class Multinoise;
class FractalNoise;
class FractalNoise_Exp;
class FractalNoise_SplinePts;


/// Noise generator
class Noiser
{
public:
    Noiser() { };
    virtual ~Noiser() { };

    virtual float getNoise(float x, float y, float z) = 0;
    virtual float getNoise(float x, float y) = 0;

    /// Used for testing purposes. Checks the noise values for a size x size terrain and outputs the absolute maximum and minimum
    void noiseTester(Noiser* noiser, size_t size) const;        //!< Range: [min, max]
};


/// Single octave noise
class SimpleNoise : public Noiser
{
protected:
    FastNoiseLite noise;
    float scale;
    int seed;

public:
    SimpleNoise(FastNoiseLite::NoiseType NoiseType, float scale, int seed);
    virtual ~SimpleNoise() { };

    float getNoise(float x, float y, float z) override;
    float getNoise(float x, float y) override;
};


// Callbacks for Multinoise objects
float default3D_callback(float x, float y, float z, std::vector<std::shared_ptr<Noiser>>& noisers);
float default2D_callback(float x, float y, std::vector<std::shared_ptr<Noiser>>& noisers);
float getNoise_C_E_PV(float x, float y, float z, std::vector<std::shared_ptr<Noiser>>& noisers);

/// Noise generator that mixes outputs of different Noiser objects.
class Multinoise : public Noiser
{
    std::vector<std::shared_ptr<Noiser>> noisers;

    float(*getNoise2D_callback) (float x, float y, std::vector<std::shared_ptr<Noiser>>& noisers);            //!< Callback (stablish here how the different noises interact to produce the final noise)
    float(*getNoise3D_callback) (float x, float y, float z, std::vector<std::shared_ptr<Noiser>>& noisers);   //!< Callback (stablish here how the different noises interact to produce the final noise)

public:
    Multinoise(std::vector<std::shared_ptr<Noiser>>& noisers, float(*getNoise3D)(float, float, float, std::vector<std::shared_ptr<Noiser>>&) = default3D_callback, float(*getNoise2D)(float, float, std::vector<std::shared_ptr<Noiser>>&) = default2D_callback);
    ~Multinoise() { };

    float getNoise(float x, float y, float z) override;
    float getNoise(float x, float y) override;
};


class FractalNoise : public Noiser
{
protected:
    FastNoiseLite noise;

    FastNoiseLite::NoiseType noiseType;     //!< FastnoiseLite::NoiseType_ ... OpenSimplex2, OpenSimplex2S, Cellular, Perlin, ValueCubic, Value
    int numOctaves;                         //!< Layers with different contributions each (by default, frequency doubles and amplitude halfs)
    float lacunarity;                       //!< How quickly frequency increases with each layer (by default, frequency doubles)
    float persistence;                      //!< How quickly amplitude decreases with each layer (by default, amplitude halfs)
    float scale;                            //!< Increase noise scale
    float multiplier;                       //!< Multiply scale noise
    int seed;
    //float offsetX, offsetY, offsetZ;      //!< Translate noise
    //unsigned curveDegree;                 //!< Flatter valleys, sharper peaks. Raise unit noise to this power and multiply the result with the actual noise.
    //std::vector<std::tuple<float, float>> splinePts;    //!< Spline points (pair<noise, height)

public:
    FractalNoise(
        FastNoiseLite::NoiseType NoiseType,
        int NumOctaves,
        float Lacunarity,
        float Persistence,
        float Scale,
        float Multiplier,
        int Seed);

    virtual ~FractalNoise() { };

    virtual float getNoise(float x, float y, float z) override;
    virtual float getNoise(float x, float y) override;

    friend std::ostream& operator << (std::ostream& os, const FractalNoise& obj);
};

std::ostream& operator << (std::ostream& os, const FractalNoise& obj);


/**
    @class Noiser
    @brief Computes the coherent noise (FBM) value for specific 2D/3D coordinates. Range = [-maxHeight, maxHeight]

    Parameters used:
    <ul>
     <li>Noise type: FastnoiseLite::NoiseType_ ... OpenSimplex2, OpenSimplex2S, Cellular, Perlin, ValueCubic, Value</li>
     <li>Octaves: Number of octaves</li>
     <li>Lacunarity (>1): Determines frequency for each octave</li>
     <li>Persistence [0,1]: Determines Amplitude for each octave</li>
     <li>Scale: Scales X and Y coordinates and the noise value</li>
     <li>Multiplier: The resulting noise value is multiplied by this (affects height) after applying scaling</li>
     <li>CurveDegree: Degree of the monomial (makes height difference progressive)</li>
     <li>Offsets (XYZ): Noise translation</li>
     <li>Seed: Seed for noise generation</li>
    </ul>
*/
class FractalNoise_Exp : public FractalNoise
{
    unsigned curveDegree;                 //!< Flatter valleys, sharper peaks. Raise unit noise to this power and multiply the result with the actual noise.

public:
    FractalNoise_Exp(
        FastNoiseLite::NoiseType NoiseType,
        int NumOctaves,
        float Lacunarity,
        float Persistence,
        float Scale,
        float Multiplier,
        int Seed,
        unsigned CurveDegree);
    ~FractalNoise_Exp() { }

    // Get noise. Computations are directly performed by this method (Octaves, Lacunarity, Persistence, Offsets, Scale, Multiplier, Degree).
    float getProcessedNoise(float x, float y, float z);

    // Get noise after the full process. Computations performed by FastNoise (Octaves, Lacunarity, Persistence) and this method (Scale, Multiplier, Degree).
    float getNoise(float x, float y, float z) override;
    float getNoise(float x, float y) override;
};


class FractalNoise_SplinePts : public FractalNoise
{
    float lerp(float a, float b, float t);

    std::vector<std::array<float, 2>> splinePts;       // pair(noiseValue, finalValue)  (noise values must cover range [-1, 1])
  
    float value;    // helper
    unsigned i;     // helper

public:
    FractalNoise_SplinePts(
        FastNoiseLite::NoiseType NoiseType,
        int NumOctaves,
        float Lacunarity,
        float Persistence,
        float scale,
        int Seed,
        std::vector<std::array<float, 2>> splinePts);
    ~FractalNoise_SplinePts() { }

    float getNoise(float x, float y, float z) override;
    float getNoise(float x, float y) override;
};


// -----------------------------------------------------------------------------------

/*
*   @brief Makes a vertex buffer containing the vertex and colors for a 3D axis system in the origin
*   @param array[12][3] Pointer to the array where data will be stored (float array[12][3])
*   @param sizeOfAxis Desired length for each axis
*/
void fillAxis(float array[6][6], float sizeOfAxis);

/*
*   @brief Makes a vertex buffer containing the vertex and color for an horizontal sea surface
*   @param array[12][3] Pointer to the array where data will be stored (float array[12][7])
*   @param height Sea level
*   @param transparency Alpha channel (range: [0, 1])
*   @param x0 Origin X coordinate of the square surface
*   @param y0 Origin Y coordinate of the square surface
*   @param x1 Ending X coordinate of the square surface
*   @param y1 Ending Y coordinate of the square surface
*/
void fillSea(float array[6][10], float height, float transparency, float x0, float y0, float x1, float y1);

/*
*   @brief Makes a vertex buffer containing a sphere generated out of a cube, with a specified color
*   @param array Pointer to the array where data will be stored ( float array[6][numVertexPerSide * numVertexPerSide][6] )
*   @param indices Pointer to the array where indices will be stored ( unsigned indices[6][(numVertexPerSide-1)*(numVertexPerSide-1)*2][3] )
*   @param radius Sphere's radius
*   @param numVertexPerSide Number of vertex per side in each square side of the cube
*   @param R Red value
*   @param G Green value
*   @param B Blue value
*/
void fillCubicSphere(float*** array, unsigned*** indices, float radius, unsigned numVertexPerSide, float R, float G, float B);



#endif

#ifndef NOISE_HPP
#define NOISE_HPP

#include <random>

#include "FastNoiseLite.h"
//#include "FastNoise/FastNoise.h"
//#include "FastSIMD/FastSIMD.h"

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"


class Noiser
{
public:
    Noiser() { };
    virtual ~Noiser() { };

    virtual float GetNoise(float x, float y, float z) = 0;
    virtual float GetNoise(float x, float y) = 0;
};


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
class SingleNoise : public Noiser
{
public:
    SingleNoise(
        FastNoiseLite::NoiseType NoiseType,
        int NumOctaves,
        float Lacunarity,
        float Persistence,
        float Scale,
        float Multiplier,
        unsigned CurveDegree,
        float OffsetX, float OffsetY, float OffsetZ,
        int Seed);

    // Get noise. Computations are performed by this method (Octaves, Lacunarity, Persistence, Offsets, Scale, Multiplier, Degree).
    float getProcessedNoise(float x, float y, float z);

    // Get noise after the full process. Computations performed by FastNoise (Octaves, Lacunarity, Persistence) and this method (Offsets, Scale, Multiplier, Degree).
    float GetNoise(float x, float y, float z) override;
    float GetNoise(float x, float y) override;

    friend std::ostream& operator << (std::ostream& os, const SingleNoise& obj);

private:
    FastNoiseLite noise;

    FastNoiseLite::NoiseType noiseType;
    int numOctaves;
    float lacunarity;
    float persistence;
    float scale;
    float multiplier;
    unsigned curveDegree;
    float offsetX, offsetY, offsetZ;
    int seed;

    // Helpers
    float totalAmplitude;
    float maxHeight;
    float result;
    float frequency;
    float amplitude;

    float powLinInterp(float base, float exponent);     //!< Uses linear interpolation to get an aproximation of a base raised to a float exponent

    /*
    *  @brief Used for testing purposes. Checks the noise values for a size x size terrain and outputs the absolute maximum and minimum
    *  @param size Size of one side of the square that will be tested
    */
    void noiseTester(size_t size);
};

std::ostream& operator << (std::ostream& os, const SingleNoise& obj);

/// Takes many SingleNoise and mixes them
//class NoiseMix : public Noiser
//{
//public:
//    NoiseMix() { };
//    virtual ~MixNoise() { };
//
//    float GetNoise(float x, float y, float z) override;
//    float GetNoise(float x, float y) override;
//};

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


#include <iostream>
#include <cmath>
#include <numbers>
#include <random>

#include "noise.hpp"
#include "common.hpp"


void Noiser::noiseTester(Noiser* noiser, size_t size) const
{
    float max = 0, min = 0;
    float noise;

    for (size_t i = 1; i != size; i++)
    {
        std::cout << "%: " << 100 * (float)i / size << "     \r";   // Output computation progress
        for (size_t j = 1; j != size; j++)
        {
            noise = noiser->getNoise(i, j);
            if (noise > max) max = noise;
            else if (noise < min) min = noise;
        }
    }

    std::cout << "Range = [" << min << ", " << max << ']' << std::endl;
}

SimpleNoise::SimpleNoise(FastNoiseLite::NoiseType NoiseType, float scale, int seed)
    : noise(NoiseType), scale(scale), seed(seed)
{
    noise.SetSeed(seed);
}

float SimpleNoise::getNoise(float x, float y, float z)
{
    return noise.GetNoise(x / scale, y / scale, z / scale);
}

float SimpleNoise::getNoise(float x, float y)
{
    return noise.GetNoise(x / scale, y / scale);
}

FractalNoise::FractalNoise(FastNoiseLite::NoiseType NoiseType, int NumOctaves, float Lacunarity, float Persistence, float Scale, float Multiplier, int Seed)
    : Noiser(), noiseType(NoiseType), numOctaves(NumOctaves), lacunarity(Lacunarity), persistence(Persistence), scale(Scale), multiplier(Multiplier), seed(Seed)
{
    // Set FastNoiseLite object
    noise.SetNoiseType(noiseType);
    noise.SetSeed(seed);

    noise.SetFractalLacunarity(lacunarity);
    noise.SetFractalGain(persistence);
    noise.SetFractalOctaves(numOctaves);    // Set to 0 or 1 if getProcessedNoise is used

    noise.SetFrequency(0.01f);
    noise.SetFractalType(FastNoiseLite::FractalType::FractalType_FBm);
    noise.SetFractalWeightedStrength(0.0f);

    noise.SetFractalPingPongStrength(2.0f);
    noise.SetRotationType3D(FastNoiseLite::RotationType3D::RotationType3D_None);

    //      Cellular
    noise.SetCellularDistanceFunction(FastNoiseLite::CellularDistanceFunction::CellularDistanceFunction_EuclideanSq);
    noise.SetCellularReturnType(FastNoiseLite::CellularReturnType::CellularReturnType_Distance);
    noise.SetCellularJitter(1.0f);

    //      Domain warp
    noise.SetDomainWarpType(FastNoiseLite::DomainWarpType::DomainWarpType_OpenSimplex2);
    noise.SetDomainWarpAmp(1.0f);
    //noise.DomainWarp(FNfloat & x, FNfloat & y, FNfloat & z);

    //float totalAmplitude = 0;
    //float amplitude = 1;
    //for (int i = 0; i < numOctaves; i++)
    //{
    //    totalAmplitude += amplitude;
    //    amplitude *= persistence;
    //}

    //maxHeight = multiplier * scale;

    //std::mt19937_64 engine;
    //engine.seed(seed);

    #ifdef DEBUG_NOISE
        std::cout << *this;

        std::uniform_int_distribution<int> distribution(-10000, 10000);
        std::cout << "Random: " << distribution(engine) << ", " << distribution(engine) << ", " << distribution(engine) << std::endl;
    #endif
}

float FractalNoise::FractalNoise::getNoise(float x, float y, float z)
{
    return multiplier * scale * noise.GetNoise(x / scale, y / scale, z / scale);
}

float FractalNoise::FractalNoise::getNoise(float x, float y)
{
    return multiplier * scale * noise.GetNoise(x/scale, y/scale);
}

Multinoise::Multinoise(std::vector<std::shared_ptr<Noiser>>& noisers, float(*getNoise3D)(float, float, float, std::vector<std::shared_ptr<Noiser>>&), float(*getNoise2D)(float, float, std::vector<std::shared_ptr<Noiser>>&))
    : noisers(noisers), getNoise2D_callback(getNoise2D), getNoise3D_callback(getNoise3D) { };

float Multinoise::getNoise(float x, float y, float z) { return getNoise3D_callback(x, y, z, noisers); }

float Multinoise::getNoise(float x, float y) { return getNoise2D_callback(x, y, noisers); }

float default3D_callback(float x, float y, float z, std::vector<std::shared_ptr<Noiser>>& noisers)
{
    return noisers[0]->getNoise(x, y, z);
}

float default2D_callback(float x, float y, std::vector<std::shared_ptr<Noiser>>& noisers)
{
    return noisers[0]->getNoise(x, y);
}

float getNoise_C_E_PV(float x, float y, float z, std::vector<std::shared_ptr<Noiser>>& noisers)
{
    float continentalness = noisers[0]->getNoise(x, y, z);
    float erosion = noisers[1]->getNoise(x, y, z);         // range [0,1]
    float PV = noisers[2]->getNoise(x, y, z);

    //return continentalness;
    //return ((1 - erosion) * 2 - 1) * 200;
    //return PV;
    return 
        erosion * (
        continentalness * 200 + 
        PV * 600 * (continentalness > 0 ? continentalness : 0) );
}


std::ostream& operator << (std::ostream& os, const FractalNoise& obj)
{
    std::cout
        << "Noise data: \n"
        << "   Octaves: " << obj.numOctaves << '\n'
        //<< "   Max amplitude: " << obj.totalAmplitude << '\n'
        << "   Scale: " << obj.scale << '\n'
        << "   Multiplier: " << obj.multiplier << '\n'
        << "   Lacunarity: " << obj.lacunarity << '\n'
        << "   Persistence: " << obj.persistence << '\n';

    return os;
}

FractalNoise_Exp::FractalNoise_Exp(
    FastNoiseLite::NoiseType NoiseType,
    int NumOctaves,
    float Lacunarity,
    float Persistence,
    float Scale,
    float Multiplier,
    int Seed,
    unsigned CurveDegree)
    : FractalNoise(NoiseType, NumOctaves, Lacunarity, Persistence, Scale, Multiplier, Seed), curveDegree(CurveDegree) { }

float FractalNoise_Exp::getProcessedNoise(float x, float y, float z)
{
    float result = 0;
    float frequency = 1;
    float amplitude = 1;
    float totalAmplitude = 0;   // Used for normalizing result to [-1.0, 1.0]
    float maxHeight = multiplier * scale;

    for (int i = 0; i < numOctaves; i++)
    {
        result += noise.GetNoise(               // GetNoise returns values in range [-1, 1]
            frequency * x / scale,
            frequency * y / scale,
            frequency * z / scale) * amplitude;

        totalAmplitude += amplitude;
        amplitude *= persistence;
        frequency *= lacunarity;
    }

    result = multiplier * scale * result / totalAmplitude;
    return result * std::pow(result / maxHeight, curveDegree);
}

float FractalNoise_Exp::getNoise(float x, float y, float z)
{
    return multiplier * scale * std::pow(noise.GetNoise(x / scale, y / scale, z / scale), curveDegree);
    //result = multiplier * scale * noise.GetNoise((x + offsetX) / scale, (y + offsetY) / scale, (z + offsetZ) / scale) / totalAmplitude;
    //return result * std::pow(result / maxHeight, curveDegree);
}

float FractalNoise_Exp::getNoise(float x, float y)
{
    return multiplier * scale * std::pow(noise.GetNoise(x / scale, y / scale), curveDegree);
    //result = multiplier * scale * noise.GetNoise((x + offsetX) / scale, (y + offsetY) / scale) / totalAmplitude;
    //return result * std::pow(result / maxHeight, curveDegree);
}

FractalNoise_SplinePts::FractalNoise_SplinePts(
    FastNoiseLite::NoiseType NoiseType,
    int NumOctaves,
    float Lacunarity,
    float Persistence,
    float scale,
    int Seed,
    std::vector<std::array<float, 2>> splinePts)
    : FractalNoise(NoiseType, NumOctaves, Lacunarity, Persistence, scale, 1, Seed), splinePts(splinePts)
{
    // If the spline points provided are not enough or don't cover range [-1, 1], you will get noise == 0.
    if (splinePts.size() < 2)
    {
        std::cout << "Not enough spline points provided!" << std::endl;
        splinePts = { {-1, 0}, {1, 0} };
    }
    else if (splinePts[0][0] != -1.f || splinePts[splinePts.size() - 1][0] != 1.f)
    {
        std::cout << "The provided spline points don't cover range [-1, 1]!" << std::endl;
        splinePts = { {-1, 0}, {1, 0} };
    }
}

float FractalNoise_SplinePts::lerp(float a, float b, float t) { return a + (b - a) * t; }

float FractalNoise_SplinePts::getNoise(float x, float y, float z)
{
    value = noise.GetNoise(x * scale, y * scale, z * scale);

    for (i = 1; i < splinePts.size(); i++)
        if (value <= splinePts[i][0])
            return lerp(splinePts[i-1][1], splinePts[i][1], (value - splinePts[i - 1][0]) / (splinePts[i][0] - splinePts[i - 1][0]));   // get interpolated value

    return 0;
    //return multiplier * scale * noise.GetNoise(x / scale, y / scale, z / scale);
}

float FractalNoise_SplinePts::getNoise(float x, float y)
{
    return multiplier * scale * noise.GetNoise(x / scale, y / scale);
}


// ----------------------------------------------------------------------------------

void fillAxis(float array[6][6], float sizeOfAxis)
{
    array[0][0] = 0.f;  // First vertex
    array[0][1] = 0.f;
    array[0][2] = 0.f;
    array[0][3] = 1.f;  // Color
    array[0][4] = 0.f;
    array[0][5] = 0.f;

    array[1][0] = sizeOfAxis;
    array[1][1] = 0.f;
    array[1][2] = 0.f;
    array[1][3] = 1.f;
    array[1][4] = 0.f;
    array[1][5] = 0.f;

    array[2][0] = 0.f;
    array[2][1] = 0.f;
    array[2][2] = 0.f;
    array[2][3] = 0.f;
    array[2][4] = 1.f;
    array[2][5] = 0.f;

    array[3][0] = 0.f;
    array[3][1] = sizeOfAxis;
    array[3][2] = 0.f;
    array[3][3] = 0.f;
    array[3][4] = 1.f;
    array[3][5] = 0.f;

    array[4][0] = 0.f;
    array[4][1] = 0.f;
    array[4][2] = 0.f;
    array[4][3] = 0.f;
    array[4][4] = 0.f;
    array[4][5] = 1.f;

    array[5][0] = 0.f;
    array[5][1] = 0.f;
    array[5][2] = sizeOfAxis;
    array[5][3] = 0.f;
    array[5][4] = 0.f;
    array[5][5] = 1.f;
}

void fillSea(float array[6][10], float height, float transparency, float x0, float y0, float x1, float y1)
{
    // 3 vertex position + 4 colors + 3 normal = 10 parameters

    float color[4] = { 0.1f, 0.1f, 0.8f, transparency };

    array[0][0] = x0;
    array[0][1] = y0;
    array[0][2] = height;
    array[0][3] = color[0];
    array[0][4] = color[1];
    array[0][5] = color[2];
    array[0][6] = color[3];
    array[0][7] = 0;
    array[0][8] = 0;
    array[0][9] = 1;

    array[1][0] = x1;
    array[1][1] = y0;
    array[1][2] = height;
    array[1][3] = color[0];
    array[1][4] = color[1];
    array[1][5] = color[2];
    array[1][6] = color[3];
    array[1][7] = 0;
    array[1][8] = 0;
    array[1][9] = 1;

    array[2][0] = x0;
    array[2][1] = y1;
    array[2][2] = height;
    array[2][3] = color[0];
    array[2][4] = color[1];
    array[2][5] = color[2];
    array[2][6] = color[3];
    array[2][7] = 0;
    array[2][8] = 0;
    array[2][9] = 1;

    array[3][0] = x1;
    array[3][1] = y0;
    array[3][2] = height;
    array[3][3] = color[0];
    array[3][4] = color[1];
    array[3][5] = color[2];
    array[3][6] = color[3];
    array[3][7] = 0;
    array[3][8] = 0;
    array[3][9] = 1;

    array[4][0] = x1;
    array[4][1] = y1;
    array[4][2] = height;
    array[4][3] = color[0];
    array[4][4] = color[1];
    array[4][5] = color[2];
    array[4][6] = color[3];
    array[4][7] = 0;
    array[4][8] = 0;
    array[4][9] = 1;

    array[5][0] = x0;
    array[5][1] = y1;
    array[5][2] = height;
    array[5][3] = color[0];
    array[5][4] = color[1];
    array[5][5] = color[2];
    array[5][6] = color[3];
    array[5][7] = 0;
    array[5][8] = 0;
    array[5][9] = 1;
}

// <<< Is this useful?
void fillCubicSphere(float*** array, unsigned*** indices, float radius, unsigned numVertexPerSide, float R, float G, float B)
{
    float *** vertex = array;        //float array[6][numVertexPerSide * numVertexPerSide][6]
    unsigned *** ind = indices;      //unsigned indices[6][(numVertexPerSide-1)*(numVertexPerSide-1)*2][3]

    // Get position, color and normals

    for(size_t i = 0; i < numVertexPerSide; i++)
    {

    }




    // Get indices




}


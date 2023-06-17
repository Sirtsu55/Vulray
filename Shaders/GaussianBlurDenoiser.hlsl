
[[vk::binding(0, 0)]] [[vk::combinedImageSampler]] Texture2D<float4> inputImage;
[[vk::binding(0, 0)]] [[vk::combinedImageSampler]] SamplerState inputSampler;

[[vk::binding(1, 0)]] RWTexture2D<float4> outputImage;

struct Settings
{
    uint2 ImageSize;
    uint Radius;
    float Sigma;
};

[[vk::push_constant]] Settings settings;


float CalculateGaussian(float x, float y)
{
    const float sigma = settings.Sigma;
    float g = 1.0f / (2.0f * 3.14159265358979323846f * sigma * sigma);
    g *= exp(-(x * x + y * y) / (2.0f * sigma * sigma));
    return g;
}

[numthreads(16, 16, 1)]
void GaussianBlurDenoiser_main(uint3 threadID : SV_DispatchThreadID)
{
    float2 outUV = float2(threadID.x, threadID.y);

    float4 color = float4(0.0f, 0.0f, 0.0f, 0.0f);

    const int kernelSize = settings.Radius;

    float normalizationFactor = 0.0f;

    for (int x = -kernelSize; x <= kernelSize; x++)
    {
        for (int y = -kernelSize; y <= kernelSize; y++)
        {
            float weight = CalculateGaussian(x, y);
            normalizationFactor += weight;
            float2 uv = threadID.xy + float2(x, y);
            uv = uv / float2(settings.ImageSize);
            color += inputImage.SampleLevel(inputSampler, uv, 0.0f) * weight;
        }
    }

    color = saturate(color / normalizationFactor);

    outputImage[outUV] = color;
}
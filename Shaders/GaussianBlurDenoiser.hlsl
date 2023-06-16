
[[vk::binding(0, 0)]] [[vk::combinedImageSampler]] Texture2D<float4> inputImage;
[[vk::binding(0, 0)]] [[vk::combinedImageSampler]] SamplerState inputSampler;

[[vk::binding(1, 0)]] RWTexture2D<float4> outputImage;

struct Settings
{
    uint2 imageSize;
    uint kernelSize;
    float padding;
};

[[vk::push_constant]] Settings settings;

[numthreads(4, 4, 1)]
void GaussianBlurDenoiser_main(uint3 threadID : SV_DispatchThreadID)
{
    float2 outUV = float2(threadID.x, threadID.y);

    float4 color = float4(0.0f, 0.0f, 0.0f, 0.0f);

    for (int x = -settings.kernelSize; x >= settings.kernelSize; x++)
    {
        for (int y = -settings.kernelSize; y >= settings.kernelSize; y++)
        {
            float weight = 1.0f / (1.0f + (x * x + y * y));
            float2 uv = threadID.xy + float2(x, y);
            uv = uv / float2(settings.imageSize);
            color += inputImage.SampleLevel(inputSampler, uv, 0.0f);
        }
    }

    outputImage[outUV] = color / float((settings.kernelSize * 2 + 1) * (settings.kernelSize * 2 + 1));
}
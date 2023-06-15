
[[vk::binding(0, 0)]] [[vk::combinedImageSampler]] Texture2D<float3> inputImage;
[[vk::binding(0, 0)]] [[vk::combinedImageSampler]] SamplerState inputSampler;

[[vk::binding(0, 1)]] RWTexture2D<float3> outputImage;



[numthreads(1, 1, 1)]
void MedianDenoiser(uint3 threadID : SV_DispatchThreadID )
{
    float2 uv = float2(threadID.x, threadID.y) / float2(1024, 1024);
    outputImage[uv.xy] = inputImage.SampleLevel(inputSampler, uv.xy, 0);
}
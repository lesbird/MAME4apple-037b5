//
//  MameShaders.metal
//  MAME4apple
//
//  Minimal textured-quad pipeline for presenting the emulated frame.
//  The quad's NDC positions and UVs are computed on the CPU each frame
//  (aspect-fit / integer-scale), so the shaders are pass-through.
//

#include <metal_stdlib>
using namespace metal;

struct MameVertex
{
    float2 position; // clip-space (NDC)
    float2 uv;       // texture coordinate
};

struct RasterData
{
    float4 position [[position]];
    float2 uv;
};

vertex RasterData mame_vertex(uint vertexID [[vertex_id]],
                              constant MameVertex *verts [[buffer(0)]])
{
    RasterData out;
    out.position = float4(verts[vertexID].position, 0.0, 1.0);
    out.uv = verts[vertexID].uv;
    return out;
}

fragment float4 mame_fragment(RasterData in [[stage_in]],
                              texture2d<float> tex [[texture(0)]],
                              sampler samp [[sampler(0)]])
{
    return tex.sample(samp, in.uv);
}

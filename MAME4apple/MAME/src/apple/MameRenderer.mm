//
//  MameRenderer.mm
//  MAME4apple
//
//  Metal renderer + triple-buffered frame handoff. See MameRenderer.h.
//

#import "MameRenderer.h"

#if !TARGET_OS_TV

#import <simd/simd.h>
#import <os/lock.h>

// ---------------------------------------------------------------------------
// Triple buffer shared between the emulator thread (producer) and the MTKView
// draw callback on the main thread (consumer). A tiny lock guards the index
// swap and (re)allocation; the emulator thread never blocks on the display.
// ---------------------------------------------------------------------------

typedef struct { float position[2]; float uv[2]; } MameVertex;

// Shader source is compiled at runtime (newLibraryWithSource) so the build has
// no dependency on the offline Metal toolchain. It is a trivial pass-through
// textured-quad pipeline; see also MameShaders.metal (kept for reference).
static NSString *const kMameShaderSource = @R"METAL(
#include <metal_stdlib>
using namespace metal;

struct MameVertex { float2 position; float2 uv; };
struct RasterData { float4 position [[position]]; float2 uv; };

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
)METAL";

static os_unfair_lock sLock = OS_UNFAIR_LOCK_INIT;

static uint32_t *sBuf[3] = { NULL, NULL, NULL };
static int  sTexW = 0, sTexH = 0;
static int  sBackIdx = 0, sReadyIdx = 1, sFrontIdx = 2;
static int  sReadyActiveW = 0, sReadyActiveH = 0;
static int  sFrontActiveW = 0, sFrontActiveH = 0;
static bool sHasNew = false;
static bool sConfigured = false;
static int  sPixelAspectY = 1;

static id<MTLDevice>  sDevice = nil;
static id<MTLTexture> sTexture = nil;

// Strong: the MTKView is retained by its superview, but this renderer object is
// the view's (weak) delegate and owns the pipeline/queue — nothing else retains
// it, so a weak ref here would let it deallocate right after viewDidLoad and the
// view would render nothing (black).
static MameRenderer *sShared = nil;

@implementation MameRenderer
{
    id<MTLCommandQueue>        _queue;
    id<MTLRenderPipelineState> _pipeline;
    id<MTLSamplerState>        _nearest;
    id<MTLSamplerState>        _linear;
}

+ (instancetype)shared { return sShared; }
+ (void)setShared:(MameRenderer *)renderer { sShared = renderer; }

- (instancetype)initWithFrame:(CGRect)frame
{
    if ((self = [super init]))
    {
        _crispScaling = YES;
        _integerScaling = YES;

        sDevice = MTLCreateSystemDefaultDevice();
        NSAssert(sDevice, @"Metal is not supported on this device");

        MTKView *v = [[MTKView alloc] initWithFrame:frame device:sDevice];
        v.colorPixelFormat = MTLPixelFormatBGRA8Unorm;
        v.framebufferOnly = YES;
        v.opaque = YES;
        v.clearColor = MTLClearColorMake(0, 0, 0, 1);
        v.autoResizeDrawable = YES;
        v.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
        v.enableSetNeedsDisplay = NO;
        v.paused = YES;      // driven by its own display link only while a game runs
        v.hidden = YES;
        v.delegate = self;
        _view = v;

        _queue = [sDevice newCommandQueue];

        [self buildPipeline];
        [self buildSamplers];
    }
    return self;
}

- (void)buildPipeline
{
    NSError *libErr = nil;
    id<MTLLibrary> lib = [sDevice newLibraryWithSource:kMameShaderSource
                                               options:nil
                                                 error:&libErr];
    NSAssert(lib, @"Metal shader compile failed: %@", libErr);

    MTLRenderPipelineDescriptor *desc = [[MTLRenderPipelineDescriptor alloc] init];
    desc.vertexFunction = [lib newFunctionWithName:@"mame_vertex"];
    desc.fragmentFunction = [lib newFunctionWithName:@"mame_fragment"];
    desc.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm;

    NSError *err = nil;
    _pipeline = [sDevice newRenderPipelineStateWithDescriptor:desc error:&err];
    NSAssert(_pipeline, @"pipeline failed: %@", err);
}

- (void)buildSamplers
{
    MTLSamplerDescriptor *s = [[MTLSamplerDescriptor alloc] init];
    s.sAddressMode = MTLSamplerAddressModeClampToEdge;
    s.tAddressMode = MTLSamplerAddressModeClampToEdge;

    s.minFilter = MTLSamplerMinMagFilterNearest;
    s.magFilter = MTLSamplerMinMagFilterNearest;
    _nearest = [sDevice newSamplerStateWithDescriptor:s];

    s.minFilter = MTLSamplerMinMagFilterLinear;
    s.magFilter = MTLSamplerMinMagFilterLinear;
    _linear = [sDevice newSamplerStateWithDescriptor:s];
}

#pragma mark - MTKViewDelegate

- (void)mtkView:(MTKView *)view drawableSizeWillChange:(CGSize)size
{
    // Quad is recomputed every frame from the drawable size; nothing to do.
}

- (void)drawInMTKView:(MTKView *)view
{
    id<MTLTexture> tex = nil;
    int aw = 0, ah = 0, texW = 0, texH = 0, aspectY = 1;

    os_unfair_lock_lock(&sLock);
    if (sConfigured)
    {
        if (sHasNew)
        {
            int tmp = sFrontIdx; sFrontIdx = sReadyIdx; sReadyIdx = tmp;
            sFrontActiveW = sReadyActiveW;
            sFrontActiveH = sReadyActiveH;
            sHasNew = false;
        }
        if (sTexture && sFrontActiveW > 0 && sFrontActiveH > 0)
        {
            MTLRegion region = MTLRegionMake2D(0, 0, sFrontActiveW, sFrontActiveH);
            [sTexture replaceRegion:region
                        mipmapLevel:0
                          withBytes:sBuf[sFrontIdx]
                        bytesPerRow:sTexW * 4];
        }
        tex = sTexture;
        aw = sFrontActiveW; ah = sFrontActiveH;
        texW = sTexW; texH = sTexH;
        aspectY = sPixelAspectY;
    }
    os_unfair_lock_unlock(&sLock);

    id<CAMetalDrawable> drawable = view.currentDrawable;
    MTLRenderPassDescriptor *pass = view.currentRenderPassDescriptor;
    if (!drawable || !pass) return;

    id<MTLCommandBuffer> cmd = [_queue commandBuffer];
    id<MTLRenderCommandEncoder> enc = [cmd renderCommandEncoderWithDescriptor:pass];

    if (tex && aw > 0 && ah > 0)
    {
        MameVertex verts[4];
        [self buildQuad:verts
              drawable:view.drawableSize
              activeW:aw activeH:ah
                 texW:texW texH:texH
              aspectY:aspectY];

        [enc setRenderPipelineState:_pipeline];
        [enc setVertexBytes:verts length:sizeof(verts) atIndex:0];
        [enc setFragmentTexture:tex atIndex:0];
        [enc setFragmentSamplerState:(_crispScaling ? _nearest : _linear) atIndex:0];
        [enc drawPrimitives:MTLPrimitiveTypeTriangleStrip vertexStart:0 vertexCount:4];
    }

    [enc endEncoding];
    [cmd presentDrawable:drawable];
    [cmd commit];
}

- (void)buildQuad:(MameVertex *)v
         drawable:(CGSize)drawable
          activeW:(int)aw activeH:(int)ah
             texW:(int)texW texH:(int)texH
          aspectY:(int)aspectY
{
    double dw = drawable.width, dh = drawable.height;
    if (dw <= 0 || dh <= 0) { dw = 1; dh = 1; }

    double gw = (double)aw;
    double gh = (double)ah * (double)aspectY;   // logical size after pixel-aspect

    double scale = fmin(dw / gw, dh / gh);
    if (_integerScaling && scale >= 1.0)
        scale = floor(scale);
    if (scale <= 0.0) scale = fmin(dw / gw, dh / gh);

    double qw = gw * scale;
    double qh = gh * scale;

    float x = (float)(qw / dw);   // half-width in NDC
    float y = (float)(qh / dh);   // half-height in NDC

    float u1 = (texW > 0) ? (float)aw / (float)texW : 1.0f;
    float w1 = (texH > 0) ? (float)ah / (float)texH : 1.0f;

    // Triangle strip: TL, BL, TR, BR. Texture row 0 is the top of the game,
    // so the top (+y) vertices sample v = 0.
    v[0] = (MameVertex){ { -x,  y }, { 0.0f, 0.0f } };  // top-left
    v[1] = (MameVertex){ { -x, -y }, { 0.0f, w1   } };  // bottom-left
    v[2] = (MameVertex){ {  x,  y }, { u1,   0.0f } };  // top-right
    v[3] = (MameVertex){ {  x, -y }, { u1,   w1   } };  // bottom-right
}

@end

// ---------------------------------------------------------------------------
// C handoff API (declared in MameShared.h)
// ---------------------------------------------------------------------------

void mame_renderer_configure(int width, int height)
{
    if (width <= 0 || height <= 0) return;

    id<MTLTexture> newTex = nil;
    if (sDevice)
    {
        MTLTextureDescriptor *td =
            [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA8Unorm
                                                               width:width
                                                              height:height
                                                           mipmapped:NO];
        td.usage = MTLTextureUsageShaderRead;
        td.storageMode = MTLStorageModeShared;
        newTex = [sDevice newTextureWithDescriptor:td];
    }

    os_unfair_lock_lock(&sLock);
    for (int i = 0; i < 3; i++)
    {
        free(sBuf[i]);
        sBuf[i] = (uint32_t *)calloc((size_t)width * height, sizeof(uint32_t));
    }
    sTexW = width; sTexH = height;
    sBackIdx = 0; sReadyIdx = 1; sFrontIdx = 2;
    sReadyActiveW = sReadyActiveH = 0;
    sFrontActiveW = width; sFrontActiveH = height;
    sHasNew = false;
    sTexture = newTex;
    sConfigured = true;
    os_unfair_lock_unlock(&sLock);

    dispatch_async(dispatch_get_main_queue(), ^{
        MameRenderer *r = [MameRenderer shared];
        r.view.hidden = NO;
        r.view.paused = NO;
    });
}

uint32_t *mame_renderer_backbuffer(void)
{
    return sConfigured ? sBuf[sBackIdx] : NULL;
}

void mame_renderer_publish(int active_width, int active_height)
{
    os_unfair_lock_lock(&sLock);
    if (sConfigured)
    {
        int tmp = sReadyIdx; sReadyIdx = sBackIdx; sBackIdx = tmp;
        sReadyActiveW = active_width;
        sReadyActiveH = active_height;
        sHasNew = true;
    }
    os_unfair_lock_unlock(&sLock);
}

void mame_renderer_hide(void)
{
    dispatch_async(dispatch_get_main_queue(), ^{
        MameRenderer *r = [MameRenderer shared];
        r.view.paused = YES;
        r.view.hidden = YES;
    });
}

void mame_renderer_set_pixel_aspect_y(int scale)
{
    os_unfair_lock_lock(&sLock);
    sPixelAspectY = (scale >= 1) ? scale : 1;
    os_unfair_lock_unlock(&sLock);
}

#endif /* !TARGET_OS_TV */

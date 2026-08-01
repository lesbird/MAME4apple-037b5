//
//  iOS_sound.m
//  MAME4apple
//
//  Created by Les Bird on 9/20/16.
//  Rewritten for iOS: AVAudioEngine + AVAudioSourceNode fed by a lock-free
//  single-producer / single-consumer ring buffer. The emulator thread is the
//  producer (osd_update_audio_stream); the real-time audio render block is the
//  consumer. Under-runs emit silence and over-runs drop the newest samples, so
//  timing jitter can never corrupt or desync the stream (the old unsynchronised
//  ring was the source of the crackle). tvOS keeps the original AudioQueue path.
//

#import <TargetConditionals.h>
#import <Foundation/Foundation.h>
#include "driver.h"

#if !TARGET_OS_TV

#import <AVFoundation/AVFoundation.h>
#import "MameShared.h"
#include <stdatomic.h>

// --- lock-free SPSC ring of interleaved INT16 samples --------------------
#define RING_SHORTS   32768u            // power of two; ~0.37s @ 44.1k stereo
#define RING_MASK     (RING_SHORTS - 1u)

static int16_t          g_ring[RING_SHORTS];
static _Atomic uint32_t g_wr = 0;       // total shorts written (monotonic)
static _Atomic uint32_t g_rd = 0;       // total shorts read (monotonic)

static int              g_channels = 1;
static double           g_sample_rate = 22050.0;
static UINT32           g_samplesPerFrame = 0;
static int              g_attenuation = 0;   // MAME master volume, in dB (<=0)
static int              g_enabled = 1;

static AVAudioEngine     *g_engine = nil;
static AVAudioSourceNode *g_srcNode = nil;
static id                 g_interruptObserver = nil;
static id                 g_routeObserver = nil;

static inline float attenuation_to_linear(int db)
{
    if (db <= -32) return 0.0f;
    return powf(10.0f, (float)db / 20.0f);
}

// Consumer (real-time audio thread): pull up to `needed` shorts, zero-fill the
// remainder on under-run. No locks / no allocation here.
static void ring_read(int16_t *dst, uint32_t needed)
{
    uint32_t rd = atomic_load_explicit(&g_rd, memory_order_relaxed);
    uint32_t wr = atomic_load_explicit(&g_wr, memory_order_acquire);
    uint32_t used = wr - rd;
    uint32_t n = (needed < used) ? needed : used;

    for (uint32_t i = 0; i < n; i++)
        dst[i] = g_ring[(rd + i) & RING_MASK];
    for (uint32_t i = n; i < needed; i++)
        dst[i] = 0;                     // under-run -> silence

    atomic_store_explicit(&g_rd, rd + n, memory_order_release);
}

// Producer (emulator thread): write what fits, drop the newest on over-run.
static void ring_write(const int16_t *src, uint32_t count)
{
    uint32_t wr = atomic_load_explicit(&g_wr, memory_order_relaxed);
    uint32_t rd = atomic_load_explicit(&g_rd, memory_order_acquire);
    uint32_t used = wr - rd;
    uint32_t space = RING_SHORTS - used;
    uint32_t n = (count < space) ? count : space;

    for (uint32_t i = 0; i < n; i++)
        g_ring[(wr + i) & RING_MASK] = src[i];

    atomic_store_explicit(&g_wr, wr + n, memory_order_release);
}

static void configure_audio_session(void)
{
    AVAudioSession *session = [AVAudioSession sharedInstance];
    NSError *err = nil;
    [session setCategory:AVAudioSessionCategoryPlayback
                    mode:AVAudioSessionModeDefault
                 options:0
                   error:&err];
    if (err) NSLog(@"AVAudioSession setCategory error: %@", err);
    [session setActive:YES error:&err];
    if (err) NSLog(@"AVAudioSession setActive error: %@", err);
}

static void register_notifications(void)
{
    AVAudioSession *session = [AVAudioSession sharedInstance];
    NSNotificationCenter *nc = [NSNotificationCenter defaultCenter];

    g_interruptObserver =
    [nc addObserverForName:AVAudioSessionInterruptionNotification
                    object:session
                     queue:[NSOperationQueue mainQueue]
                usingBlock:^(NSNotification *note) {
        NSInteger type = [note.userInfo[AVAudioSessionInterruptionTypeKey] integerValue];
        if (type == AVAudioSessionInterruptionTypeBegan)
        {
            // e.g. phone call: park emulation and stop the engine.
            mame_pause_set(1);
            [g_engine pause];
        }
        else
        {
            NSInteger opt = [note.userInfo[AVAudioSessionInterruptionOptionKey] integerValue];
            configure_audio_session();
            NSError *err = nil;
            if (g_engine && !g_engine.isRunning)
                [g_engine startAndReturnError:&err];
            if (opt & AVAudioSessionInterruptionOptionShouldResume)
                mame_pause_set(0);
        }
    }];

    g_routeObserver =
    [nc addObserverForName:AVAudioSessionRouteChangeNotification
                    object:session
                     queue:[NSOperationQueue mainQueue]
                usingBlock:^(NSNotification *note) {
        NSInteger reason = [note.userInfo[AVAudioSessionRouteChangeReasonKey] integerValue];
        // Headphones unplugged: pause so we don't blast the speaker.
        if (reason == AVAudioSessionRouteChangeReasonOldDeviceUnavailable)
            mame_pause_set(1);
    }];
}

static void unregister_notifications(void)
{
    NSNotificationCenter *nc = [NSNotificationCenter defaultCenter];
    if (g_interruptObserver) { [nc removeObserver:g_interruptObserver]; g_interruptObserver = nil; }
    if (g_routeObserver)     { [nc removeObserver:g_routeObserver];     g_routeObserver = nil; }
}

int osd_start_audio_stream(int stereo)
{
    if (stereo) stereo = 1;

    if (Machine->sample_rate == 0)
        Machine->sample_rate = 22050;

    g_channels = stereo ? 2 : 1;
    g_sample_rate = (double)Machine->sample_rate;
    g_samplesPerFrame = Machine->sample_rate / Machine->drv->frames_per_second;

    // reset the ring
    atomic_store_explicit(&g_wr, 0, memory_order_relaxed);
    atomic_store_explicit(&g_rd, 0, memory_order_relaxed);
    memset(g_ring, 0, sizeof(g_ring));

    NSLog(@"## AVAudioEngine start: %.0f Hz, %d ch, %u samples/frame ##",
          g_sample_rate, g_channels, g_samplesPerFrame);

    configure_audio_session();

    AVAudioFormat *fmt =
        [[AVAudioFormat alloc] initWithCommonFormat:AVAudioPCMFormatInt16
                                         sampleRate:g_sample_rate
                                           channels:(AVAudioChannelCount)g_channels
                                        interleaved:YES];

    g_srcNode = [[AVAudioSourceNode alloc] initWithFormat:fmt
        renderBlock:^OSStatus(BOOL *isSilence,
                              const AudioTimeStamp *ts,
                              AVAudioFrameCount frameCount,
                              AudioBufferList *outputData) {
            if (outputData->mNumberBuffers < 1) return noErr;
            AudioBuffer *ab = &outputData->mBuffers[0];
            int16_t *dst = (int16_t *)ab->mData;
            uint32_t maxShorts = ab->mDataByteSize / sizeof(int16_t);
            uint32_t needed = frameCount * (uint32_t)g_channels;
            if (needed > maxShorts) needed = maxShorts;

            if (!g_enabled)
            {
                memset(dst, 0, needed * sizeof(int16_t));
                if (isSilence) *isSilence = YES;
                return noErr;
            }
            ring_read(dst, needed);
            return noErr;
        }];

    g_engine = [[AVAudioEngine alloc] init];
    [g_engine attachNode:g_srcNode];
    [g_engine connect:g_srcNode to:g_engine.mainMixerNode format:fmt];
    g_engine.mainMixerNode.outputVolume = attenuation_to_linear(g_attenuation);

    NSError *err = nil;
    if (![g_engine startAndReturnError:&err])
        NSLog(@"## AVAudioEngine start failed: %@ ##", err);

    register_notifications();

    return g_samplesPerFrame;
}

int osd_update_audio_stream(INT16 *buffer)
{
    uint32_t count = g_samplesPerFrame * (uint32_t)g_channels;
    ring_write((const int16_t *)buffer, count);
    return g_samplesPerFrame;
}

void osd_stop_audio_stream(void)
{
    unregister_notifications();

    if (g_engine)
    {
        [g_engine stop];
        if (g_srcNode) [g_engine detachNode:g_srcNode];
    }
    g_srcNode = nil;
    g_engine = nil;

    NSError *err = nil;
    [[AVAudioSession sharedInstance] setActive:NO
                                  withOptions:AVAudioSessionSetActiveOptionNotifyOthersOnDeactivation
                                        error:&err];

    atomic_store_explicit(&g_wr, 0, memory_order_relaxed);
    atomic_store_explicit(&g_rd, 0, memory_order_relaxed);
}

// MAME passes attenuation in dB (<= 0, 0 == full volume).
void osd_set_mastervolume(int attenuation)
{
    g_attenuation = attenuation;
    if (g_engine)
        g_engine.mainMixerNode.outputVolume = attenuation_to_linear(attenuation);
}

int osd_get_mastervolume(void)
{
    return g_attenuation;
}

void osd_sound_enable(int enable)
{
    g_enabled = enable ? 1 : 0;
}

void osd_opl_control(int chip, int reg) {}
void osd_opl_write(int chip, int data) {}

#else  // TARGET_OS_TV : original AudioQueue implementation

#include <AudioToolbox/AudioQueue.h>
#include <AudioToolbox/AudioFile.h>
#include <AudioToolbox/AudioFormat.h>
#include "GameScene.h"

static const int kNumberBuffers = 3;
struct AQPlayerState
{
    AudioStreamBasicDescription   mDataFormat;
    AudioQueueRef                 mQueue;
    AudioQueueBufferRef           mBuffers[kNumberBuffers];
};

struct AQPlayerState audioState;

#define BUFFER_CACHE_SIZE (4096 * 3)
UINT8 bufferCache[BUFFER_CACHE_SIZE];

UINT32 bufferOutOffset;
UINT32 bufferInOffset;
UINT32 samplesPerFrame;
UINT32 bytesPerFrame;

static void AudioBufferCallback(void *inUserData, AudioQueueRef inAQ, AudioQueueBufferRef inCompleteAQBuffer)
{
    unsigned char *coreAudioBuffer;
    coreAudioBuffer = (unsigned char*) inCompleteAQBuffer->mAudioData;

    memset(coreAudioBuffer, 0, bytesPerFrame);

    if (bufferOutOffset != bufferInOffset)
    {
        memcpy(coreAudioBuffer, &bufferCache[bufferOutOffset], bytesPerFrame);
        bufferOutOffset += bytesPerFrame;
        if (bufferOutOffset + bytesPerFrame >= BUFFER_CACHE_SIZE)
        {
            bufferOutOffset = 0;
        }
    }

    AudioQueueEnqueueBuffer(inAQ, inCompleteAQBuffer, 0, NULL);
}

int osd_start_audio_stream(int stereo)
{
    if (stereo) stereo = 1;

    if (Machine->sample_rate == 0)
    {
        Machine->sample_rate = 22050;
    }

    samplesPerFrame = Machine->sample_rate / Machine->drv->frames_per_second;

    audioState.mDataFormat.mBitsPerChannel = 16;
    audioState.mDataFormat.mBytesPerFrame = (stereo ? 4 : 2);
    audioState.mDataFormat.mBytesPerPacket = audioState.mDataFormat.mBytesPerFrame;
    audioState.mDataFormat.mChannelsPerFrame = (stereo ? 2 : 1);
    audioState.mDataFormat.mFormatFlags = kLinearPCMFormatFlagIsSignedInteger | kAudioFormatFlagIsPacked;
    audioState.mDataFormat.mFormatID = kAudioFormatLinearPCM;
    audioState.mDataFormat.mFramesPerPacket = 1;
    audioState.mDataFormat.mSampleRate = Machine->sample_rate;
    audioState.mDataFormat.mReserved = 0;

    bufferInOffset = 0;
    bufferOutOffset = 0;

    bytesPerFrame = samplesPerFrame * audioState.mDataFormat.mBytesPerFrame;

    INT32 result = AudioQueueNewOutput(&audioState.mDataFormat, AudioBufferCallback, NULL, NULL, kCFRunLoopCommonModes, 0, &audioState.mQueue);
    if (result == noErr)
    {
        for (int i = 0; i < kNumberBuffers; i++)
        {
            AudioQueueAllocateBuffer(audioState.mQueue, bytesPerFrame, &audioState.mBuffers[i]);
            audioState.mBuffers[i]->mAudioDataByteSize = bytesPerFrame;
            AudioQueueEnqueueBuffer(audioState.mQueue, audioState.mBuffers[i], 0, NULL);
        }
        AudioQueueStart(audioState.mQueue, NULL);
    }

    return samplesPerFrame;
}

int osd_update_audio_stream(INT16 *buffer)
{
    UINT8 *ptr = (UINT8 *)buffer;
    memcpy(&bufferCache[bufferInOffset], ptr, bytesPerFrame);
    bufferInOffset += bytesPerFrame;
    if (bufferInOffset + bytesPerFrame >= BUFFER_CACHE_SIZE)
    {
        bufferInOffset = 0;
    }
    return samplesPerFrame;
}

void osd_stop_audio_stream(void)
{
    AudioQueueDispose(audioState.mQueue, true);
    bufferOutOffset = 0;
    bufferInOffset = 0;
}

void osd_set_mastervolume(int attenuation) {}
int osd_get_mastervolume(void) { return 0; }
void osd_sound_enable(int enable) {}
void osd_opl_control(int chip, int reg) {}
void osd_opl_write(int chip, int data) {}

#endif

//
//  iOS_sound.m
//  MAME4apple
//
//  Created by Les Bird on 9/20/16.
//

#import <Foundation/Foundation.h>
#include <AudioToolbox/AudioQueue.h>
#include <AudioToolbox/AudioFile.h>
#include <AudioToolbox/AudioFormat.h>
#include "GameScene.h"
#include "driver.h"

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
    if (stereo) stereo = 1;	/* make sure it's either 0 or 1 */
    
    if (Machine->sample_rate == 0)
    {
        Machine->sample_rate = 22050;
    }
    
    samplesPerFrame = Machine->sample_rate / Machine->drv->frames_per_second;

    NSLog(@"## AudioQueue set up - sample rate %d ##", samplesPerFrame);
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
        NSLog(@"## allocate audio buffers ##");
        for (int i = 0; i < kNumberBuffers; i++)
        {
            AudioQueueAllocateBuffer(audioState.mQueue, bytesPerFrame, &audioState.mBuffers[i]);
            audioState.mBuffers[i]->mAudioDataByteSize = bytesPerFrame;
            AudioQueueEnqueueBuffer(audioState.mQueue, audioState.mBuffers[i], 0, NULL);
        }
        
        NSLog(@"## AudioQueueStart() ##");
        AudioQueueStart(audioState.mQueue, NULL);
    }
    else
    {
        NSLog(@"## AudioQueueNewOutput() failed ##");
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

void osd_set_mastervolume(int attenuation)
{
}

int osd_get_mastervolume(void)
{
    return 0;
}

void osd_sound_enable(int enable)
{
}

/* direct access to the Sound Blaster OPL chip */
void osd_opl_control(int chip,int reg)
{
}

void osd_opl_write(int chip,int data)
{
}

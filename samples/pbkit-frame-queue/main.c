// SPDX-License-Identifier: CC0-1.0

#include <hal/debug.h>
#include <hal/video.h>
#include <pbkit/pbkit.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <xboxkrnl/xboxkrnl.h>

#include "pbkit_queue_trace.h"

#ifndef PBKIT_QUEUE_TEST_REFRESHES
#define PBKIT_QUEUE_TEST_REFRESHES 3600
#endif

#define NXDK_STOCK_COMMIT "fafbe41b48fa00a31b79dd49ad1f86a30adcdd9e"
#define NXDK_PATCH_COMMIT "fbc6192464825172dad193b007a3c47e3eccf5d8"
#define COMPILER_ID "clang 22.1.8 (MSYS2 MINGW-packages d34feaaa)"
#define MAX_INTERVALS 8192

static uint32_t selected_intervals_us[MAX_INTERVALS];

static int compare_u32(const void *a, const void *b)
{
    uint32_t av=*(const uint32_t *)a;
    uint32_t bv=*(const uint32_t *)b;
    return av>bv ? 1 : av<bv ? -1 : 0;
}

static const char *event_name(uint32_t type)
{
    switch(type)
    {
        case PBKIT_QUEUE_EVENT_QUEUED: return "queued";
        case PBKIT_QUEUE_EVENT_SELECTED: return "selected";
        case PBKIT_QUEUE_EVENT_REMOVED: return "removed";
        case PBKIT_QUEUE_EVENT_DEFERRED: return "deferred";
        default: return "unknown";
    }
}

static FILE *open_result(char path[64])
{
    for (unsigned i=1;i<=99;i++)
    {
        snprintf(path,64,"D:\\pbkit-queue-%s-%s-%02u.csv",PBKIT_QUEUE_VARIANT,
#ifdef PBKIT_QUEUE_TEST_720P
                 "720p",
#else
                 "480p",
#endif
                 i);
        FILE *existing=fopen(path,"r");
        if (existing)
        {
            fclose(existing);
            continue;
        }
        return fopen(path,"w");
    }
    return NULL;
}

static void write_results(uint32_t frames, uint32_t stall_episodes,
                          uint64_t stall_ticks, uint64_t max_stall_ticks,
                          uint64_t elapsed_ticks, VIDEO_MODE mode)
{
    PBKIT_QUEUE_TRACE_SUMMARY summary=pbkit_queue_trace_summary();
    const PBKIT_QUEUE_TRACE_EVENT *events=pbkit_queue_trace_events();
    uint64_t frequency=KeQueryPerformanceFrequency();
    uint64_t last_selected=0;
    uint32_t interval_count=0;
    uint32_t max_interval=0;
    uint32_t p95_interval=0;
    char path[64];
    FILE *output;

    for (uint32_t i=0;i<summary.event_count;i++)
    {
        if (events[i].type!=PBKIT_QUEUE_EVENT_SELECTED) continue;
        if (last_selected && interval_count<MAX_INTERVALS)
        {
            uint32_t us=(uint32_t)((events[i].ticks-last_selected)*1000000ULL/frequency);
            selected_intervals_us[interval_count++]=us;
            if (us>max_interval) max_interval=us;
        }
        last_selected=events[i].ticks;
    }
    if (interval_count)
    {
        qsort(selected_intervals_us,interval_count,sizeof(selected_intervals_us[0]),compare_u32);
        p95_interval=selected_intervals_us[((uint64_t)interval_count*95+99)/100-1];
    }

    output=open_result(path);
    if (!output)
    {
        debugPrint("Could not create result file\n");
        return;
    }

    fprintf(output,"metadata,value\n");
    fprintf(output,"variant,%s\n",PBKIT_QUEUE_VARIANT);
    fprintf(output,"stock_commit,%s\n",NXDK_STOCK_COMMIT);
    fprintf(output,"patched_commit,%s\n",NXDK_PATCH_COMMIT);
    fprintf(output,"compiler,%s\n",COMPILER_ID);
    fprintf(output,"video_mode,%dx%dx%d@%d\n",mode.width,mode.height,mode.bpp,mode.refresh);
    fprintf(output,"target_refreshes,%u\n",PBKIT_QUEUE_TEST_REFRESHES);
    fprintf(output,"observed_refreshes,%u\n",summary.refresh_count);
    fprintf(output,"elapsed_us,%llu\n",(unsigned long long)(elapsed_ticks*1000000ULL/frequency));
    fprintf(output,"frames_submitted,%u\n",frames);
    fprintf(output,"frames_queued,%u\n",summary.queued_count);
    fprintf(output,"buffer_selections,%u\n",summary.selected_count);
    fprintf(output,"queue_removals,%u\n",summary.removed_count);
    fprintf(output,"premature_removals,%u\n",summary.premature_removal_count);
    fprintf(output,"repeated_refreshes,%u\n",summary.repeated_refresh_count);
    fprintf(output,"deferred_removals,%u\n",summary.deferred_count);
    fprintf(output,"stall_episodes,%u\n",stall_episodes);
    fprintf(output,"stall_total_us,%llu\n",(unsigned long long)(stall_ticks*1000000ULL/frequency));
    fprintf(output,"stall_max_us,%llu\n",(unsigned long long)(max_stall_ticks*1000000ULL/frequency));
    fprintf(output,"selected_interval_p95_us,%u\n",p95_interval);
    fprintf(output,"selected_interval_max_us,%u\n",max_interval);
    fprintf(output,"trace_events,%u\n",summary.event_count);
    fprintf(output,"trace_overflow,%u\n",summary.overflow_count);
    fprintf(output,"\nsequence,ticks,refresh,event,slot,buffer,frame,scanout,target,repeated,premature\n");
    for (uint32_t i=0;i<summary.event_count;i++)
    {
        const PBKIT_QUEUE_TRACE_EVENT *event=&events[i];
        fprintf(output,"%u,%llu,%u,%s,%u,%u,%u,0x%08x,0x%08x,%u,%u\n",
                event->sequence,(unsigned long long)event->ticks,event->refresh,
                event_name(event->type),event->queue_slot,event->buffer_index,
                event->frame,event->scanout,event->target,
                event->repeated_refresh,event->premature_removal);
    }
    fclose(output);

    debugPrint("pbkit queue %s complete: %s\n",PBKIT_QUEUE_VARIANT,path);
    debugPrint("refresh=%u queued=%u removed=%u premature=%u repeated=%u deferred=%u\n",
               summary.refresh_count,summary.queued_count,summary.removed_count,
               summary.premature_removal_count,summary.repeated_refresh_count,
               summary.deferred_count);
}

int main(void)
{
#ifdef PBKIT_QUEUE_TEST_720P
    const int requested_width=1280;
    const int requested_height=720;
#else
    const int requested_width=640;
    const int requested_height=480;
#endif
    uint32_t frame=1;
    uint32_t stall_episodes=0;
    uint64_t stall_ticks=0;
    uint64_t max_stall_ticks=0;
    uint64_t started;
    uint64_t finished;

    if (!XVideoSetMode(requested_width,requested_height,32,REFRESH_60HZ))
    {
        debugPrint("Unsupported video mode %dx%d\n",requested_width,requested_height);
        Sleep(INFINITE);
    }
    VIDEO_MODE mode=XVideoGetMode();

    int status=pb_init();
    if (status)
    {
        debugPrint("pb_init failed: %d\n",status);
        Sleep(INFINITE);
    }
    pb_show_front_screen();
    pbkit_queue_trace_reset();
    started=KeQueryPerformanceCounter();

    while (pbkit_queue_trace_summary().refresh_count<PBKIT_QUEUE_TEST_REFRESHES)
    {
        uint32_t width=pb_back_buffer_width();
        uint32_t height=pb_back_buffer_height();
        uint32_t color=0x00101010u|((frame*37u)&0xffu)<<16|((frame*73u)&0xffu)<<8|((frame*109u)&0xffu);

        pb_reset();
        pb_target_back_buffer();
        pb_fill(0,0,width,height,color);
        pb_erase_text_screen();
        pb_print("PBKIT FRAME QUEUE TEST\n\n");
        pb_print("Variant: %s\n",PBKIT_QUEUE_VARIANT);
        pb_print("Mode: %dx%d @ %d\n",mode.width,mode.height,mode.refresh);
        pb_print("Frame: %u\n",frame);
        pb_print("Target refreshes: %u\n",PBKIT_QUEUE_TEST_REFRESHES);
        pb_draw_text_screen();

        while (pb_busy()) {}

        pbkit_queue_trace_set_next_frame(frame);
        uint64_t stall_start=0;
        while (pb_finished())
        {
            if (!stall_start)
            {
                stall_start=KeQueryPerformanceCounter();
                stall_episodes++;
            }
        }
        if (stall_start)
        {
            uint64_t duration=KeQueryPerformanceCounter()-stall_start;
            stall_ticks+=duration;
            if (duration>max_stall_ticks) max_stall_ticks=duration;
        }
        frame++;
    }

    for (unsigned i=0;i<4;i++) pb_wait_for_vbl();
    finished=KeQueryPerformanceCounter();
    pb_kill();
    write_results(frame-1,stall_episodes,stall_ticks,max_stall_ticks,
                  finished-started,mode);
    Sleep(INFINITE);
    return 0;
}

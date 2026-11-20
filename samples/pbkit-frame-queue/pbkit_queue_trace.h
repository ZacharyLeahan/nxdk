// SPDX-License-Identifier: CC0-1.0

#ifndef PBKIT_QUEUE_TRACE_H
#define PBKIT_QUEUE_TRACE_H

#include <stddef.h>
#include <stdint.h>

#define PBKIT_QUEUE_TRACE_CAPACITY 32768

typedef enum {
    PBKIT_QUEUE_EVENT_QUEUED = 1,
    PBKIT_QUEUE_EVENT_SELECTED,
    PBKIT_QUEUE_EVENT_REMOVED,
    PBKIT_QUEUE_EVENT_DEFERRED,
} PBKIT_QUEUE_EVENT_TYPE;

typedef struct {
    uint64_t ticks;
    uint32_t sequence;
    uint32_t refresh;
    uint32_t type;
    uint32_t queue_slot;
    uint32_t buffer_index;
    uint32_t frame;
    uint32_t scanout;
    uint32_t target;
    uint32_t repeated_refresh;
    uint32_t premature_removal;
} PBKIT_QUEUE_TRACE_EVENT;

typedef struct {
    uint32_t event_count;
    uint32_t overflow_count;
    uint32_t refresh_count;
    uint32_t queued_count;
    uint32_t selected_count;
    uint32_t removed_count;
    uint32_t deferred_count;
    uint32_t repeated_refresh_count;
    uint32_t premature_removal_count;
} PBKIT_QUEUE_TRACE_SUMMARY;

void pbkit_queue_trace_reset(void);
void pbkit_queue_trace_set_next_frame(uint32_t frame);
PBKIT_QUEUE_TRACE_SUMMARY pbkit_queue_trace_summary(void);
const PBKIT_QUEUE_TRACE_EVENT *pbkit_queue_trace_events(void);

#endif

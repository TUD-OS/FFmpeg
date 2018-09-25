#ifndef AVUTIL_DEBUG_H
#define AVUTIL_DEBUG_H

#include <time.h>
#include <stdint.h>

#include "libavutil/attributes.h"
#include "libavutil/avassert.h"
#include "libavutil/log.h"

/* ******** BEGIN GLOBAL CONFIGURATION ******** */

/* Whether to extract metrics at all*/
#define EXTRACT_METRICS 1

/* Wether to measure computing times at all */
#define MEASURE_TIME 1

/* Use RDTSC for time measurements, much faster than the default
 * implementation based on clock_gettime
 * Must currently not be disabled! */
#define USE_RDTSC 1

/* Base frequency of the local CPU, only used on RDTSC time measureing */
#define CPU_BASE_FREQ 4.0

/* Whether to measure cabac related times, creates large overhead if enabled */
#if MEASURE_TIME
#define MEASURE_CABAC 1
#endif

/* Print times in milliseconds rather than nanoseconds */
#define PRINT_MS 0

/* ******** END GLOBAL CONFIGURATION ******** */

#ifdef USE_RDTSC
#include "libavutil/x86/timer.h"
#endif

typedef struct
{
    uint64_t frame_number;

    // Metrics
    int slice_type;
    uint32_t slice_size;
    uint32_t cabac_size;
    uint32_t pixel_count;
    uint32_t cu_count;
    uint32_t intra_cu_count;
    uint32_t inter_cu_count;
    uint32_t skip_cu_count;
    uint32_t pcm_cu_count;
    uint32_t bit_depth;
    uint32_t ctb_size; // Same for ALL CTUs 64/32/16
    uint32_t tu_count;
    uint32_t inter_pu_count;
    uint32_t intra_pu_count;
    uint32_t deblock_luma_edge_count;
    uint32_t deblock_chroma_edge_count;
    uint32_t sao_band_count;
    uint32_t sao_edge_count;

    // Time measurements
    // BEFORE you add new time measurements, ensure you will add them to the post processing functions in avpriv_log_stats_ctx()!
    uint64_t frame_time; // -complete frame time- / hevc_decode_frame
    uint64_t slice_time; // hls_decode_entry
    uint64_t cabac_time; // get_cabac_bypass, GET_CABAC, ff_hevc_cabac_init

    uint64_t intra_cu_time;
    uint64_t inter_cu_time;
    uint64_t pcm_cu_time; // -time spent for pcm intra decoding, most likely not used- / hls_pcm_sample
    uint64_t transform_time; // -residual- / hls_transform_tree
    uint64_t deblock_time; // ff_hevc_deblocking_boundary_strengths, deblocking_filter_CTB
    uint64_t sao_time; // hls_sao_param, sao_filter_CTB
    // sao_time and deblocking_filter_CTB NOT included in CU time!

    // Cabac exclusion
    uint64_t cabac_intra_cu_time;
    uint64_t cabac_inter_cu_time;
    uint64_t cabac_pcm_cu_time;
    uint64_t cabac_transform_time;
    uint64_t cabac_deblock_time;
    uint64_t cabac_sao_time;

    int cabac_intra_cu_flag;
    int cabac_inter_cu_flag;
    int cabac_pcm_cu_flag;
    int cabac_transform_flag;
    int cabac_deblock_flag;
    int cabac_sao_flag;

    // Other exclusions
    uint64_t transform_included_time;
    uint64_t pcm_included_time;
} AVStatsContext;

AVStatsContext* avpriv_reset_stats_ctx(AVStatsContext* ctx);
void avpriv_log_stats_ctx(AVStatsContext* ctx);

void avpriv_init_log(const char *input_file);
void avpriv_log(const char* message);
void avpriv_finalize_log(void);

static av_always_inline av_unused void avpriv_log_info(const char* message)
{
    av_log(NULL, AV_LOG_INFO,  "%s\n", message);
}

static av_always_inline av_unused void avpriv_log_error(const char* message)
{
    av_log(NULL, AV_LOG_ERROR, "%s\n", message);
}

static av_always_inline av_unused void avpriv_log_errno(const char *message)
{
    char* errno_msg = strerror(errno);
    char msg[512];
    snprintf(msg, 512, "%s: %s", message, errno_msg);
    avpriv_log_error(msg);
}

static av_always_inline av_unused void avpriv_get_thread_time(struct timespec* time)
{
    //CLOCK_THREAD_CPUTIME_ID
    int ret = clock_gettime(CLOCK_MONOTONIC_RAW, time);
    if (ret != 0) {
        avpriv_log_errno("Could not get thread time");
    }
}

static av_always_inline av_unused void avpriv_calc_timespan(struct timespec* result, struct timespec* start_time, struct timespec* end_time)
{
    long secs, nsecs;
    if ((end_time->tv_nsec - start_time->tv_nsec) < 0) {
        secs = end_time->tv_sec - start_time->tv_sec - 1;
        nsecs = 1000000000 + end_time->tv_nsec - start_time->tv_nsec;
    } else {
        secs = end_time->tv_sec - start_time->tv_sec;
        nsecs = end_time->tv_nsec - start_time->tv_nsec;
    }

    av_assert0(secs >= 0);
    av_assert0(nsecs >= 0);

    result->tv_sec = secs;
    result->tv_nsec = nsecs;
}

static av_always_inline av_unused void avpriv_calc_timespan_now(struct timespec* result, struct timespec* start_time)
{
    struct timespec end_time;
    avpriv_get_thread_time(&end_time);
    avpriv_calc_timespan(result, start_time, &end_time);
}


static av_always_inline av_unused int64_t avpriv_calc_timespan_ns(struct timespec *start_time, struct timespec *end_time)
{
    struct timespec result;
    avpriv_calc_timespan(&result, start_time, end_time);
    return result.tv_sec * 1000000000 + result.tv_nsec;
}

static av_always_inline av_unused int64_t avpriv_calc_timespan_now_ns(struct timespec *start_time)
{
    struct timespec result;
    avpriv_calc_timespan_now(&result, start_time);
    return result.tv_sec * 1000000000 + result.tv_nsec;
}

static av_always_inline av_unused int64_t ns_to_ms(int64_t ns)
{
    return (double)(ns)/1000000.0;
}

static av_always_inline av_unused int64_t rtdsc_to_ns(int64_t cycles) {
    return (double)(cycles) / CPU_BASE_FREQ;
}

#if EXTRACT_METRICS
#define FFMPEG_EXTRACT_METRICS(x) x
#else
#define FFMPEG_EXTRAKT_METRICS(x)
#endif // EXTRACT_METRICS

#if MEASURE_TIME
#if USE_RDTSC
#define FFMPEG_TIME_BEGINN(measurement)  \
    uint64_t start_time_##measurement = AV_READ_TIME();

#define FFMPEG_TIME_END(measurement) \
    *measurement += AV_READ_TIME() - start_time_##measurement;
#else
#define FFMPEG_TIME_BEGINN(measurement) \
    struct timespec start_time_##measurement; \
    avpriv_get_thread_time(&start_time_##measurement)

#define FFMPEG_TIME_END(measurement) \
    *measurement += avpriv_calc_timespan_now_ns(&start_time_##measurement)
#endif // USE_RDTSC
#else
#define FFMPEG_TIME_BEGINN(measurement)
#define FFMPEG_TIME_END(measurement)
#endif // MEASURE_TIME

#if MEASURE_CABAC
#define FFMPEG_MEASURE_CABAC(x) x

#define FFMPEG_CABAC_TIME_BEGIN(measurement, statsctx) \
    uint64_t start_time_##measurement = AV_READ_TIME();

#define FFMPEG_CABAC_TIME_END(measurement, stats) \
    do { \
        uint64_t diff_##measurement = AV_READ_TIME() - start_time_##measurement; \
        stats->cabac_time += diff_##measurement; \
        if (stats->cabac_intra_cu_flag) { \
            stats->cabac_intra_cu_time += diff_##measurement; \
        } else if (stats->cabac_inter_cu_flag) { \
            stats->cabac_inter_cu_time += diff_##measurement; \
        } else if (stats->cabac_pcm_cu_flag) { \
           stats->cabac_pcm_cu_time += diff_##measurement; \
        } else if (stats->cabac_transform_flag) { \
            stats->cabac_transform_time += diff_##measurement; \
        } else if (stats->cabac_deblock_flag) { \
            stats->cabac_deblock_time += diff_##measurement; \
        } else if (stats->cabac_sao_flag) { \
            stats->cabac_sao_time += diff_##measurement; \
        } \
    } while (0)

#else
#define FFMPEG_MEASURE_CABAC(x)
#define FFMPEG_CABAC_TIME_BEGINN(measurement, stats)
#define FFMPEG_CABAC_TIME_END(measurement, stats)
#endif // MEASURE_CABAC

#endif // AVUTIL_DEBUG_H

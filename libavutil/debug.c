#include "debug.h"

#include <errno.h>
#include <libgen.h>
#include <stdio.h>
#include <time.h>

static FILE* logfile_;
static char logfile_name_[256];


AVStatsContext* avpriv_reset_stats_ctx(AVStatsContext* ctx)
{
    memset(ctx, 0, sizeof (AVStatsContext));
    return ctx;
}

void avpriv_log_stats_ctx(AVStatsContext* ctx)
{
    /* TODO: Improve*/
#define LOG_LINE_LENGTH 256
    char line[LOG_LINE_LENGTH];
#ifdef USE_RDTSC
    ctx->frame_time = rtdsc_to_ns(ctx->frame_time);
    ctx->cu_time = rtdsc_to_ns(ctx->cu_time);
    ctx->cabac_time = rtdsc_to_ns(ctx->cabac_time);

    ctx->intra_cu_time = rtdsc_to_ns(ctx->intra_cu_time);
    ctx->inter_cu_time = rtdsc_to_ns(ctx->inter_cu_time);
    ctx->pcm_cu_time = rtdsc_to_ns(ctx->pcm_cu_time);
    ctx->transform_time = rtdsc_to_ns(ctx->transform_time);
    ctx->filter_time = rtdsc_to_ns(ctx->filter_time);
#endif
#if PRINT_MS
    ctx->frame_time = ns_to_ms(ctx->frame_time);
    ctx->cu_time = ns_to_ms(ctx->cu_time);
    ctx->cabac_time = ns_to_ms(ctx->cabac_time);

    ctx->intra_cu_time = ns_to_ms(ctx->intra_cu_time);
    ctx->inter_cu_time = ns_to_ms(ctx->inter_cu_time);
    ctx->pcm_cu_time = ns_to_ms(ctx->pcm_cu_time);
    ctx->transform_time = ns_to_ms(ctx->transform_time);
    ctx->filter_time = ns_to_ms(ctx->filter_time);
#endif
    snprintf(line, LOG_LINE_LENGTH
             , "%" PRIu64 ", %" PRIu64 ", %" PRIu64 ", %" PRIu64 ", %" PRIu64 ", %" PRIu64 ", %" PRIu64 ", %" PRIu64 ", %" PRIu64
             ", %d, %" PRIu32 ", %" PRIu32 ", %" PRIu32 ", %" PRIu32 ", %" PRIu32 ", %" PRIu32 ", %" PRIu32 ", %" PRIu32
             , ctx->frame_number, ctx->frame_time, ctx->cu_time, ctx->cabac_time, ctx->intra_cu_time, ctx->inter_cu_time, ctx->pcm_cu_time, ctx->transform_time, ctx->filter_time
             , ctx->slice_type, ctx->slice_size, ctx->cabac_size, ctx->pixel_count, ctx->cu_count, ctx->inter_cu_count, ctx->intra_cu_count, ctx->skip_cu_count, ctx->pcm_cu_count
             );
    avpriv_log(line);
}


void avpriv_init_log(const char* input_file)
{
    if (input_file == NULL) {
        sprintf(logfile_name_, "log/debug.csv");
    } else {
        char* name_copy = malloc(strlen(input_file));
        strcpy(name_copy, input_file);
        snprintf(logfile_name_, 256, "log/%s.csv", basename(name_copy));
        free(name_copy);
    }

    av_assert0(logfile_ == NULL);
    logfile_ = fopen(logfile_name_, "w");

    if (logfile_ == NULL) {
        char msg [384];
        snprintf(msg, 384, "Could not open logfile \"%s\"", logfile_name_);
        avpriv_log_errno(msg);
        exit(EXIT_FAILURE);
    }
    avpriv_log("frame_num, frame_time, cu_time, cabac_time, intra_cu_time, inter_cu_time, pcm_cu_time, transform_time, filter_time"
               ", slice_type, slice_size, cabac_size, pixel_count, cu_count, inter_cu_count, intra_cu_count, skip_cu_count, pcm_cu_count"
               );
}

void avpriv_log(const char *message)
{
    int size = fprintf(logfile_, "%s\n", message);
    if (size < 0) {
        avpriv_log_error("Error on writing logfile");
    }
}

void avpriv_finalize_log(void)
{
    int result = fclose(logfile_);
    if (result != 0) {
        avpriv_log_errno("Could not close log");
        exit(EXIT_FAILURE);
    }
}

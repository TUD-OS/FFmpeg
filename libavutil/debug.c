#include "debug.h"

#include <errno.h>
#include <libgen.h>
#include <stdio.h>
#include <time.h>

static FILE* logfile_;
static char logfile_name_[256];


AVStatsContext* avpriv_reset_stats_ctx(AVStatsContext* ctx)
{
    ctx->frame_number = 0;
    ctx->frame_time = 0;
    ctx->cabac_time = 0;
    ctx->slice_type = 0;
    return ctx;
}

void avpriv_log_stats_ctx(AVStatsContext* ctx)
{
    /* TODO: Improve*/
#define LOG_LINE_LENGTH 256
    char line[LOG_LINE_LENGTH];
#ifdef USE_RDTSC
    ctx->cabac_time = rtdsc_to_ns(ctx->cabac_time);
    ctx->frame_time = rtdsc_to_ns(ctx->frame_time);
#endif
#ifdef PRINT_MS
    snprintf(line, LOG_LINE_LENGTH, "%d, %ld, %ld, %d", ctx->frame_number, ns_to_ms(ctx->frame_time), ns_to_ms(ctx->cabac_time), ctx->slice_type);
#else
    snprintf(line, LOG_LINE_LENGTH, "%d, %ld, %ld, %d", ctx->frame_number, ctx->frame_time, ctx->cabac_time, ctx->slice_type);
#endif
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
    avpriv_log("frame_num, complete, cabac, slice_type");
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

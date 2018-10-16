#include "debug.h"

#include <errno.h>
#include <libgen.h>
#include <stdio.h>
#include <time.h>

static FILE* logfile_;
static char logfile_name_[256];

static size_t stats_list_idx_max = 8192;
static AVStatsContext* stats_list;
static size_t stats_list_idx = 0;

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

    ctx->intra_cu_time -= ctx->cabac_intra_cu_time;
    ctx->inter_cu_time -= ctx->cabac_inter_cu_time;
    ctx->pcm_cu_time -= ctx->cabac_pcm_cu_time;
    ctx->transform_time -= ctx->cabac_transform_time;
    ctx->deblock_time -= ctx->cabac_deblock_time;
    ctx->sao_time -= ctx->cabac_sao_time;

    ctx->transform_time -= ctx->transform_included_time;
    ctx->pcm_cu_time -= ctx->pcm_included_time;

#ifdef USE_RDTSC
    ctx->frame_time = rtdsc_to_ns(ctx->frame_time);
    ctx->slice_time = rtdsc_to_ns(ctx->slice_time);
    ctx->cabac_time = rtdsc_to_ns(ctx->cabac_time);

    ctx->intra_cu_time = rtdsc_to_ns(ctx->intra_cu_time);
    ctx->inter_cu_time = rtdsc_to_ns(ctx->inter_cu_time);
    ctx->pcm_cu_time = rtdsc_to_ns(ctx->pcm_cu_time);
    ctx->transform_time = rtdsc_to_ns(ctx->transform_time);
    ctx->deblock_time = rtdsc_to_ns(ctx->deblock_time);
    ctx->sao_time = rtdsc_to_ns(ctx->sao_time);
#endif
#if PRINT_MS
    ctx->frame_time = ns_to_ms(ctx->frame_time);
    ctx->slice_time = ns_to_ms(ctx->slice_time);
    ctx->cabac_time = ns_to_ms(ctx->cabac_time);

    ctx->intra_cu_time = ns_to_ms(ctx->intra_cu_time);
    ctx->inter_cu_time = ns_to_ms(ctx->inter_cu_time);
    ctx->pcm_cu_time = ns_to_ms(ctx->pcm_cu_time);
    ctx->transform_time = ns_to_ms(ctx->transform_time);
    ctx->deblock_time = ns_to_ms(ctx->deblock_time);
    ctx->sao_time = ns_to_ms(ctx->sao_time);
#endif
    snprintf(line, LOG_LINE_LENGTH
             , "%" PRIu64 ", %" PRIu64 ", %" PRIu64 ", %" PRIu64 ", %" PRIu64 ", %" PRIu64 ", %" PRIu64 ", %" PRIu64 ", %" PRIu64 ", %" PRIu64
             ", %d, %" PRIu32 ", %" PRIu32 ", %" PRIu32 ", %" PRIu32 ", %" PRIu32 ", %" PRIu32 ", %" PRIu32 ", %" PRIu32 ", %" PRIu32 ", %" PRIu32 ", %" PRIu32 ", %" PRIu32 ", %" PRIu32  ", %" PRIu32  ", %" PRIu32  ", %" PRIu32  ", %" PRIu32
             , ctx->frame_number, ctx->frame_time, ctx->slice_time, ctx->cabac_time, ctx->intra_cu_time, ctx->inter_cu_time, ctx->pcm_cu_time, ctx->transform_time, ctx->deblock_time, ctx->sao_time
             , ctx->slice_type, ctx->slice_size, ctx->cabac_size, ctx->cu_count, ctx->inter_cu_count, ctx->intra_cu_count, ctx->skip_cu_count, ctx->pcm_cu_count, ctx->pixel_count, ctx->bit_depth, ctx->ctb_size, ctx->tu_count, ctx->inter_pu_count, ctx->intra_pu_count, ctx->deblock_luma_edge_count, ctx->deblock_chroma_edge_count, ctx->sao_band_count, ctx->sao_edge_count
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
#if EXTRACT_METRICS
        snprintf(logfile_name_, 256, "log/%s.csv", basename(name_copy));
#else
        snprintf(logfile_name_, 256, "log/%s_no_extract.csv", basename(name_copy));
#endif // EXTRACT_METRICS
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
    avpriv_log("frame_num, frame_time, slice_time, cabac_time, intra_cu_time, inter_cu_time, pcm_cu_time, transform_time, deblock_time, sao_time"
               ", slice_type, slice_size, cabac_size, cu_count, inter_cu_count, intra_cu_count, skip_cu_count, pcm_cu_count, pixel_count, bit_depth, ctb_size, tu_count, inter_pu_count, intra_pu_count, deblock_luma_edge_count, deblock_chroma_edge_count, sao_band_count, sao_edge_count"
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
    printf("Writing log: %lu items\n", stats_list_idx);
    for(size_t i = 0; i < stats_list_idx; i++) {
        avpriv_log_stats_ctx(&stats_list[i]);
    }
    free(stats_list);
    int result = fclose(logfile_);
    if (result != 0) {
        avpriv_log_errno("Could not close log");
        exit(EXIT_FAILURE);
    }
}

void avpriv_push_back_stats(AVStatsContext *stats)
{
    if (stats_list_idx >= stats_list_idx_max) {
        stats_list_idx_max *= 2;
        stats_list = (AVStatsContext*)realloc(stats_list, stats_list_idx_max * sizeof(AVStatsContext));
    }
    memcpy(&stats_list[stats_list_idx], stats, sizeof (AVStatsContext));
    stats_list_idx++;
}

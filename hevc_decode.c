#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libavutil/debug.h"
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavdevice/avdevice.h"
#include "libavutil/avutil.h"

int main(int argc, char* argv[])
{
    int res = 0;

    if (argc < 2) {
        fprintf(stderr, "No input file specified. Exiting!\n");
        exit(1);
    }

    char* file_name = argv[1];

    avdevice_register_all();

    AVFormatContext* format_context = avformat_alloc_context();

    res = avformat_open_input(&format_context, file_name, NULL, NULL);
    if (res != 0) {
        exit(1);
    }
    printf("Format %s: duration %ld us, bit_rate %ld\n", format_context->iformat->long_name, format_context->duration, format_context->bit_rate);

    res = avformat_find_stream_info(format_context, NULL);
    if (res != 0) {
        printf("ERROR could not get the stream info\n");
        exit(1);
    }

    AVCodecParameters* codec_parameters = format_context->streams[0]->codecpar;
    AVCodec* codec = avcodec_find_decoder(codec_parameters->codec_id);
    printf("Video Codec [%s]: resolution %d x %d\n", codec->name, codec_parameters->width, codec_parameters->height);

    AVCodecContext* codec_context = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(codec_context, codec_parameters);
    avcodec_open2(codec_context, codec, NULL);

    AVFrame* frame = av_frame_alloc();
    AVPacket* packet = av_packet_alloc();

    avpriv_init_log(file_name);

    uint64_t decode_loop_time_v;
    uint64_t* decode_loop_time = &decode_loop_time_v;
    FFMPEG_TIME_BEGINN(decode_loop_time);

    while (av_read_frame(format_context, packet) >= 0) {
        if (packet->stream_index == 0) {

            res = avcodec_send_packet(codec_context, packet);
            if (res < 0) {
                printf("Error while sending a packet to the decoder: %s\n", av_err2str(res));
                exit(1);
            }

            while (res >= 0) {
                res = avcodec_receive_frame(codec_context, frame);
                if (res == AVERROR(EAGAIN) || res == AVERROR_EOF) {
                      break;
                } else if (res < 0) {
                    printf("Error while receiving a frame from the decoder: %s", av_err2str(res));
                    exit(1);
                }
            }
        }
        av_packet_unref(packet);
    }
    FFMPEG_TIME_END(decode_loop_time);
    fprintf(stderr, "\nDecoding took %f seconds\n", (double)(decode_loop_time_v) / (1000000000.0 * CPU_BASE_FREQ));

    avformat_close_input(&format_context);
    avformat_free_context(format_context);
    av_packet_free(&packet);
    av_frame_free(&frame);
    avcodec_free_context(&codec_context);

    avpriv_finalize_log();

    return 0;
}

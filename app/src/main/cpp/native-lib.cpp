#include <jni.h>
#include <string>
#include <unistd.h>
#include <android/log.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <cstdio>

#include "com_example_zjf_ffmpegavfilter_FFmpeg.h"
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <libavfilter/avfiltergraph.h>
#include <libavfilter/buffersrc.h>
#include <libavfilter/buffersink.h>
#include <libswresample/swresample.h>
#include <libavutil/opt.h>
#define TAG "FFmpegFilter"
#define LOGI(FORMAT,...) __android_log_print(ANDROID_LOG_INFO, TAG, FORMAT, ##__VA_ARGS__);
#define LOGE(FORMAT,...) __android_log_print(ANDROID_LOG_ERROR, TAG, FORMAT, ##__VA_ARGS__);
#define LOGD(FORMAT,...) __android_log_print(ANDROID_LOG_DEBUG, TAG, FORMAT, ##__VA_ARGS__);
};

int is_playing;
int again;
int release;
jboolean playAudio = JNI_TRUE;


AVFilterContext *buffersink_ctx;
AVFilterContext *buffersrc_ctx;
AVFilterGraph *filter_graph;

JNIEXPORT jint JNICALL Java_com_example_zjf_ffmpegavfilter_FFmpeg_filter
        (JNIEnv* env, jobject obj, jstring filePath,jobject surface, jstring filterDescr){
    LOGD("play");

    // sd卡中的视频文件地址,可自行修改或者通过jni传入
    const char *file_name = (env)->GetStringUTFChars(filePath, JNI_FALSE);
    const char *filter_descr = (env)->GetStringUTFChars(filterDescr, JNI_FALSE);

    av_register_all();

    avfilter_register_all();//added by ws for AVfilter

    AVFormatContext *pFormatCtx = avformat_alloc_context();

    // Open video file
    if (avformat_open_input(&pFormatCtx, file_name, NULL, NULL) != 0) {

        LOGD("Couldn't open file:%s\n", file_name);
        //goto end;
        return -1; // Couldn't open file
    }

    // Retrieve stream information
    if (avformat_find_stream_info(pFormatCtx, NULL) < 0) {
        LOGD("Couldn't find stream information.");
        return -1;
    }

    // Find the first video stream
    int videoStream = -1, i;
    for (i = 0; i < pFormatCtx->nb_streams; i++) {
        if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO && videoStream < 0){
            videoStream = i;
        }
    }
    if (videoStream == -1) {
        LOGD("Didn't find a video stream.");
        return -1; // Didn't find a video stream
        //goto end;
    }

    // Get a pointer to the codec context for the video stream
    AVCodecContext *pCodecCtx = avcodec_alloc_context3(NULL);
    if (pCodecCtx == NULL)
    {
        LOGD("Could not allocate AVCodecContext\n");
        return -1;
        //goto end;
    }
    avcodec_parameters_to_context(pCodecCtx, pFormatCtx->streams[videoStream]->codecpar);
    //added by ws for AVfilter start----------init AVfilter--------------------------ws

    char args[512];
    int ret;
    AVFilter *buffersrc  = avfilter_get_by_name("buffer");
    AVFilter *buffersink = avfilter_get_by_name("buffersink");//新版的ffmpeg库必须为buffersink
    AVFilterInOut *outputs = avfilter_inout_alloc();
    AVFilterInOut *inputs  = avfilter_inout_alloc();
    enum AVPixelFormat pix_fmts[] = {AV_PIX_FMT_YUV420P, AV_PIX_FMT_NONE };
    AVRational time_base = pFormatCtx->streams[videoStream]->time_base;
    AVBufferSinkParams *buffersink_params;

    filter_graph = avfilter_graph_alloc();

    snprintf(args, sizeof(args),
             "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
             pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt,
             time_base.num, time_base.den,
             pCodecCtx->sample_aspect_ratio.num, pCodecCtx->sample_aspect_ratio.den);

    ret = avfilter_graph_create_filter(&buffersrc_ctx, buffersrc, "in",
                                       args, NULL, filter_graph);
    if (ret < 0) {
        LOGD("Cannot create buffer source === %d\n",ret);
        //goto end;
        return ret;
    }

    /* buffer video sink: to terminate the filter chain. */
    buffersink_params = av_buffersink_params_alloc();
    buffersink_params->pixel_fmts = pix_fmts;
    ret = avfilter_graph_create_filter(&buffersink_ctx, buffersink, "out",
                                       NULL, buffersink_params, filter_graph);
    av_free(buffersink_params);
    if (ret < 0) {
        LOGD("Cannot create buffer sink\n");
        return ret;
        //goto end;
    }

    /* Endpoints for the filter graph. */
    outputs->name       = av_strdup("in");
    outputs->filter_ctx = buffersrc_ctx;
    outputs->pad_idx    = 0;
    outputs->next       = NULL;

    inputs->name       = av_strdup("out");
    inputs->filter_ctx = buffersink_ctx;
    inputs->pad_idx    = 0;
    inputs->next       = NULL;


    if ((ret = avfilter_graph_parse_ptr(filter_graph, filter_descr,
                                        &inputs, &outputs, NULL)) < 0) {
        LOGD("Cannot avfilter_graph_parse_ptr\n");
        return ret;
        //goto end;
    }

    if ((ret = avfilter_graph_config(filter_graph, NULL)) < 0) {
        LOGD("Cannot avfilter_graph_config\n");
        //goto end;
        return ret;
    }

    //added by ws for AVfilter start------------init AVfilter------------------------------ws

    // Find the decoder for the video stream
    AVCodec *pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
    if (pCodec == NULL) {
        LOGD("Codec not found.");
        //goto end;
        return -1; // Codec not found
    }

    if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {
        LOGD("Could not open codec.");
        //goto end;
        return -1; // Could not open codec
    }

    // 获取native window
    ANativeWindow *nativeWindow = ANativeWindow_fromSurface(env, surface);

    // 获取视频宽高
    int videoWidth = pCodecCtx->width;
    int videoHeight = pCodecCtx->height;

    // 设置native window的buffer大小,可自动拉伸
    ANativeWindow_setBuffersGeometry(nativeWindow, videoWidth, videoHeight,
                                     WINDOW_FORMAT_RGBA_8888);
    ANativeWindow_Buffer windowBuffer;

    if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {
        LOGD("Could not open codec.");
        //goto end;
        return -1; // Could not open codec
    }

    // Allocate video frame
    AVFrame *pFrame = av_frame_alloc();

    // 用于渲染
    AVFrame *pFrameRGBA = av_frame_alloc();
    if (pFrameRGBA == NULL || pFrame == NULL) {
        LOGD("Could not allocate video frame.");
        //goto end;
        return -1;
    }

    // Determine required buffer size and allocate buffer
    // buffer中数据就是用于渲染的,且格式为RGBA
    int numBytes = av_image_get_buffer_size(AV_PIX_FMT_RGBA, pCodecCtx->width, pCodecCtx->height,
                                            1);
    uint8_t *buffer = (uint8_t *) av_malloc(numBytes * sizeof(uint8_t));
    av_image_fill_arrays(pFrameRGBA->data, pFrameRGBA->linesize, buffer, AV_PIX_FMT_RGBA,
                         pCodecCtx->width, pCodecCtx->height, 1);

    // 由于解码出来的帧格式不是RGBA的,在渲染之前需要进行格式转换
    struct SwsContext *sws_ctx = sws_getContext(pCodecCtx->width,
                                                pCodecCtx->height,
                                                pCodecCtx->pix_fmt,
                                                pCodecCtx->width,
                                                pCodecCtx->height,
                                                AV_PIX_FMT_RGBA,
                                                SWS_BILINEAR,
                                                NULL,
                                                NULL,
                                                NULL);

    AVPacket packet;
    while (av_read_frame(pFormatCtx, &packet) >= 0) {
        if(again){
            goto again;
        }
        // Is this a packet from the video stream?
        if (packet.stream_index == videoStream) {
            //解码该帧
            if(avcodec_send_packet(pCodecCtx, &packet) == 0){
                while (avcodec_receive_frame(pCodecCtx, pFrame) == 0) {
                    //added by ws for AVfilter start
                    pFrame->pts = av_frame_get_best_effort_timestamp(pFrame);

                    //* push the decoded frame into the filtergraph
                    if (av_buffersrc_add_frame(buffersrc_ctx, pFrame) < 0) {
                        LOGD("Could not av_buffersrc_add_frame");
                        break;
                    }

                    ret = av_buffersink_get_frame(buffersink_ctx, pFrame);
                    if (ret < 0) {
                        LOGD("Could not av_buffersink_get_frame");
                        break;
                    }

                    // lock native window buffer
                    ANativeWindow_lock(nativeWindow, &windowBuffer, 0);

                    // 格式转换
                    sws_scale(sws_ctx, (uint8_t const *const *) pFrame->data,
                              pFrame->linesize, 0, pCodecCtx->height,
                              pFrameRGBA->data, pFrameRGBA->linesize);

                    // 获取stride
                    uint8_t *dst = (uint8_t *) windowBuffer.bits;
                    int dstStride = windowBuffer.stride * 4;
                    uint8_t *src = (pFrameRGBA->data[0]);
                    int srcStride = pFrameRGBA->linesize[0];

                    // 由于window的stride和帧的stride不同,因此需要逐行复制
                    int h;
                    for (h = 0; h < videoHeight; h++) {
                        memcpy(dst + h * dstStride, src + h * srcStride, srcStride);
                    }
                    ANativeWindow_unlockAndPost(nativeWindow);
                }
            }
        }
        av_packet_unref(&packet);
    }

    //end:
    is_playing = 0;
    //释放内存以及关闭文件
    av_free(buffer);
    av_free(pFrameRGBA);
    av_free(pFrame);
    avcodec_close(pCodecCtx);
    avformat_close_input(&pFormatCtx);
    avfilter_free(buffersrc_ctx);
    avfilter_free(buffersink_ctx);
    avfilter_graph_free(&filter_graph);

    free(buffer);
    free(sws_ctx);
    free(&windowBuffer);

    ANativeWindow_release(nativeWindow);
    (env)->ReleaseStringUTFChars(filePath, file_name);
    (env)->ReleaseStringUTFChars(filterDescr, filter_descr);
    LOGE("do release...");
    again:
    again = 0;
    LOGE("play again...");
    return ret;
}


long duration;
/*
 * Class:     com_example_zjf_ffmpegavfilter_FFmpeg
 * Method:    play
 * Signature: (Ljava/lang/String;Ljava/lang/Object;)I
 */
JNIEXPORT jint JNICALL Java_com_example_zjf_ffmpegavfilter_FFmpeg_play
        (JNIEnv *env, jobject obj, jstring filePath, jobject surface){
}

/*
 * Class:     com_example_zjf_ffmpegavfilter_FFmpeg
 * Method:    setPlayRate
 * Signature: (F)I
 */
JNIEXPORT jint JNICALL Java_com_example_zjf_ffmpegavfilter_FFmpeg_setPlayRate
        (JNIEnv *env, jobject obj, jfloat flt){

}

/*
 * Class:     com_example_zjf_ffmpegavfilter_FFmpeg
 * Method:    again
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_example_zjf_ffmpegavfilter_FFmpeg_again
        (JNIEnv *env, jobject obj){
    again = 1;
}

/*
 * Class:     com_example_zjf_ffmpegavfilter_FFmpeg
 * Method:    release
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_example_zjf_ffmpegavfilter_FFmpeg_release
        (JNIEnv *env, jobject obj){
    release = 1;
}

/*
 * Class:     com_example_zjf_ffmpegavfilter_FFmpeg
 * Method:    playAudio
 * Signature: (Z)V
 */
JNIEXPORT void JNICALL Java_com_example_zjf_ffmpegavfilter_FFmpeg_playAudio
        (JNIEnv *env, jobject obj, jboolean playAudio){
    //pla = playAudio;
}

#include "loop.h"

#include "decode.h"

int seek_to_pos(MusicHandle* handle) {
    if (!handle) return FFMPEG_CORE_ERR_NULLPTR;
    int re = FFMPEG_CORE_ERR_OK;
    DWORD r = WaitForSingleObject(handle->mutex, INFINITE);
    AVRational base = { 1, handle->sdl_spec.freq };
    if (r != WAIT_OBJECT_0) {
        re = FFMPEG_CORE_ERR_WAIT_MUTEX_FAILED;
        goto end;
    }
    if (handle->seek_pos >= handle->pts && handle->seek_pos <= handle->end_pts) {
        // 已经在缓冲区，直接从缓冲区移除不需要的数据
        int64_t samples = min(av_rescale_q_rnd(handle->seek_pos - handle->pts, AV_TIME_BASE_Q, base, AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX), av_audio_fifo_size(handle->buffer));
        if ((re = av_audio_fifo_drain(handle->buffer, samples)) < 0) {
            ReleaseMutex(handle->mutex);
            goto end;
        }
        // 增大当前时间
        handle->pts += av_rescale_q_rnd(samples, base, AV_TIME_BASE_Q, AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX);
    } else {
        // 不在缓冲区，调用av_seek_frame并清空缓冲区
        int flags = 0;
        if (handle->seek_pos < handle->end_pts) {
            flags |= AVSEEK_FLAG_BACKWARD;
        }
        if ((re = av_seek_frame(handle->fmt, handle->is->index, av_rescale_q_rnd(handle->seek_pos + handle->first_pts, AV_TIME_BASE_Q, handle->is->time_base, AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX), flags)) < 0) {
            ReleaseMutex(handle->mutex);
            goto end;
        }
        re = 0;
        av_audio_fifo_reset(handle->buffer);
        handle->set_new_pts = 1;
        handle->is_eof = 0;
    }
    ReleaseMutex(handle->mutex);
end:
    handle->is_seek = 0;
    return re;
}

DWORD WINAPI event_loop(LPVOID handle) {
    if (!handle) return FFMPEG_CORE_ERR_NULLPTR;
    MusicHandle* h = (MusicHandle*)handle;
    int samples = h->decoder->sample_rate * 15;
    /// 本次循环是否有做事
    char doing = 0;
    /// 是否往缓冲区加了数据
    char writed = 0;
    int buffered_size = h->sdl_spec.freq * 15;
    while (1) {
        doing = 0;
        if (h->stoping) break;
        if (h->is_seek && h->first_pts != INT64_MIN) {
            int re = seek_to_pos(h);
            if (re) {
                h->have_err = 1;
                h->err = re;
            }
            doing = 1;
            goto end;
        }
        if (!h->is_eof) {
            if (av_audio_fifo_size(h->buffer) < buffered_size) {
                int re = decode_audio(handle, &writed);
                if (re) {
                    h->have_err = 1;
                    h->err = re;
                }
                doing = 1;
            }
        } else {
            // 播放完毕，自动停止播放
            if (av_audio_fifo_size(h->buffer) == 0) {
                SDL_PauseAudioDevice(h->device_id, 1);
                h->is_playing = 0;
            }
        }
end:
        if (!doing) {
            Sleep(10);
        }
    }
    return FFMPEG_CORE_ERR_OK;
}

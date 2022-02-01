#ifndef _MUSICPLAYER2_CORE_H
#define _MUSICPLAYER2_CORE_H
#if __cplusplus
extern "C" {
#endif
#include "../ffmpeg_core.h"
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libavutil/avutil.h"
#include "libavutil/audio_fifo.h"
#include "libavutil/rational.h"
#include "libswresample/swresample.h"
#include "SDL2/SDL.h"
#include <Windows.h>

#ifndef min
#define min(x, y) (((x) < (y)) ? (x) : (y))
#endif

typedef struct MusicHandle {
/// Demux 用
AVFormatContext* fmt;
/// 要解码的流
AVStream* is;
/// 解码器类型
const AVCodec* codec;
/// 解码器
AVCodecContext* decoder;
/// 用于转换音频格式
struct SwrContext* swrac;
/// 指定的SDL输出格式
SDL_AudioSpec sdl_spec;
/// 事件处理线程
HANDLE thread;
/// 事件处理线程线程ID
DWORD thread_id;
/// 音频缓冲区
AVAudioFifo* buffer;
/// 输出格式
enum AVSampleFormat target_format;
/// 每样本的字节数
int target_format_pbytes;
/// SDL音频设备ID
SDL_AudioDeviceID device_id;
/// 错误信息（ffmpeg错误或Core错误
int err;
/// Mutex对象，作为线程锁（用于保护缓冲区和时间）
HANDLE mutex;
/// 缓冲区开始时间
int64_t pts;
/// 缓冲区结束时间
int64_t end_pts;
/// 第一个sample的pts
int64_t first_pts;
int64_t seek_pos;
/// SDL是否被初始化
unsigned char sdl_initialized : 1;
/// 让事件处理线程退出标志位
unsigned char stoping : 1;
/// 是否已读到文件尾部
unsigned char is_eof : 1;
/// 是否有错误
unsigned char have_err : 1;
/// 是否需要Seek
unsigned char is_seek : 1;
/// 是否需要设置新的缓冲区时间
unsigned char set_new_pts : 1;
unsigned char is_playing : 1;
} MusicHandle;
#if __cplusplus
}
#endif
#endif

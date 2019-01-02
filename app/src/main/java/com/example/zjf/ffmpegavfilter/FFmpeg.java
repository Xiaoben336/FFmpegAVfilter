package com.example.zjf.ffmpegavfilter;

import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;

public class FFmpeg {
	static {
		System.loadLibrary("avutil-55");
		System.loadLibrary("avcodec-57");
		System.loadLibrary("avformat-57");
		System.loadLibrary("avdevice-57");
		System.loadLibrary("swresample-2");
		System.loadLibrary("swscale-4");
		System.loadLibrary("postproc-54");
		System.loadLibrary("avfilter-6");
		System.loadLibrary("native-lib");
	}

	/**
	 * 创建AudioTrack对象，供JNI调用
	 * @param sampleRate sampleRate
	 * @param channels channels
	 * @return AudioTrack
	 */
	public AudioTrack createAudioTrack(int sampleRate, int channels){
		int audioFormat = AudioFormat.ENCODING_PCM_16BIT;
		int channelConfig;
		if(channels == 1){
			channelConfig = AudioFormat.CHANNEL_OUT_MONO;
		}else if(channels == 2){
			channelConfig = AudioFormat.CHANNEL_OUT_STEREO;
		}else{
			channelConfig = AudioFormat.CHANNEL_OUT_STEREO;
		}

		int bufferSizeInBytes = AudioTrack.getMinBufferSize(sampleRate, channelConfig, audioFormat);

		return new AudioTrack(AudioManager.STREAM_MUSIC, sampleRate, channelConfig, audioFormat,
				bufferSizeInBytes, AudioTrack.MODE_STREAM);
	}

	/**
	 *
	 * @param filePath		源文件目录
	 * @param surface		surfaceview
	 * @param filterType		滤镜类型
	 * @return
	 */
	public native int filter(String filePath, Object surface, String filterType);
	public native int play(String filePath, Object surface);
	public native int setPlayRate(float playRate);
	public native void again();
	public native void release();
	public native void playAudio(boolean play);
}

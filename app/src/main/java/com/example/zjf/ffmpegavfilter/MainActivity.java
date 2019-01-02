package com.example.zjf.ffmpegavfilter;

import android.os.Environment;
import android.os.Handler;
import android.os.Message;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.support.v7.widget.LinearLayoutManager;
import android.support.v7.widget.RecyclerView;
import android.util.Log;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;


import java.io.File;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

public class MainActivity extends AppCompatActivity implements SurfaceHolder.Callback {
	private static final String TAG = "MainActivity";
	//SD卡根目录
	private final static String PATH = Environment.getExternalStorageDirectory().getPath();
	//本地视频路径
	private final static String VIDEO_PATH = PATH + File.separator + "Beyond.mp4";

	private FFmpeg ffmpeg;
	//surface是否已经创建
	private boolean surfaceCreated;
	//是否正在播放
	private boolean isPlaying;
	//滤镜数组

	private final static int MSG_HIDE = 999;
	private final static int DELAY_TIME  = 5 * 1000;

	private Handler mHandler = new Handler(){
		@Override
		public void handleMessage(Message msg) {
			super.handleMessage(msg);
			if (msg.what == MSG_HIDE) {
				recyclerView.setVisibility(View.GONE);
			}
		}
	};

	private class HideRunnable implements Runnable{
		@Override
		public void run() {
			mHandler.obtainMessage(MSG_HIDE).sendToTarget();
		}
	}

	private HideRunnable hideRunnable;

	private String[] filters = new String[]{
			"lutyuv='u=128:v=128'",
			"hue='h=60:s=-3'",
			"lutrgb='r=0:g=0'",
			"edgedetect=low=0.1:high=0.4",
			"boxblur=2:1",
			"drawgrid=w=iw/3:h=ih/3:t=2:c=white@0.5",
			"colorbalance=bs=0.3",
			"drawbox=x=100:y=100:w=100:h=100:color=red@0.5'",
			"vflip",
			"unsharp",
			"movie=/storage/emulated/0/zjf.jpg[wm];[in][wm]overlay=5:5[out]"
	};
	private String[] txtArray = new String[]{
			"素描",
			"鲜明",//hue
			"暖蓝",
			"边缘",
			"模糊",
			"九宫格",
			"均衡",
			"矩形",
			"翻转",//vflip上下翻转,hflip是左右翻转
			"锐化",
			"水印"
	};
	private HorizontalAdapter horizontalAdapter;
	private RecyclerView recyclerView;
	private SurfaceView sfvSurfaceView;
	private SurfaceHolder sfhSurfaceHolder;
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_main);
		initView();
		registerListener();

		hideRunnable = new HideRunnable();
		mHandler.postDelayed(hideRunnable,DELAY_TIME);
	}

	//注册监听器
	private void registerListener() {
		horizontalAdapter.setOnItemClickListener(new OnItemClickListener() {
			@Override
			public void onItemClick(int position) {
				if (!surfaceCreated) {
					return;
				}

				final int mPosition = position;
				new Thread(new Runnable() {
					@Override
					public void run() {
						//切换播放
						if (isPlaying) {
							ffmpeg.again();
						}
						isPlaying = true;
						Log.d("filter","filters[mPosition] === " + filters[mPosition]);
						ffmpeg.filter(VIDEO_PATH,sfhSurfaceHolder.getSurface(),filters[mPosition]);
					}
				}).start();
			}
		});

		sfvSurfaceView.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				recyclerView.setVisibility(View.VISIBLE);
				mHandler.postDelayed(hideRunnable,DELAY_TIME);
			}
		});
	}

	/**
	 * 初始化控件
	 */
	private void initView() {
		ffmpeg = new FFmpeg();
		sfvSurfaceView = (SurfaceView) findViewById(R.id.sfvSurfaceView);
		sfhSurfaceHolder = sfvSurfaceView.getHolder();
		sfhSurfaceHolder.addCallback(this);

		recyclerView = (RecyclerView) findViewById(R.id.recycler_view);
		LinearLayoutManager linearLayoutManager = new LinearLayoutManager(this);
		linearLayoutManager.setOrientation(LinearLayoutManager.HORIZONTAL);
		recyclerView.setLayoutManager(linearLayoutManager);

		List<String> itemList = new ArrayList<>();
		itemList.addAll(Arrays.asList(txtArray));

		horizontalAdapter = new HorizontalAdapter(itemList);
		recyclerView.setAdapter(horizontalAdapter);
	}

	@Override
	public void surfaceCreated(final SurfaceHolder holder) {
		surfaceCreated = true;
	}

	@Override
	public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {

	}

	@Override
	public void surfaceDestroyed(SurfaceHolder holder) {
		if (holder != null) {
			holder.removeCallback(this);
			holder = null;
		}
		Log.d(TAG,"surfaceDestroyed");
		surfaceCreated = false;
	}

	@Override
	protected void onPause() {
		super.onPause();
		isPlaying = false;
		//horizontalAdapter = null;
	}

	@Override
	protected void onDestroy() {
		super.onDestroy();
		isPlaying = false;
		horizontalAdapter = null;
	}
}

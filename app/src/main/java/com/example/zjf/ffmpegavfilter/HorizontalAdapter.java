package com.example.zjf.ffmpegavfilter;

import android.graphics.Color;
import android.support.annotation.NonNull;
import android.support.v7.widget.RecyclerView;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;


import java.util.ArrayList;
import java.util.List;

public class HorizontalAdapter extends RecyclerView.Adapter {
	private List<String> itemList = new ArrayList<>();
	private OnItemClickListener onItemClickListener;
	private int lastClickPosition;

	public HorizontalAdapter(List<String> itemList){
		this.itemList = itemList;
	}

	public void setOnItemClickListener(OnItemClickListener onItemClickListener) {
		this.onItemClickListener = onItemClickListener;
	}

	@NonNull
	@Override
	public RecyclerView.ViewHolder onCreateViewHolder(@NonNull ViewGroup viewGroup, int i) {
		return new MyHolder(LayoutInflater.from(viewGroup.getContext())
				.inflate(R.layout.item_select,viewGroup,false));
	}

	@Override
	public void onBindViewHolder(@NonNull RecyclerView.ViewHolder viewHolder, int i) {
		final MyHolder myHolder = (MyHolder) viewHolder;
		myHolder.btnSelect.setText(itemList.get(i));
		myHolder.btnSelect.setTextColor(Color.DKGRAY);

		if (onItemClickListener != null) {
			myHolder.btnSelect.setOnClickListener(new View.OnClickListener() {
				@Override
				public void onClick(View v) {
					notifyItemChanged(lastClickPosition);
					Log.d("onBindViewHolder","lastClickPosition === " + lastClickPosition);

					//设置当前选中颜色
					myHolder.btnSelect.setTextColor(Color.BLUE);
					onItemClickListener.onItemClick(myHolder.getAdapterPosition());
					lastClickPosition = myHolder.getAdapterPosition();
				}
			});
		}
	}

	@Override
	public int getItemCount() {
		return itemList != null ? itemList.size() : 0;
	}

	private class MyHolder extends RecyclerView.ViewHolder{
		Button btnSelect;
		public MyHolder(@NonNull View itemView) {
			super(itemView);
			btnSelect = (Button) itemView.findViewById(R.id.btn_select);
		}
	}
}

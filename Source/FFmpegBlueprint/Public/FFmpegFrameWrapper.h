// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include <optional>

extern "C" {
#include <libavutil/frame.h>
}

/**
 *
 */
struct FFMPEGBLUEPRINT_API FFFmpegFrameWrapper {
public:
	AVFrame* GetRawFrame() const;

public:
	FFFmpegFrameWrapper();
	FFFmpegFrameWrapper(AVFrame* InRawFrame);
	static FFFmpegFrameWrapper CreateFrame(
	    const FString& ImagePath, int FrameIndex,
	    std::optional<int> FrameWidth = {}, std::optional<int> FrameHeight = {},
	    AVPixelFormat PixelFormat = AVPixelFormat::AV_PIX_FMT_YUV420P);
	static FFFmpegFrameWrapper CreateFrame(
	    const FImage& Image, int FrameIndex, std::optional<int> FrameWidth = {},
	    std::optional<int> FrameHeight = {},
	    AVPixelFormat      PixelFormat = AVPixelFormat::AV_PIX_FMT_YUV420P);

public:
	~FFFmpegFrameWrapper();

public:
	// copy constructor
	FFFmpegFrameWrapper(const FFFmpegFrameWrapper& Source);
	// move constructor
	FFFmpegFrameWrapper(FFFmpegFrameWrapper&& Source);
	// copy assignment operator
	FFFmpegFrameWrapper& operator=(const FFFmpegFrameWrapper& Source);
	// move assignment operator
	FFFmpegFrameWrapper& operator=(FFFmpegFrameWrapper&& Source);

private:
	AVFrame* RawFrame = nullptr;
};

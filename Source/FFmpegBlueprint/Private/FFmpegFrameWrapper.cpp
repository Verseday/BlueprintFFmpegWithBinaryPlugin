// Fill out your copyright notice in the Description page of Project Settings.

#include "FFmpegFrameWrapper.h"

#include "FFmpegUtils.h"
#include "ImageUtils.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/frame.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

AVFrame* FFFmpegFrameWrapper::GetRawFrame() const {
	return RawFrame;
}

FFFmpegFrameWrapper::FFFmpegFrameWrapper() : RawFrame(av_frame_alloc()) {}

FFFmpegFrameWrapper::FFFmpegFrameWrapper(AVFrame* InRawFrame)
    : RawFrame(InRawFrame) {}

FFFmpegFrameWrapper FFFmpegFrameWrapper::CreateFrame(
    const FString& ImagePath, const int FrameIndex,
    std::optional<int> FrameWidth, std::optional<int> FrameHeight,
    AVPixelFormat PixelFormat) {
	FImage Image;
	FImageUtils::LoadImage(*ImagePath, Image);
	return CreateFrame(Image, FrameIndex, FrameWidth, FrameHeight, PixelFormat);
}

FFFmpegFrameWrapper FFFmpegFrameWrapper::CreateFrame(
    const FImage& Image, const int FrameIndex, std::optional<int> FrameWidth,
    std::optional<int> FrameHeight, AVPixelFormat PixelFormat) {
	FFFmpegFrameWrapper FFmpegFrame;

	const auto& SrcFormat = UFFmpegUtils::FFmpegFrameFormatOf(Image.Format);
	const auto& SrcWidth  = Image.GetWidth();
	const auto& SrcHeight = Image.GetHeight();

	const auto& RawFrame = FFmpegFrame.GetRawFrame();

	RawFrame->pts    = FrameIndex;
	RawFrame->format = PixelFormat;
	RawFrame->width  = FrameWidth.value_or(SrcWidth);
	RawFrame->height = FrameHeight.value_or(SrcHeight);

	// フレームのバッファを初期化
	if (av_frame_get_buffer(RawFrame, 0) < 0) {
		UE_LOG(LogTemp, Error, TEXT("Failed to allocate AVFrame buffer"));
		return FFmpegFrame;
	}

	SwsContext* SwsConvertFormatContext =
	    sws_getContext(SrcWidth, SrcHeight, SrcFormat, SrcWidth, SrcHeight,
	                   PixelFormat, SWS_BILINEAR, nullptr, nullptr, nullptr);
	if (nullptr == SwsConvertFormatContext) {
		UE_LOG(LogTemp, Error, TEXT("Failed to create SwsContext."));
		return FFmpegFrame;
	}

	const auto&    RawImageData  = Image.RawData;
	const auto&    BytesPerPixel = Image.GetBytesPerPixel();
	const auto&    DestImageData = RawFrame->data[0];
	const uint8_t* SrcData[8]    = {RawImageData.GetData(),
	                                nullptr,
	                                nullptr,
	                                nullptr,
	                                nullptr,
	                                nullptr,
	                                nullptr,
	                                nullptr};
	const int SrcLineSize[8] = {SrcWidth * BytesPerPixel, 0, 0, 0, 0, 0, 0, 0};
	sws_scale(SwsConvertFormatContext, SrcData, SrcLineSize, 0, SrcHeight,
	          RawFrame->data, RawFrame->linesize);

	sws_freeContext(SwsConvertFormatContext);

	return FFmpegFrame;
}

FFFmpegFrameWrapper::~FFFmpegFrameWrapper() {
	if (nullptr != RawFrame) {
		av_frame_free(&RawFrame);
	}
}

FFFmpegFrameWrapper::FFFmpegFrameWrapper(const FFFmpegFrameWrapper& Source) {
	*this = Source;
}

FFFmpegFrameWrapper::FFFmpegFrameWrapper(FFFmpegFrameWrapper&& Source) {
	*this = MoveTemp(Source);
}

FFFmpegFrameWrapper&
    FFFmpegFrameWrapper::operator=(const FFFmpegFrameWrapper& Source) {
	this->RawFrame = av_frame_clone(Source.GetRawFrame());
	return *this;
}

FFFmpegFrameWrapper&
    FFFmpegFrameWrapper::operator=(FFFmpegFrameWrapper&& Source) {
	this->RawFrame  = Source.GetRawFrame();
	Source.RawFrame = nullptr;
	return *this;
}

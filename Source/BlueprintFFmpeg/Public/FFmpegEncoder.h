// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CreateImageFromTextureRHI.h"
#include "FFmpegEncoderConfig.h"
#include "FFmpegFrameWrapper.h"
#include "FFmpegUtils.h"
#include "LogFFmpegEncoder.h"

#include <atomic>
#include <concepts>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavcodec/codec.h>
#include <libavformat/avformat.h>
#include <libavformat/avio.h>
#include <libavutil/frame.h>
}

#include "FFmpegEncoder.generated.h"

/**
 * Result type of UFFmpegEncoder::Open
 */
UENUM(BlueprintType)
enum class FFmpegEncoderOpenResult : uint8 { Success, Failure };

/**
 * Result type of UFFmpegEncoder::Close
 */
UENUM(BlueprintType)
enum class FFmpegEncoderCloseResult : uint8 { Success, Failure };

/**
 * Result type of UFFmpegEncoder::AddFrame
 */
UENUM(BlueprintType)
enum class FFmpegEncoderAddFrameResult : uint8 { Success, Failure };

enum class FFmpegEncoderThreadResult {
	Success = 0,
	CodecH264IsNotFound,
	FailedToAllocateCodecContext,
	FailedToInitializeCodecContext,
	FailedToInitializeIOContext,
	FailedToAllocateFormatContext,
	FailedToAddANewStream,
	FailedToSetCodecParameters,
	FailedToWriteHeader,

	FailedToSendFrame,
	FailedToAllocatePacket,
	FailedToWritePacket,

	FailedToFlushSendFrame,
	FailedToWriteTrailer
};

/**
 * A video encoder that uses FFmpeg and can be used from blueprint.
 * How to use:
 *   1. Create instance of this class
 *   2. call Open function
 *   3. call AddFrame function for each frames you want to encode
 *   4. call Close function
 * then the video is output to the OutputFilePath specified in Open function.
 */
UCLASS(Blueprintable, BlueprintType)
class BLUEPRINTFFMPEG_API UFFmpegEncoder: public UObject, public FRunnable {
	GENERATED_BODY()

public:
	/**
	 * Initialize and put into encoding standby status.
	 * @param FFmpegEncoderConfig   setting.
	 * @param OutputFilePath   Output destination file path.
	 *                         The output format is determined by the
	 *                         extension of this path.
	 * @param[out] Result   result.
	 */
	UFUNCTION(BlueprintCallable, meta = (ExpandEnumAsExecs = "Result"))
	void Open(const FFFmpegEncoderConfig& FFmpegEncoderConfig,
	          const FString& OutputFilePath, FFmpegEncoderOpenResult& Result,
	          FString& ErrorMessage);

	/**
	 * Terminate encoding. The encoding result is output to the file specified by
	 * OutputFilePath of the Open function.
	 */
	UFUNCTION(BlueprintCallable)
	void Close();

	/**
	 * Add a frame. The argument is converted to a YUV420P format image, added as
	 * a frame, and appended to the file immediately after the frame data is
	 * finalized.
	 */
	UFUNCTION(BlueprintCallable, meta = (ExpandEnumAsExecs = "Result"))
	void AddFrameFromRenderTarget(
	    const UTextureRenderTarget2D* TextureRenderTarget,
	    FFmpegEncoderAddFrameResult& Result, FString& ErrorMessage);

	/**
	 * Add a frame. The argument is converted to a YUV420P format image, added as
	 * a frame, and appended to the file immediately after the frame data is
	 * finalized.
	 */
	UFUNCTION(BlueprintCallable, meta = (ExpandEnumAsExecs = "Result"))
	void AddFrameFromImagePath(const FString&               ImagePath,
	                           FFmpegEncoderAddFrameResult& Result,
	                           FString&                     ErrorMessage);

public:
	/**
	 * Add a frame. The argument is converted to a YUV420P format image, added as
	 * a frame, and appended to the file immediately after the frame data is
	 * finalized.
	 */
	template <typename FTextureRHIRef_T>
	  requires std::is_same_v<const FTextureRHIRef&, FTextureRHIRef_T> ||
	           std::is_same_v<FTextureRHIRef, FTextureRHIRef_T>
	void AddFrame(FTextureRHIRef_T&&           TextureRHI,
	              FFmpegEncoderAddFrameResult& Result, FString& ErrorMessage);

	/**
	 * Add a frame. The argument is converted to a YUV420P format image, added as
	 * a frame, and appended to the file immediately after the frame data is
	 * finalized.
	 */
	template <typename FImage_T>
	  requires std::is_same_v<const FImage&, FImage_T> ||
	           std::is_same_v<FImage, FImage_T>
	void AddFrame(FImage_T&& Image, FFmpegEncoderAddFrameResult& Result,
	              FString& ErrorMessage);

	/**
	 * Add a frame. The argument is converted to a YUV420P format image, added as
	 * a frame, and appended to the file immediately after the frame data is
	 * finalized.
	 */
	template <typename FFFmpegFrameThreadSafeSharedPtr_T>
	  requires std::is_same_v<const FFFmpegFrameThreadSafeSharedPtr&,
	                          FFFmpegFrameThreadSafeSharedPtr_T> ||
	           std::is_same_v<FFFmpegFrameThreadSafeSharedPtr,
	                          FFFmpegFrameThreadSafeSharedPtr_T>
	void AddFrame(FFFmpegFrameThreadSafeSharedPtr_T&& Image,
	              FFmpegEncoderAddFrameResult& Result, FString& ErrorMessage);

public:
	~UFFmpegEncoder();

	// FRunnable interfaces
public:
	virtual uint32 Run() override;
	virtual void   Stop() override;

	// private fields: no data race
private:
	bool                 bOpened = false;
	bool                 bClosed = false;
	FFFmpegEncoderConfig Config;
	FString              VideoPath;
	int64_t              FrameIndex = 0;
	FRunnableThread*     Thread     = nullptr;

	// private fields: beware of data race
private:
	// single-producer, single-consumer
	TQueue<FFFmpegFrameThreadSafeSharedPtr, EQueueMode::Spsc> Frames;
	std::atomic_bool                                          bRunning = true;
};

template <typename FTextureRHIRef_T>
  requires std::is_same_v<const FTextureRHIRef&, FTextureRHIRef_T> ||
           std::is_same_v<FTextureRHIRef, FTextureRHIRef_T>
void UFFmpegEncoder::AddFrame(FTextureRHIRef_T&&           TextureRHI,
                              FFmpegEncoderAddFrameResult& Result,
                              FString&                     ErrorMessage) {
	auto Image = CreateImageFromTextureRHI(Forward<FTextureRHIRef_T>(TextureRHI));
	return AddFrame(MoveTemp(Image), Result, ErrorMessage);
}

template <typename FImage_T>
  requires std::is_same_v<const FImage&, FImage_T> ||
           std::is_same_v<FImage, FImage_T>
void UFFmpegEncoder::AddFrame(FImage_T&&                   Image,
                              FFmpegEncoderAddFrameResult& Result,
                              FString&                     ErrorMessage) {
	auto FFmpegFrameWrapper = UFFmpegUtils::CreateFrame(
	    Forward<FImage_T>(Image), FrameIndex, Config.Width, Config.Height);
	return AddFrame(MoveTemp(FFmpegFrameWrapper), Result, ErrorMessage);
}

template <typename FFFmpegFrameThreadSafeSharedPtr_T>
  requires std::is_same_v<const FFFmpegFrameThreadSafeSharedPtr&,
                          FFFmpegFrameThreadSafeSharedPtr_T> ||
           std::is_same_v<FFFmpegFrameThreadSafeSharedPtr,
                          FFFmpegFrameThreadSafeSharedPtr_T>
void UFFmpegEncoder::AddFrame(FFFmpegFrameThreadSafeSharedPtr_T&& Frame,
                              FFmpegEncoderAddFrameResult&        Result,
                              FString& ErrorMessage) {
	// helper function to finish with success
	const auto& Success = [&]() {
		Result = FFmpegEncoderAddFrameResult::Success;
	};

	// helper function to finish with failure
	const auto& Failure = [&](const FString& Message) {
		ErrorMessage = Message;
		UE_LOG(LogFFmpegEncoder, Error, TEXT("%s"), *ErrorMessage);
		Result = FFmpegEncoderAddFrameResult::Failure;
	};

	// get Raw frame
	const auto& RawFrame = Frame.Get();

	// check FrameIndex is correct
	check(FrameIndex == RawFrame->pts);

	// enqueue frame
	const auto& SuccessToEnqueue =
	    Frames.Enqueue(Forward<FFFmpegFrameThreadSafeSharedPtr_T>(Frame));
	if (!SuccessToEnqueue) {
		return Failure("Failed to enqueue the frame.");
	}

	// increment FrameIndex
	++FrameIndex;

	return Success();
}

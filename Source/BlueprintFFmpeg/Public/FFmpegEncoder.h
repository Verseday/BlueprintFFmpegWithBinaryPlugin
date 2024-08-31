// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "FFmpegEncoderConfig.h"
#include "FFmpegFrameWrapper.h"

#include <atomic>

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
	void AddFrame(const FTextureRHIRef&        TextureRHI,
	              FFmpegEncoderAddFrameResult& Result, FString& ErrorMessage);

	/**
	 * Add a frame. The argument is converted to a YUV420P format image, added as
	 * a frame, and appended to the file immediately after the frame data is
	 * finalized.
	 */
	void AddFrame(const FImage& Image, FFmpegEncoderAddFrameResult& Result,
	              FString& ErrorMessage);

	/**
	 * Add a frame. The argument is converted to a YUV420P format image, added as
	 * a frame, and appended to the file immediately after the frame data is
	 * finalized.
	 */
	void AddFrame(const FFFmpegFrameThreadSafeSharedPtr& Frame,
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

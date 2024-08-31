// Fill out your copyright notice in the Description page of Project Settings.

#include "FFmpegEncoder.h"

#include "Engine/TextureRenderTarget2D.h"
#include "ImageUtils.h"

#include <tuple>

void UFFmpegEncoder::Open(const FFFmpegEncoderConfig& FFmpegEncoderConfig,
                          const FString&              OutputFilePath,
                          FFmpegEncoderOpenResult&    Result,
                          FString&                    ErrorMessage) {
	// helper function to finish with success
	const auto& Success = [&]() {
		Result = FFmpegEncoderOpenResult::Success;
	};

	// helper function to finish with failure
	const auto& Failure = [&](const FString& Message) {
		ErrorMessage = Message;
		UE_LOG(LogFFmpegEncoder, Error, TEXT("%s"), *ErrorMessage);
		Result = FFmpegEncoderOpenResult::Failure;
	};

	// Open function must be called only once.
	check(!bOpened);

	// Mark as opened
	bOpened = true;

	// copy Config
	Config = FFmpegEncoderConfig;

	// copy OutputFilePath
	VideoPath = OutputFilePath;

	// create encode thread
	Thread = FRunnableThread::Create(this, TEXT("FFmpeg encode thread"));
	if (nullptr == Thread) {
		return Failure("Failed to create encode thread.");
	}

	// finish as success
	return Success();
}

void UFFmpegEncoder::Close() {
	// Open function must be called and Close function must not be called.
	check(bOpened && !bClosed);

	// Mark as closed
	bClosed = true;

	// stop background thread
	Stop();
}

void UFFmpegEncoder::AddFrameFromRenderTarget(
    const UTextureRenderTarget2D* TextureRenderTarget,
    FFmpegEncoderAddFrameResult& Result, FString& ErrorMessage) {
	// helper function to finish with failure
	const auto& Failure = [&](const FString& Message) {
		ErrorMessage = Message;
		UE_LOG(LogFFmpegEncoder, Error, TEXT("%s"), *ErrorMessage);
		Result = FFmpegEncoderAddFrameResult::Failure;
	};

	// check TextureRenderTarget
	check(nullptr != TextureRenderTarget);

	// Open function must be called and Close function must not be called.
	check(bOpened && !bClosed);

	// get TextureResource
	const auto& TextureResource = TextureRenderTarget->GetResource();

	// check TextureResource
	if (nullptr == TextureResource) {
		return Failure("TextureResource is nullptr");
	}

	// get RHITexture
	const auto& RHITexture = TextureResource->GetTexture2DRHI();

	// check TextureResource
	if (nullptr == RHITexture) {
		return Failure("RHITexture is nullptr");
	}

	return AddFrame(RHITexture, Result, ErrorMessage);
}

void UFFmpegEncoder::AddFrameFromImagePath(const FString& ImagePath,
                                           FFmpegEncoderAddFrameResult& Result,
                                           FString& ErrorMessage) {
	// helper function to finish with failure
	const auto& Failure = [&](const FString& Message) {
		ErrorMessage = Message;
		UE_LOG(LogFFmpegEncoder, Error, TEXT("%s"), *ErrorMessage);
		Result = FFmpegEncoderAddFrameResult::Failure;
	};

	// Load image from ImagePath
	FImage      Image;
	const auto& SuccessToLoadImage = FImageUtils::LoadImage(*ImagePath, Image);

	// if failed to load image
	if (!SuccessToLoadImage) {
		return Failure("Failed to load image.");
	}

	// Add Frame from Image
	return AddFrame(MoveTemp(Image), Result, ErrorMessage);
}

#pragma region AddFrame functions just forward to AddFrame_Internal
void           UFFmpegEncoder::AddFrame(const FTextureRHIRef&        TextureRHI,
                                        FFmpegEncoderAddFrameResult& Result,
                                        FString&                     ErrorMessage) {
  return AddFrame_Internal(TextureRHI, Result, ErrorMessage);
}

void UFFmpegEncoder::AddFrame(FTextureRHIRef&&             TextureRHI,
                              FFmpegEncoderAddFrameResult& Result,
                              FString&                     ErrorMessage) {
	return AddFrame_Internal(MoveTemp(TextureRHI), Result, ErrorMessage);
}

void UFFmpegEncoder::AddFrame(const FImage&                Image,
                              FFmpegEncoderAddFrameResult& Result,
                              FString&                     ErrorMessage) {
	return AddFrame_Internal(Image, Result, ErrorMessage);
}

void UFFmpegEncoder::AddFrame(FImage&&                     Image,
                              FFmpegEncoderAddFrameResult& Result,
                              FString&                     ErrorMessage) {
	return AddFrame_Internal(MoveTemp(Image), Result, ErrorMessage);
}

void UFFmpegEncoder::AddFrame(const FFFmpegFrameThreadSafeSharedPtr& Frame,
                              FFmpegEncoderAddFrameResult&           Result,
                              FString& ErrorMessage) {
	return AddFrame_Internal(Frame, Result, ErrorMessage);
}

void UFFmpegEncoder::AddFrame(FFFmpegFrameThreadSafeSharedPtr&& Frame,
                              FFmpegEncoderAddFrameResult&      Result,
                              FString&                          ErrorMessage) {
	return AddFrame_Internal(MoveTemp(Frame), Result, ErrorMessage);
}
#pragma endregion

UFFmpegEncoder::~UFFmpegEncoder() {
	if (Thread) {
		// wait to finish thread
		Thread->Kill(true);

		// release memory for Thread
		delete Thread;
	}
}

#pragma region Run on the new thread functions

uint32 UFFmpegEncoder::Run() {
	using enum FFmpegEncoderThreadResult;

#pragma region Open
	// get Codec
	const auto& CodecH264 = avcodec_find_encoder(AV_CODEC_ID_H264);
	if (nullptr == CodecH264) {
		return static_cast<uint32>(CodecH264IsNotFound);
	}

	// get Codec Context
	auto ContextH264 = avcodec_alloc_context3(CodecH264);
	if (nullptr == ContextH264) {
		return static_cast<uint32>(FailedToAllocateCodecContext);
	}

	// get from Config
	const auto& [Width, Height, FrameRate, BitRate] =
	    std::tie(Config.Width, Config.Height, Config.FrameRate, Config.BitRate);

	// FrameRate as Rational
	const auto FrameRateAsRational = av_d2q(FrameRate, INT_MAX);

	// set Codec Context settings
	ContextH264->width     = Width;
	ContextH264->height    = Height;
	ContextH264->bit_rate  = BitRate;
	ContextH264->time_base = av_inv_q(FrameRateAsRational);
	ContextH264->framerate = FrameRateAsRational;

	ContextH264->gop_size     = 300;
	ContextH264->max_b_frames = 12;
	ContextH264->pix_fmt      = AV_PIX_FMT_YUV420P;

	// set CRF quality value
	AVDictionary* EncodeOptions = nullptr;
	av_dict_set(&EncodeOptions, "crf", "18", 0);
	if (avcodec_open2(ContextH264, CodecH264, &EncodeOptions) != 0) {
		return static_cast<uint32>(FailedToInitializeCodecContext);
	}
	av_dict_free(&EncodeOptions);

	// open output file
	auto         OutputFilePathInUTF8 = StringCast<UTF8CHAR>(*VideoPath);
	AVIOContext* IOContext            = nullptr;
	if (avio_open(&IOContext,
	              reinterpret_cast<const char*>(OutputFilePathInUTF8.Get()),
	              AVIO_FLAG_WRITE) < 0) {
		return static_cast<uint32>(FailedToInitializeIOContext);
	}

	// allocate memory to FormatContext
	AVFormatContext* FormatContext = nullptr;
	if (avformat_alloc_output_context2(
	        &FormatContext, nullptr, nullptr,
	        reinterpret_cast<const char*>(OutputFilePathInUTF8.Get())) < 0) {
		return static_cast<uint32>(FailedToAllocateFormatContext);
	}

	// set FormatContext to output to specified output file
	FormatContext->pb = IOContext;

	// add new stream to file
	const auto& Stream = avformat_new_stream(FormatContext, CodecH264);
	if (nullptr == Stream) {
		return static_cast<uint32>(FailedToAddANewStream);
	}

	// set Stream information
	Stream->sample_aspect_ratio = ContextH264->sample_aspect_ratio;
	Stream->time_base           = ContextH264->time_base;

	// set parameter from codec context h264
	if (avcodec_parameters_from_context(Stream->codecpar, ContextH264) != 0) {
		return static_cast<uint32>(FailedToSetCodecParameters);
	}

	// write header to output file
	if (avformat_write_header(FormatContext, nullptr) != 0) {
		return static_cast<uint32>(FailedToWriteHeader);
	}
#pragma endregion

#pragma region AddFrame
	auto ReceiveAllPendingPackets = [&]() {
		// allocate Packet
		AVPacket* Packet = av_packet_alloc();
		if (nullptr == Packet) {
			return FailedToAllocatePacket;
		}

		// receive a Packet
		while (avcodec_receive_packet(ContextH264, Packet) == 0) {
			check(Packet->size != 0);

			// set stream index of this packet from stream
			Packet->stream_index = Stream->index;

			// rescale
			av_packet_rescale_ts(Packet, ContextH264->time_base, Stream->time_base);

			// write Packet to output media file
			if (av_interleaved_write_frame(FormatContext, Packet) != 0) {
				return FailedToWritePacket;
			}

			// un reference the buffer of Packet
			av_packet_unref(Packet);
		}

		// free Packet resource
		av_packet_free(&Packet);

		// success
		return Success;
	};

	// Pause encode thread until Stop or AddFrame is called
	//	TryPauseEncodeThread();

	// Loop while the status is in running or Frames is not empty.
	while (bRunning || !Frames.IsEmpty()) {
		// if Frames queue is empty
		if (Frames.IsEmpty()) {
			// sleep for 10/FPS seconds
			FPlatformProcess::Sleep(10.0f / FrameRate);

			// loop again
			continue;
		}

		// get a frame pending encoding frame
		FFFmpegFrameThreadSafeSharedPtr Frame;
		Frames.Dequeue(Frame);

		// send a frame
		if (avcodec_send_frame(ContextH264, Frame.Get()) != 0) {
			return static_cast<uint32>(FailedToSendFrame);
		}

		// Receive all packets
		const auto& ReceiveResult = ReceiveAllPendingPackets();

		// failed to receive packets
		if (ReceiveResult != Success) {
			return static_cast<uint32>(ReceiveResult);
		}
	}
#pragma endregion

#pragma region Close
	// notify that encoding is finished
	if (avcodec_send_frame(ContextH264, nullptr) != 0) {
		return static_cast<int32>(FailedToFlushSendFrame);
	}

	// Receive all packets
	const auto& ReceiveResult = ReceiveAllPendingPackets();

	// failed to receive packets
	if (ReceiveResult != Success) {
		return static_cast<uint32>(ReceiveResult);
	}

	// write trailer to output file
	if (av_write_trailer(FormatContext) != 0) {
		return static_cast<int32>(FailedToWriteTrailer);
	}

	// free resources
	avcodec_free_context(&ContextH264);
	avformat_free_context(FormatContext);
	avio_closep(&IOContext);
#pragma endregion

	return static_cast<uint32>(Success);
}

#pragma endregion

void UFFmpegEncoder::Stop() {
	// stop running
	bRunning = false;
}

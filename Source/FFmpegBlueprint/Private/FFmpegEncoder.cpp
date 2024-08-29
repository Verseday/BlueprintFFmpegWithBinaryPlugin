// Fill out your copyright notice in the Description page of Project Settings.

#include "FFmpegEncoder.h"

#include "CreateImageFromTextureRHI.h"
#include "Engine/TextureRenderTarget2D.h"
#include "ImageUtils.h"
#include "LogFFmpegEncoder.h"

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

	// copy Config
	Config = FFmpegEncoderConfig;

	// get Codec
	CodecH264 = avcodec_find_encoder(AV_CODEC_ID_H264);
	if (nullptr == CodecH264) {
		return Failure("Codec H264 was not found.");
	}

	// get Codec Context
	ContextH264 = avcodec_alloc_context3(CodecH264);
	if (nullptr == ContextH264) {
		return Failure("Failed to allocate codec context.");
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
		return Failure("Failed to initialize codec context to use the h264 codec.");
	}
	av_dict_free(&EncodeOptions);

	// open output file
	auto OutputFilePathInUTF8 = StringCast<UTF8CHAR>(*OutputFilePath);
	IOContext                 = nullptr;
	if (avio_open(&IOContext,
	              reinterpret_cast<const char*>(OutputFilePathInUTF8.Get()),
	              AVIO_FLAG_WRITE) < 0) {
		return Failure(FString::Printf(
		    TEXT("Failed to initialize io context and open file: %s."),
		    *OutputFilePath));
	}

	// allocate memory to FormatContext
	if (avformat_alloc_output_context2(
	        &FormatContext, nullptr, nullptr,
	        reinterpret_cast<const char*>(OutputFilePathInUTF8.Get())) < 0) {
		return Failure("Failed to allocate format context.");
	}

	// set FormatContext to output to specified output file
	FormatContext->pb = IOContext;

	// add new stream to file
	Stream = avformat_new_stream(FormatContext, CodecH264);
	if (nullptr == Stream) {
		return Failure("Failed to add a new stream.");
	}

	// set Stream information
	Stream->sample_aspect_ratio = ContextH264->sample_aspect_ratio;
	Stream->time_base           = ContextH264->time_base;

	// set parameter from codec context h264
	if (avcodec_parameters_from_context(Stream->codecpar, ContextH264) != 0) {
		return Failure("Failed to set parameters.");
	}

	// write header to output file
	if (avformat_write_header(FormatContext, nullptr) != 0) {
		return Failure(
		    "Failed to write the stream header to the output media file.");
	}

	// finish as success
	return Success();
}

void UFFmpegEncoder::Close(FFmpegEncoderCloseResult& Result,
                           FString&                  ErrorMessage) {
	// helper function to finish with failure
	const auto& Failure = [&](const FString& Message) {
		ErrorMessage = Message;
		UE_LOG(LogFFmpegEncoder, Error, TEXT("%s"), *ErrorMessage);
		Result = FFmpegEncoderCloseResult::Failure;
	};

	// notify that encoding is finished
	if (avcodec_send_frame(ContextH264, nullptr) != 0) {
		return Failure("Failed to flush send frame.");
	}

	// allocate Packet
	AVPacket* Packet = av_packet_alloc();

	// receive remaining packets
	while (avcodec_receive_packet(ContextH264, Packet) == 0) {
		// set stream index of this packet from stream
		Packet->stream_index = Stream->index;

		// rescale
		av_packet_rescale_ts(Packet, ContextH264->time_base, Stream->time_base);

		// write Packet to output media file
		if (av_interleaved_write_frame(FormatContext, Packet) != 0) {
			return Failure(
			    "Failed to write frame to the output media file after flush.");
		}

		// unreference the buffer of Packet
		av_packet_unref(Packet);
	}

	// write trailer to output file
	if (av_write_trailer(FormatContext) != 0) {
		return Failure(
		    "Failed to write the stream header to the output media file.");
	}

	// free resources
	av_packet_free(&Packet);
	avcodec_free_context(&ContextH264);
	avformat_free_context(FormatContext);
	avio_closep(&IOContext);
	FrameIndex = 0;
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

void UFFmpegEncoder::AddFrame(const FTextureRHIRef&        TextureRHI,
                              FFmpegEncoderAddFrameResult& Result,
                              FString&                     ErrorMessage) {
	auto Image = CreateImageFromTextureRHI(TextureRHI);
	return AddFrame(MoveTemp(Image), Result, ErrorMessage);
}

void UFFmpegEncoder::AddFrame(const FImage&                Image,
                              FFmpegEncoderAddFrameResult& Result,
                              FString&                     ErrorMessage) {
	auto FFmpegFrameWrapper = FFFmpegFrameWrapper::CreateFrame(
	    Image, FrameIndex, Config.Width, Config.Height);
	return AddFrame(MoveTemp(FFmpegFrameWrapper), Result, ErrorMessage);
}

void UFFmpegEncoder::AddFrame(const FFFmpegFrameWrapper&   Frame,
                              FFmpegEncoderAddFrameResult& Result,
                              FString&                     ErrorMessage) {
	return AddFrame(Frame.GetRawFrame(), Result, ErrorMessage);
}

void UFFmpegEncoder::AddFrame(AVFrame* const               Frame,
                              FFmpegEncoderAddFrameResult& Result,
                              FString&                     ErrorMessage) {
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

	// check FrameIndex is correct
	check(FrameIndex == Frame->pts);

	// send a frame
	if (avcodec_send_frame(ContextH264, Frame) != 0) {
		return Failure("Failed to send frame.");
	}

	// allocate Packet
	AVPacket* Packet = av_packet_alloc();
	if (nullptr == Packet) {
		return Failure("Failed to allocate packet ...");
	}

	// receive a Packet
	while (avcodec_receive_packet(ContextH264, Packet) == 0) {
		check(Packet->size != 0);

		// set stream index of this packet from stream
		Packet->stream_index = Stream->index;

		// rescale
		av_packet_rescale_ts(Packet, ContextH264->time_base, Stream->time_base);
	return Success();

		// write Packet to output media file
		if (av_interleaved_write_frame(FormatContext, Packet) != 0) {
			return Failure(
			    "Failed to write the stream header to the output media file.");
		}

		// unreference the buffer of Packet
		av_packet_unref(Packet);
	}

	// free Packet resource
	av_packet_free(&Packet);

	// increment FrameIndex
	++FrameIndex;
}

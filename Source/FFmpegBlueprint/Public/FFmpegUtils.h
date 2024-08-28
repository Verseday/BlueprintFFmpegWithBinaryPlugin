// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

extern "C" {
#include <libavutil/frame.h>
}

#include <ImageCore.h>

#include "FFmpegUtils.generated.h"

/**
 *
 */
UCLASS()
class FFMPEGBLUEPRINT_API UFFmpegUtils: public UBlueprintFunctionLibrary {
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	static void GenerateVideoFromImageFiles(
	    const FString& OutputFilePath, const TArray<FString>& InputImagePaths,
	    const FFFmpegEncoderConfig& FFmpegEncoderConfig);

public:
	static constexpr AVPixelFormat
	    FFmpegFrameFormatOf(ERawImageFormat::Type UEImageFormat) noexcept;
};

#pragma region          definition of inline functions
constexpr AVPixelFormat UFFmpegUtils::FFmpegFrameFormatOf(
    ERawImageFormat::Type UEImageFormat) noexcept {
	using UEFormat = ERawImageFormat::Type;

	switch (UEImageFormat) {
	case UEFormat::G8:
		return AV_PIX_FMT_GRAY8; // 8-bit grayscale
	case UEFormat::BGRA8:
		return AV_PIX_FMT_BGRA; // 8-bit BGRA
	case UEFormat::BGRE8:
		return AV_PIX_FMT_BGR0; // 8-bit BGRX (similar to BGRE8, no alpha channel)
	case UEFormat::RGBA16:
		return AV_PIX_FMT_RGBA64; // 16-bit RGBA
	case UEFormat::RGBA16F:
		return AV_PIX_FMT_RGBA64; // No direct 16F in FFmpeg, using 64-bit RGBA as
		                          // closest
	case UEFormat::RGBA32F:
		return AV_PIX_FMT_RGBA; // No direct 32F, using 8-bit RGBA as an
		                        // approximation
	case UEFormat::G16:
		return AV_PIX_FMT_GRAY16; // 16-bit grayscale
	case UEFormat::R16F:
		return AV_PIX_FMT_GRAY16; // No direct support, using 16-bit grayscale as
		                          // closest
	case UEFormat::R32F:
		return AV_PIX_FMT_GRAYF32; // 32-bit float grayscale, FFmpeg uses
		                           // AV_PIX_FMT_GRAYF32 for float grayscale
	case UEFormat::MAX:
		return AV_PIX_FMT_NONE; // No valid conversion, use a placeholder
	case UEFormat::Invalid:
		return AV_PIX_FMT_NONE; // Invalid format, return a placeholder
	default:
		return AV_PIX_FMT_NONE; // Fallback for unspecified formats
	}
}
#pragma endregion

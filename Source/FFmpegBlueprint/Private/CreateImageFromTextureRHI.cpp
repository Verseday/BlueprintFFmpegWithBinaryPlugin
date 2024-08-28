// Fill out your copyright notice in the Description page of Project Settings.

#include "CreateImageFromTextureRHI.h"

FImage CreateImageFromTextureRHI(const FTextureRHIRef& TextureRHI) {
	FImage OutImage;

	// get description of source texture RHI
	const auto& Desc = TextureRHI->GetDesc();

	// get Width
	const auto& Width = Desc.Extent.X;

	// get Height
	const auto& Height = Desc.Extent.Y;

	// get PixelFormat
	const auto& PixelFormat = Desc.Format;

	// array of Color in image
	TArray<FColor> ColorArray;

	// ColorArray.Num() becomes Width * Height
	ColorArray.Reserve(Width * Height);

	// On Render Thread
	ENQUEUE_RENDER_COMMAND(ReadTexture)
	([&](FRHICommandListImmediate& RHICmdList) {
		// create settings
		FReadSurfaceDataFlags ReadSurfaceDataFlags;

		// assume color space of TextureRHI is gamma space
		ReadSurfaceDataFlags.SetLinearToGamma(false);

		// read texture color data to ColorArray
		RHICmdList.ReadSurfaceData(TextureRHI, FIntRect(0, 0, Width, Height),
		                           ColorArray, ReadSurfaceDataFlags);
	});

	// wait for complete above Render Thread command
	FlushRenderingCommands();

	// ColorArray should be packed with all the pixel information without wasting
	// a single byte.
	check(Width * Height == ColorArray.Num());

	// initialize OutImage
	OutImage.Init(Width, Height, ERawImageFormat::BGRA8);

	// copy ColorArray to OutImage
	FMemory::Memcpy(OutImage.RawData.GetData(), ColorArray.GetData(),
	                ColorArray.Num() * sizeof(FColor));

	return OutImage;
}

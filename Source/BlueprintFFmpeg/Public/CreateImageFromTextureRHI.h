// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

/**
 * @param TextureRHI   Source TextureRHI from which the image is created.
 * @return created image
 */
FImage CreateImageFromTextureRHI(const FTextureRHIRef& TextureRHI);

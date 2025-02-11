// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter/shell/platform/embedder/embedder_external_texture_metal.h"

#include "flutter/fml/logging.h"
#import "flutter/shell/platform/darwin/graphics/FlutterDarwinExternalTextureMetal.h"
#include "third_party/skia/include/core/SkImage.h"
#include "third_party/skia/include/core/SkSize.h"
#include "third_party/skia/include/gpu/GrBackendSurface.h"
#include "third_party/skia/include/gpu/GrDirectContext.h"

namespace flutter {

static bool ValidNumTextures(int expected, int actual) {
  if (expected == actual) {
    return true;
  } else {
    FML_LOG(ERROR) << "Invalid number of textures, expected: " << expected << ", got: " << actual;
    return false;
  }
}

EmbedderExternalTextureMetal::EmbedderExternalTextureMetal(int64_t texture_identifier,
                                                           const ExternalTextureCallback& callback)
    : Texture(texture_identifier), external_texture_callback_(callback) {
  FML_DCHECK(external_texture_callback_);
}

EmbedderExternalTextureMetal::~EmbedderExternalTextureMetal() = default;

// |flutter::Texture|
void EmbedderExternalTextureMetal::Paint(SkCanvas& canvas,
                                         const SkRect& bounds,
                                         bool freeze,
                                         GrDirectContext* context,
                                         const SkSamplingOptions& sampling,
                                         const SkPaint* paint) {
  if (last_image_ == nullptr) {
    last_image_ = ResolveTexture(Id(), context, SkISize::Make(bounds.width(), bounds.height()));
  }

  if (last_image_) {
    if (bounds != SkRect::Make(last_image_->bounds())) {
      canvas.drawImageRect(last_image_, bounds, sampling, paint);
    } else {
      canvas.drawImage(last_image_, bounds.x(), bounds.y(), sampling, paint);
    }
  }
}

sk_sp<SkImage> EmbedderExternalTextureMetal::ResolveTexture(int64_t texture_id,
                                                            GrDirectContext* context,
                                                            const SkISize& size) {
  std::unique_ptr<FlutterMetalExternalTexture> texture =
      external_texture_callback_(texture_id, size.width(), size.height());

  if (!texture) {
    return nullptr;
  }

  sk_sp<SkImage> image;

  switch (texture->pixel_format) {
    case FlutterMetalExternalTexturePixelFormat::kRGBA: {
      if (ValidNumTextures(1, texture->num_textures)) {
        id<MTLTexture> rgbaTex = (__bridge id<MTLTexture>)texture->textures[0];
        image = [FlutterDarwinExternalTextureSkImageWrapper wrapRGBATexture:rgbaTex
                                                                  grContext:context
                                                                      width:size.width()
                                                                     height:size.height()];
      }
      break;
    }
    case FlutterMetalExternalTexturePixelFormat::kYUVA: {
      if (ValidNumTextures(2, texture->num_textures)) {
        id<MTLTexture> yTex = (__bridge id<MTLTexture>)texture->textures[0];
        id<MTLTexture> uvTex = (__bridge id<MTLTexture>)texture->textures[1];
        SkYUVColorSpace colorSpace =
            texture->yuv_color_space == FlutterMetalExternalTextureYUVColorSpace::kBT601LimitedRange
                ? kRec601_Limited_SkYUVColorSpace
                : kJPEG_Full_SkYUVColorSpace;
        image = [FlutterDarwinExternalTextureSkImageWrapper wrapYUVATexture:yTex
                                                                      UVTex:uvTex
                                                              YUVColorSpace:colorSpace
                                                                  grContext:context
                                                                      width:size.width()
                                                                     height:size.height()];
      }
      break;
    }
  }

  if (!image) {
    FML_LOG(ERROR) << "Could not create external texture: " << texture_id;
  }

  return image;
}

// |flutter::Texture|
void EmbedderExternalTextureMetal::OnGrContextCreated() {}

// |flutter::Texture|
void EmbedderExternalTextureMetal::OnGrContextDestroyed() {}

// |flutter::Texture|
void EmbedderExternalTextureMetal::MarkNewFrameAvailable() {
  last_image_ = nullptr;
}

// |flutter::Texture|
void EmbedderExternalTextureMetal::OnTextureUnregistered() {}

}  // namespace flutter

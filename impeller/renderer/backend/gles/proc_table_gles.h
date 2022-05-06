// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <functional>
#include <vector>

#include "flutter/fml/logging.h"
#include "flutter/fml/macros.h"
#include "flutter/fml/mapping.h"
#include "impeller/renderer/backend/gles/gl_description.h"
#include "impeller/renderer/backend/gles/gles.h"

namespace impeller {

const char* GLErrorToString(GLenum value);

struct AutoErrorCheck {
  const PFNGLGETERRORPROC error_fn;
  const char* name;

  AutoErrorCheck(PFNGLGETERRORPROC error, const char* name)
      : error_fn(error), name(name) {}

  ~AutoErrorCheck() {
    if (error_fn) {
      auto error = error_fn();
      FML_CHECK(error == GL_NO_ERROR)
          << "GL Error " << GLErrorToString(error) << "(" << error << ")"
          << " encountered on call to " << name;
    }
  }
};

template <class T>
struct GLProc {
  using GLFunctionType = T;

  //----------------------------------------------------------------------------
  /// The name of the GL function.
  ///
  const char* name = nullptr;

  //----------------------------------------------------------------------------
  /// The pointer to the GL function.
  ///
  GLFunctionType* function = nullptr;

  //----------------------------------------------------------------------------
  /// An optional error function. If present, all calls will be followed by an
  /// error check.
  ///
  PFNGLGETERRORPROC error_fn = nullptr;

  //----------------------------------------------------------------------------
  /// @brief      Call the GL function with the appropriate parameters. Lookup
  ///             the documentation for the GL function being called to
  ///             understand the arguments and return types. The arguments
  ///             types must match and will be type checked.
  ///
  template <class... Args>
  auto operator()(Args&&... args) const {
    AutoErrorCheck error(error_fn, name);
    return function(std::forward<Args>(args)...);
  }

  constexpr bool IsAvailable() const { return function != nullptr; }

  void Reset() {
    name = nullptr;
    function = nullptr;
    error_fn = nullptr;
  }
};

#define FOR_EACH_IMPELLER_PROC(PROC)         \
  PROC(ActiveTexture);                       \
  PROC(AttachShader);                        \
  PROC(BindAttribLocation);                  \
  PROC(BindBuffer);                          \
  PROC(BindTexture);                         \
  PROC(BlendEquationSeparate);               \
  PROC(BlendFuncSeparate);                   \
  PROC(BufferData);                          \
  PROC(CheckFramebufferStatus);              \
  PROC(Clear);                               \
  PROC(ClearColor);                          \
  PROC(ClearDepthf);                         \
  PROC(ClearStencil);                        \
  PROC(ColorMask);                           \
  PROC(CompileShader);                       \
  PROC(CreateProgram);                       \
  PROC(CreateShader);                        \
  PROC(CullFace);                            \
  PROC(DeleteBuffers);                       \
  PROC(DeleteProgram);                       \
  PROC(DeleteShader);                        \
  PROC(DeleteTextures);                      \
  PROC(DepthFunc);                           \
  PROC(DepthMask);                           \
  PROC(DepthRangef);                         \
  PROC(DetachShader);                        \
  PROC(Disable);                             \
  PROC(DisableVertexAttribArray);            \
  PROC(DrawElements);                        \
  PROC(Enable);                              \
  PROC(EnableVertexAttribArray);             \
  PROC(FrontFace);                           \
  PROC(GenBuffers);                          \
  PROC(GenTextures);                         \
  PROC(GetActiveUniform);                    \
  PROC(GetBooleanv);                         \
  PROC(GetFloatv);                           \
  PROC(GetFramebufferAttachmentParameteriv); \
  PROC(GetIntegerv);                         \
  PROC(GetProgramiv);                        \
  PROC(GetShaderInfoLog);                    \
  PROC(GetShaderiv);                         \
  PROC(GetString);                           \
  PROC(GetUniformLocation);                  \
  PROC(IsFramebuffer);                       \
  PROC(IsProgram);                           \
  PROC(LinkProgram);                         \
  PROC(Scissor);                             \
  PROC(ShaderBinary);                        \
  PROC(ShaderSource);                        \
  PROC(StencilFuncSeparate);                 \
  PROC(StencilMaskSeparate);                 \
  PROC(StencilOpSeparate);                   \
  PROC(TexImage2D);                          \
  PROC(TexParameteri);                       \
  PROC(Uniform1fv);                          \
  PROC(Uniform1i);                           \
  PROC(Uniform2fv);                          \
  PROC(Uniform4fv);                          \
  PROC(UniformMatrix4fv);                    \
  PROC(UseProgram);                          \
  PROC(VertexAttribPointer);                 \
  PROC(Viewport);

#define FOR_EACH_IMPELLER_EXT_PROC(PROC) \
  PROC(PushDebugGroupKHR);               \
  PROC(PopDebugGroupKHR);                \
  PROC(ObjectLabelKHR);

enum class DebugResourceType {
  kTexture,
  kBuffer,
  kProgram,
  kShader,
};

class ProcTableGLES {
 public:
  using Resolver = std::function<void*(const char* function_name)>;
  ProcTableGLES(Resolver resolver);

  ~ProcTableGLES();

#define IMPELLER_PROC(name) \
  GLProc<decltype(gl##name)> name = {"gl" #name, nullptr};

  FOR_EACH_IMPELLER_PROC(IMPELLER_PROC);
  FOR_EACH_IMPELLER_EXT_PROC(IMPELLER_PROC);

#undef IMPELLER_PROC

  bool IsValid() const;

  void ShaderSourceMapping(GLuint shader, const fml::Mapping& mapping) const;

  const GLDescription* GetDescription() const;

  std::string DescribeCurrentFramebuffer() const;

  bool IsCurrentFramebufferComplete() const;

  void SetDebugLabel(DebugResourceType type,
                     GLint name,
                     const std::string& label) const;

 private:
  bool is_valid_ = false;
  std::unique_ptr<GLDescription> description_;

  FML_DISALLOW_COPY_AND_ASSIGN(ProcTableGLES);
};

}  // namespace impeller

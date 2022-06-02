#ifndef NODE_JPEGTURBO_UTIL_H
#define NODE_JPEGTURBO_UTIL_H

#include <napi.h>

#include <turbojpeg.h>

#include "consts.h"

// Napi::Value getProperty(const Napi::Object &obj, const char *name);

struct BufferSizeOptions
{
  bool valid;
  uint32_t width;
  uint32_t height;
  uint32_t subsampling;
};

BufferSizeOptions ParseBufferSizeOptions(const Napi::Env &env, const Napi::Object &obj);

#endif

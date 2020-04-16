#include "exports.h"
using namespace Napi;

Object Init(Env env, Object exports) {
  exports.Set("bufferSize", Function::New(env, BufferSize));
  exports.Set("compressSync", Function::New(env, CompressSync));
  exports.Set("compress", Function::New(env, Compress));
  exports.Set("decompressSync", Function::New(env, DecompressSync));
  exports.Set("decompress", Function::New(env, Decompress));
  exports.Set("FORMAT_RGB", Number::New(env, FORMAT_RGB));
  exports.Set("FORMAT_BGR", Number::New(env, FORMAT_BGR));
  exports.Set("FORMAT_RGBX", Number::New(env, FORMAT_RGBX));
  exports.Set("FORMAT_BGRX", Number::New(env, FORMAT_BGRX));
  exports.Set("FORMAT_XRGB", Number::New(env, FORMAT_XRGB));
  exports.Set("FORMAT_XBGR", Number::New(env, FORMAT_XBGR));
  exports.Set("FORMAT_GRAY", Number::New(env, FORMAT_GRAY));
  exports.Set("FORMAT_RGBA", Number::New(env, FORMAT_RGBA));
  exports.Set("FORMAT_BGRA", Number::New(env, FORMAT_BGRA));
  exports.Set("FORMAT_ABGR", Number::New(env, FORMAT_ABGR));
  exports.Set("FORMAT_ARGB", Number::New(env, FORMAT_ARGB));
  exports.Set("SAMP_444", Number::New(env, SAMP_444));
  exports.Set("SAMP_422", Number::New(env, SAMP_422));
  exports.Set("SAMP_420", Number::New(env, SAMP_420));
  exports.Set("SAMP_GRAY", Number::New(env, SAMP_GRAY));
  exports.Set("SAMP_440", Number::New(env, SAMP_440));
  return exports;
}

NODE_API_MODULE(jpegturbo, Init)

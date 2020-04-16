#include "exports.h"
using namespace Napi;

static char errStr[NJT_MSG_LENGTH_MAX] = "No error";
#define _throw(m) {snprintf(errStr, NJT_MSG_LENGTH_MAX, "%s", m); retval=-1; goto bailout;}

Number BufferSize(const CallbackInfo& info) {
  Env env = info.Env();

  int retval = 0;

  // Input
  Object options;
  Value sampValue;
  uint32_t jpegSubsamp = NJT_DEFAULT_SUBSAMPLING;
  Value widthValue;
  uint32_t width = 0;
  Value heightValue;
  uint32_t height = 0;
  uint32_t dstLength = 0;

  if (info.Length() == 0) {
    _throw("Too few arguments");
  }

  // Options
  options = info[0].As<Object>();
  if (!options.IsObject()) {
    _throw("Options must be an Object");
  }

  // Subsampling
  sampValue = options.Get("subsampling");
  if (!sampValue.IsEmpty() && !sampValue.IsUndefined())
  {
    if (!isNapiInt(env, sampValue))
    {
      _throw("Invalid subsampling method");
    }
    jpegSubsamp = sampValue.ToNumber();
  }

  switch (jpegSubsamp) {
    case SAMP_444:
    case SAMP_422:
    case SAMP_420:
    case SAMP_GRAY:
    case SAMP_440:
      break;
    default:
      _throw("Invalid subsampling method");
  }

  // Width
  widthValue = options.Get("width");
  if (widthValue.IsEmpty() || widthValue.IsUndefined()) {
    _throw("Missing width");
  }
  if (!isNapiInt(env, widthValue))
  {
    _throw("Invalid width value");
  }
  width = widthValue.ToNumber();

  // Height
  heightValue = options.Get("height");
  if (heightValue.IsEmpty() || heightValue.IsUndefined()) {
    _throw("Missing height");
  }
  if (!isNapiInt(env, heightValue))
  {
    _throw("Invalid height value");
  }
  height = heightValue.ToNumber();

  // Finally, calculate the buffer size
  dstLength = tjBufSize(width, height, jpegSubsamp);

  bailout:
  if (retval != 0) {
    TypeError::New(env, errStr).ThrowAsJavaScriptException();
  }

  return Number::New(env, dstLength);
}

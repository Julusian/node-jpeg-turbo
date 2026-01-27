#include "compress.h"

struct CompressProps
{
  unsigned char *srcData;
  uint32_t format;
  uint32_t width;
  uint32_t stride;
  uint32_t height;
  uint32_t subsampling;
  int quality;
  int bpp;
  int flags;
  unsigned long resSize;
  unsigned char *resData;
};

std::string DoCompress(CompressProps &props)
{
  tjhandle handle = tjInitCompress();
  if (handle == nullptr)
  {
    return tjGetErrorStr();
  }

  int err = tjCompress2(handle,
                        props.srcData,
                        props.width,
                        props.stride * props.bpp,
                        props.height,
                        props.format,
                        &props.resData,
                        &props.resSize,
                        props.subsampling,
                        props.quality,
                        props.flags);
  if (err != 0)
  {
    tjDestroy(handle);
    return tjGetErrorStr();
  }

  err = tjDestroy(handle);
  if (err != 0)
  {
    return tjGetErrorStr();
  }

  if (props.resData == nullptr)
  {
    return "No output data";
  }

  return "";
}

Napi::Object CompressResult(const Napi::Env &env, const Napi::Buffer<unsigned char> dstBuffer, const CompressProps &props)
{
  Napi::Object res = Napi::Object::New(env);
  res.Set("data", dstBuffer);
  res.Set("size", props.resSize);

  return res;
}

class CompressWorker : public Napi::AsyncWorker
{
public:
  CompressWorker(
      Napi::Env &env,
      Napi::Buffer<unsigned char> &srcBuffer,
      Napi::Buffer<unsigned char> &dstBuffer,
      CompressProps &props)
      : AsyncWorker(env),
        deferred(Napi::Promise::Deferred::New(env)),
        srcBuffer(Napi::Reference<Napi::Buffer<unsigned char>>::New(srcBuffer, 1)),
        dstBuffer(Napi::Reference<Napi::Buffer<unsigned char>>::New(dstBuffer, 1)),
        props(props)
  {
  }

  ~CompressWorker()
  {
    this->srcBuffer.Reset();
    this->dstBuffer.Reset();
  }

  void Execute()
  {
    std::string err = DoCompress(this->props);
    if (!err.empty())
    {
      SetError(err);
    }
  }

  void OnOK()
  {
    deferred.Resolve(CompressResult(Env(), this->dstBuffer.Value(), this->props));
  }

  void OnError(Napi::Error const &error)
  {
    deferred.Reject(error.Value());
  }

  Napi::Promise GetPromise() const
  {
    return deferred.Promise();
  }

private:
  Napi::Promise::Deferred deferred;
  Napi::Reference<Napi::Buffer<unsigned char>> srcBuffer;
  Napi::Reference<Napi::Buffer<unsigned char>> dstBuffer;
  CompressProps props;
};

Napi::Value CompressInner(const Napi::CallbackInfo &info, bool async)
{
  Napi::Env env = info.Env();

  if (info.Length() < 2)
  {
    Napi::TypeError::New(env, "Not enough arguments")
        .ThrowAsJavaScriptException();
    return env.Null();
  }

  if (!info[0].IsBuffer())
  {
    Napi::TypeError::New(env, "Invalid source buffer")
        .ThrowAsJavaScriptException();
    return env.Null();
  }
  Napi::Buffer<unsigned char> srcBuffer = info[0].As<Napi::Buffer<unsigned char>>();

  unsigned int offset = 0;
  Napi::Buffer<unsigned char> dstBuffer;
  if (info[1].IsBuffer())
  {
    dstBuffer = info[1].As<Napi::Buffer<unsigned char>>();
    offset++;
    if (dstBuffer.Length() == 0)
    {
      Napi::TypeError::New(env, "Invalid destination buffer")
          .ThrowAsJavaScriptException();
      return env.Null();
    }
  }

  if (info.Length() < offset + 2 || !info[offset + 1].IsObject())
  {
    Napi::TypeError::New(env, "Invalid options").ThrowAsJavaScriptException();
    return env.Null();
  }
  Napi::Object options = info[offset + 1].As<Napi::Object>();

  BufferSizeOptions parsedOptions = ParseBufferSizeOptions(env, options);
  if (!parsedOptions.valid)
  {
    return env.Null();
  }

  CompressProps props = {};
  props.srcData = srcBuffer.Data();
  props.width = parsedOptions.width;
  props.height = parsedOptions.height;
  props.subsampling = parsedOptions.subsampling;

  Napi::Value tmpFormat = options.Get("format");
  if (!tmpFormat.IsNumber())
  {
    Napi::TypeError::New(env, "Invalid format").ThrowAsJavaScriptException();
    return env.Null();
  }
  props.format = tmpFormat.As<Napi::Number>().Uint32Value();

  // Figure out bpp from format (needed to calculate output buffer size)
  props.bpp = 0;
  switch (props.format)
  {
  case TJPF_GRAY:
    props.bpp = 1;
    break;
  case TJPF_RGB:
  case TJPF_BGR:
    props.bpp = 3;
    break;
  case TJPF_RGBX:
  case TJPF_BGRX:
  case TJPF_XRGB:
  case TJPF_XBGR:
  case TJPF_RGBA:
  case TJPF_BGRA:
  case TJPF_ABGR:
  case TJPF_ARGB:
    props.bpp = 4;
    break;
  default:
    Napi::TypeError::New(env, "Invalid input format").ThrowAsJavaScriptException();
    return env.Null();
  }

  props.stride = parsedOptions.width;
  Napi::Value tmpStride = options.Get("stride");
  if (!tmpStride.IsUndefined())
  {
    if (!tmpStride.IsNumber())
    {
      Napi::TypeError::New(env, "Invalid stride").ThrowAsJavaScriptException();
      return env.Null();
    }
    props.stride = tmpStride.As<Napi::Number>().Uint32Value();
  }

  props.quality = NJT_DEFAULT_QUALITY;
  Napi::Value tmpQuality = options.Get("quality");
  if (!tmpQuality.IsUndefined())
  {
    if (!tmpQuality.IsNumber())
    {
      Napi::TypeError::New(env, "Invalid quality").ThrowAsJavaScriptException();
      return env.Null();
    }
    props.quality = tmpQuality.As<Napi::Number>().Uint32Value();
  }
  if (props.quality <= 0 || props.quality > 100)
  {
    Napi::TypeError::New(env, "Invalid quality").ThrowAsJavaScriptException();
    return env.Null();
  }

  if (srcBuffer.Length() < static_cast<size_t>(props.stride) * props.height * props.bpp)
  {
    Napi::TypeError::New(env, "Source data is not long enough").ThrowAsJavaScriptException();
    return env.Null();
  }

  uint32_t dstLength = tjBufSize(props.width, props.height, props.subsampling);
  if (dstBuffer.IsEmpty())
  {
    dstBuffer = Napi::Buffer<unsigned char>::New(env, dstLength);
  }

  if (dstLength > dstBuffer.Length())
  {
    Napi::TypeError::New(env, "Insufficient output buffer").ThrowAsJavaScriptException();
    return env.Null();
  }

  props.flags = TJFLAG_FASTDCT | TJFLAG_NOREALLOC;
  props.resSize = dstBuffer.Length();
  props.resData = dstBuffer.Data();

  if (async)
  {
    CompressWorker *wk = new CompressWorker(env, srcBuffer, dstBuffer, props);
    wk->Queue();
    return wk->GetPromise();
  }
  else
  {
    std::string errStr = DoCompress(props);
    if (!errStr.empty())
    {
      Napi::TypeError::New(env, errStr).ThrowAsJavaScriptException();
      return env.Null();
    }

    return CompressResult(env, dstBuffer, props);
  }
}

Napi::Value CompressAsync(const Napi::CallbackInfo &info)
{
  return CompressInner(info, true);
}

Napi::Value CompressSync(const Napi::CallbackInfo &info)
{
  return CompressInner(info, false);
}

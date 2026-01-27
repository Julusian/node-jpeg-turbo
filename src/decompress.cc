#include "compress.h"

struct DecompressProps
{
  tjhandle handle;
  unsigned char *srcData;
  uint32_t srcLength;
  uint32_t format;
  int bpp;
  int resWidth;
  int resHeight;
  unsigned long resSize;
  unsigned char *resData;
};

std::string DoDecompress(DecompressProps &props)
{

  int err = tjDecompress2(props.handle, props.srcData, props.srcLength, props.resData, props.resWidth, 0, props.resHeight, props.format, TJFLAG_FASTDCT);
  if (err != 0)
  {
    tjDestroy(props.handle);
    return tjGetErrorStr();
  }

  err = tjDestroy(props.handle);
  if (err != 0)
  {
    return tjGetErrorStr();
  }

  return "";
}

Napi::Object DecompressResult(const Napi::Env &env, const Napi::Buffer<unsigned char> dstBuffer, const DecompressProps &props)
{
  Napi::Object res = Napi::Object::New(env);
  res.Set("data", dstBuffer);
  res.Set("size", props.resSize);
  res.Set("width", props.resWidth);
  res.Set("height", props.resHeight);
  res.Set("format", props.format);

  return res;
}

class DecompressWorker : public Napi::AsyncWorker
{
public:
  DecompressWorker(
      Napi::Env &env,
      Napi::Buffer<unsigned char> &srcBuffer,
      Napi::Buffer<unsigned char> &dstBuffer,
      DecompressProps &props)
      : AsyncWorker(env),
        deferred(Napi::Promise::Deferred::New(env)),
        srcBuffer(Napi::Reference<Napi::Buffer<unsigned char>>::New(srcBuffer, 1)),
        dstBuffer(Napi::Reference<Napi::Buffer<unsigned char>>::New(dstBuffer, 1)),
        props(props)
  {
  }

  ~DecompressWorker()
  {
    this->srcBuffer.Reset();
    this->dstBuffer.Reset();
  }

  void Execute()
  {
    std::string err = DoDecompress(this->props);
    if (!err.empty())
    {
      SetError(err);
    }
  }

  void OnOK()
  {
    deferred.Resolve(DecompressResult(Env(), this->dstBuffer.Value(), this->props));
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
  DecompressProps props;
};

Napi::Value DecompressInner(const Napi::CallbackInfo &info, bool async)
{
  Napi::Env env = info.Env();

  if (info.Length() < 1)
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

  DecompressProps props = {};
  props.srcData = srcBuffer.Data();
  props.srcLength = srcBuffer.Length();

  if (info.Length() >= offset + 2)
  {
    if (!info[offset + 1].IsObject())
    {
      Napi::TypeError::New(env, "Invalid options").ThrowAsJavaScriptException();
      return env.Null();
    }
    Napi::Object options = info[offset + 1].As<Napi::Object>();

    Napi::Value tmpFormat = options.Get("format");
    if (!tmpFormat.IsNumber())
    {
      Napi::TypeError::New(env, "Invalid format").ThrowAsJavaScriptException();
      return env.Null();
    }
    props.format = tmpFormat.As<Napi::Number>().Uint32Value();
  }

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
    Napi::TypeError::New(env, "Invalid output format").ThrowAsJavaScriptException();
    return env.Null();
  }

  tjhandle handle = tjInitDecompress();
  if (handle == nullptr)
  {
    Napi::TypeError::New(env, tjGetErrorStr()).ThrowAsJavaScriptException();
    return env.Null();
  }

  props.handle = handle;

  int err = tjDecompressHeader(handle, props.srcData, props.srcLength, &props.resWidth, &props.resHeight);
  if (err != 0)
  {
    tjDestroy(handle);

    Napi::TypeError::New(env, tjGetErrorStr()).ThrowAsJavaScriptException();
    return env.Null();
  }

  auto targetSize = static_cast<size_t>(props.resWidth) * props.resHeight * props.bpp;
  if (dstBuffer.IsEmpty())
  {
    dstBuffer = Napi::Buffer<unsigned char>::New(env, targetSize);
  }

  props.resSize = targetSize;
  props.resData = dstBuffer.Data();

  if (targetSize > dstBuffer.Length())
  {
    tjDestroy(handle);

    Napi::TypeError::New(env, "Insufficient output buffer").ThrowAsJavaScriptException();
    return env.Null();
  }

  if (async)
  {
    DecompressWorker *wk = new DecompressWorker(env, srcBuffer, dstBuffer, props);
    wk->Queue();
    return wk->GetPromise();
  }
  else
  {
    std::string errStr = DoDecompress(props);
    if (!errStr.empty())
    {
      Napi::TypeError::New(env, errStr).ThrowAsJavaScriptException();
      return env.Null();
    }

    return DecompressResult(env, dstBuffer, props);
  }
  return env.Null();
}

Napi::Value DecompressAsync(const Napi::CallbackInfo &info)
{
  return DecompressInner(info, true);
}

Napi::Value DecompressSync(const Napi::CallbackInfo &info)
{
  return DecompressInner(info, false);
}

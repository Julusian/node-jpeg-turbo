#include "exports.h"
using namespace Napi;

static char errStr[NJT_MSG_LENGTH_MAX] = "No error";
#define _throw(m) {snprintf(errStr, NJT_MSG_LENGTH_MAX, "%s", m); retval=-1; goto bailout;}

int decompress(unsigned char* srcData, uint32_t srcLength, uint32_t format, int* width, int* height, uint32_t* dstLength, unsigned char** dstData, uint32_t dstBufferLength) {
  int retval = 0;
  int err;
  tjhandle handle = NULL;
  int bpp;

  // Figure out bpp from format (needed to calculate output buffer size)
  switch (format) {
    case FORMAT_GRAY:
      bpp = 1;
      break;
    case FORMAT_RGB:
    case FORMAT_BGR:
      bpp = 3;
      break;
    case FORMAT_RGBX:
    case FORMAT_BGRX:
    case FORMAT_XRGB:
    case FORMAT_XBGR:
    case FORMAT_RGBA:
    case FORMAT_BGRA:
    case FORMAT_ABGR:
    case FORMAT_ARGB:
      bpp = 4;
      break;
    default:
      _throw("Invalid output format");
  }

  handle = tjInitDecompress();
  if (handle == NULL) {
    _throw(tjGetErrorStr());
  }

  err = tjDecompressHeader(handle, srcData, srcLength, width, height);

  if (err != 0) {
    _throw(tjGetErrorStr());
  }

  *dstLength = *width * *height * bpp;

  if (dstBufferLength > 0) {
    if (dstBufferLength < *dstLength) {
      _throw("Insufficient output buffer");
    }
  }
  else {
    *dstData = (unsigned char*)malloc(*dstLength);
  }

  err = tjDecompress2(handle, srcData, srcLength, *dstData, *width, 0, *height, format, TJFLAG_FASTDCT);

  if(err != 0) {
    _throw(tjGetErrorStr());
  }


  bailout:
  if (handle != NULL) {
    err = 0;
    err = tjDestroy(handle);
    // If we already have an error retval wont be 0 so in that case we don't want to overwrite error message
    // Also cant use _throw here because infinite-loop
    if (err != 0 && retval == 0) {
      snprintf(errStr, NJT_MSG_LENGTH_MAX, "%s", tjGetErrorStr());
    }
  }

  return retval;
}

class DecompressWorker : public AsyncWorker {
  public:
    DecompressWorker(Function &callback, unsigned char* srcData, uint32_t srcLength, uint32_t format, Object &dstObject, unsigned char* dstData, uint32_t dstBufferLength) :
      AsyncWorker(callback),
      srcData(srcData),
      srcLength(srcLength),
      format(format),
      dstData(dstData),
      dstBufferLength(dstBufferLength),
      width(0),
      height(0),
      dstLength(0) {
        if (dstBufferLength > 0) {
          this->dstObject = Persistent(dstObject);
        }
      }

    ~DecompressWorker() {}

    void Execute () override {
      int err;

      err = decompress(
          this->srcData,
          this->srcLength,
          this->format,
          &this->width,
          &this->height,
          &this->dstLength,
          &this->dstData,
          this->dstBufferLength);

      if(err != 0) {
        SetError(errStr);
      }
    }

    void OnOK () override {
      Napi::Env env = Callback().Env();
      Object obj = Object::New(env);
      Object dstObject;

      if (this->dstBufferLength > 0) {
        dstObject = this->dstObject.Value();
      }
      else {
        dstObject = Buffer<char>::New(env, (char*)this->dstData, this->dstLength);
      }

      obj.Set("data", dstObject);
      obj.Set("width", this->width);
      obj.Set("height", this->height);
      obj.Set("size", this->dstLength);
      obj.Set("format", this->format);

      Callback().Call({ env.Null(), obj });
    }

  private:
    unsigned char* srcData;
    uint32_t srcLength;
    uint32_t format;

    ObjectReference dstObject;
    unsigned char* dstData;
    uint32_t dstBufferLength;
    int width;
    int height;
    uint32_t dstLength;
};

Value decompressParse(const CallbackInfo& info, bool async) {
  Env env = info.Env();

  int retval = 0;
  int cursor = 0;

  // Input
  Function callback;
  Object srcObject;
  unsigned char* srcData = NULL;
  uint32_t srcLength = 0;
  Object options;
  Value formatValue;
  uint32_t format = NJT_DEFAULT_FORMAT;

  // Output
  Object dstObject;
  uint32_t dstBufferLength = 0;
  unsigned char* dstData = NULL;
  int width;
  int height;
  uint32_t dstLength;

  // Try to find callback here, so if we want to throw something we can use callback's err
  if (async) {
    if (info[info.Length() - 1].IsFunction()) {
      callback = info[info.Length() - 1].As<Function>();
    }
    else {
      _throw("Missing callback");
    }
  }

  if ((async && info.Length() < 2) || (!async && info.Length() < 1)) {
    _throw("Too few arguments");
  }

  // Input buffer
  srcObject = info[cursor++].As<Object>();
  if (!srcObject.IsBuffer()) {
    _throw("Invalid source buffer");
  }

  srcData = (unsigned char*) srcObject.As<Buffer<char>>().Data();
  srcLength = (uint32_t) srcObject.As<Buffer<char>>().Length();

  // Options
  options = info[cursor++].As<Object>();

  // Check if options we just got is actually the destination buffer
  // If it is, pull new object from info and set that as options
  if (options.IsBuffer() && info.Length() > cursor) {
    dstObject = options;
    options = info[cursor++].As<Object>();
    dstBufferLength = (uint32_t) dstObject.As<Buffer<char>>().Length();
    dstData = (unsigned char*) dstObject.As<Buffer<char>>().Data();
  }

  // Options are optional
  if (options.IsObject()) {
    // Format of output buffer
    formatValue = options.Get("format");
    if (!formatValue.IsEmpty() && !formatValue.IsUndefined())
    {
      if (!isNapiInt(env, formatValue))
      {
        _throw("Invalid format");
      }
      format = formatValue.ToNumber();
    }
  }

  // Do either async or sync decompress
  if (async) {
    DecompressWorker* worker = new DecompressWorker(callback, srcData, srcLength, format, dstObject, dstData, dstBufferLength);
    worker->Queue();
    return env.Null();
  }
  else {
    retval = decompress(
        srcData,
        srcLength,
        format,
        &width,
        &height,
        &dstLength,
        &dstData,
        dstBufferLength);


    if(retval != 0) {
      // decompress will set the errStr
      goto bailout;
    }
    Object obj = Object::New(env);

    if (dstBufferLength == 0) {
      dstObject = Buffer<char>::New(env, (char*)dstData, dstLength);
    }

    obj.Set("data", dstObject);
    obj.Set("width", width);
    obj.Set("height", height);
    obj.Set("size", dstLength);
    obj.Set("format", format);

    return obj;
  }

  // If we have error throw error or call callback with error
  bailout:
  if (retval != 0) {
    TypeError::New(env, errStr).ThrowAsJavaScriptException();
  }
  return env.Null();
}

Value DecompressSync(const CallbackInfo& info) {
  return decompressParse(info, false);
}

void Decompress(const CallbackInfo& info) {
  decompressParse(info, true);
}

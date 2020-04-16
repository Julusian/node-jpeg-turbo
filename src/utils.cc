#include "utils.h"

bool isNapiInt(Napi::Env& env, Napi::Value& val) {
  if (!val.IsNumber()) {
    return false;
  }
  Napi::Number num = val.ToNumber();
  return num.DoubleValue() == (double)num.Int32Value();
}

#ifndef NODEPOPPLER_H_
#define NODEPOPPLER_H_

#include <nan.h>
#include <QtCore/QBuffer>
#include <map>
#include <string>

using namespace std;

struct WriteFieldsParams {
  WriteFieldsParams(string a, string b, map<string,string> c) : sourcePdfFileName(a), saveFormat(b), fields(c){}
  string sourcePdfFileName;
  QByteArray sourceBuffer;
  string saveFormat;
  map<string, string> fields;
  int cores;
  double scale_factor;
  bool antialiasing;
};

NAN_METHOD(ReadSync);
NAN_METHOD(ReadBufferSync);
NAN_METHOD(WriteSync);
NAN_METHOD(WriteBufferSync);

WriteFieldsParams v8ParamsToCpp(const Nan::FunctionCallbackInfo<v8::Value>& args, bool isBuffer = false);
QBuffer *writePdfFields(const struct WriteFieldsParams &params, bool isBuffer = false);

// }

#endif  // NODEPOPPLER_H_

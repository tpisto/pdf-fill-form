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
  string saveFormat;
  map<string, string> fields;
};

NAN_METHOD(ReadSync);
NAN_METHOD(WriteSync);

WriteFieldsParams v8ParamsToCpp(const Nan::FunctionCallbackInfo<v8::Value>& args);
QBuffer *writePdfFields(WriteFieldsParams params);

// }

#endif  // NODEPOPPLER_H_

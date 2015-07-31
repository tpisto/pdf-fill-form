#ifndef PISTONPOPPLER_H_
#define PISTONPOPPLER_H_

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

WriteFieldsParams v8ParamsToCpp(const v8::Arguments& args);

QBuffer *writePdfFields(WriteFieldsParams params);

// }

#endif  // PISTONPOPPLER_H_

#include <nan.h>
#include <QtCore/QBuffer>
#include "NodePoppler.h"

using v8::Function;
using v8::Local;
using v8::Null;
using v8::Number;
using v8::Value;
using v8::Object;
using v8::String;
using v8::Array;
using namespace std;

class PdfWriteWorker : public NanAsyncWorker {
  public:
    PdfWriteWorker(NanCallback *callback, string sourcePdfFileName, string saveFormat, map<string, string> fields)
      : NanAsyncWorker(callback), sourcePdfFileName(sourcePdfFileName), saveFormat(saveFormat), fields(fields) {}
    ~PdfWriteWorker() {}

    // Executed inside the worker-thread.
    // It is not safe to access V8, or V8 data structures
    // here, so everything we need for input and output
    // should go on `this`.
    void Execute () {
      buffer = writePdfFields(sourcePdfFileName, saveFormat, fields);
    }

    // Executed when the async work is complete
    // this function will be run inside the main event loop
    // so it is safe to use V8 again
    void HandleOKCallback () {
      NanScope();
      Local<Value> argv[] = {
          NanNull()
        , NanNewBufferHandle(buffer->data().data(), buffer->size())
      };

      callback->Call(2, argv);
    }

  private:
    QBuffer *buffer;
    string sourcePdfFileName;
    string saveFormat;
    map<string, string> fields;

};

// Asynchronous access to the `writePdfFields` function
NAN_METHOD(WriteAsync) {
  NanScope();

  // Convert all parameters to c++ native
  Local<Object> parameters;
  string saveFormat = "imgpdf";
  map<string, string> fields;

  string sourcePdfFileName = *NanAsciiString(args[0]);
  Local<Object> changeFields = args[1]->ToObject();
  
  // Check if any configuration parameters
  if (args.Length() > 2) {
    parameters = args[2]->ToObject();
    Local<Value> saveParam = parameters->Get(NanNew<String>("save"));
    if (!saveParam->IsUndefined()) {
      saveFormat = *NanAsciiString(parameters->Get(NanNew<String>("save")));
    }
  } 

  // Convert form fields to c++ map
  Local<Array> fieldArray = changeFields->GetPropertyNames();
  for (uint32_t i = 0; i < fieldArray->Length(); i += 1) {
    Local<Value> name = fieldArray->Get(i);
    Local<Value> value = changeFields->Get(name);
    fields[std::string(*v8::String::Utf8Value(name))] = std::string(*v8::String::Utf8Value(value));
  }

  // Now do the asynchronus magic
  NanCallback *callback = new NanCallback(args[3].As<Function>());

  NanAsyncQueueWorker(new PdfWriteWorker(callback, sourcePdfFileName, saveFormat, fields));
  NanReturnUndefined();
}
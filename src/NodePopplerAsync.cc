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

class PdfWriteWorker : public Nan::AsyncWorker {
  public:
    PdfWriteWorker(Nan::Callback *callback, struct WriteFieldsParams params)
      : Nan::AsyncWorker(callback), params(params) {}
    ~PdfWriteWorker() {}

    // Executed inside the worker-thread.
    // It is not safe to access V8, or V8 data structures
    // here, so everything we need for input and output
    // should go on `this`.
    void Execute () {
      try
      {
        buffer = writePdfFields(params);
      }
      catch (string error)
      {
        SetErrorMessage(error.c_str());
      }
    }

    // Executed when the async work is complete
    // this function will be run inside the main event loop
    // so it is safe to use V8 again
    void HandleOKCallback () {
      Nan:: HandleScope scope;
      Local<Value> argv[] = {
          Nan::Null()
        , Nan::CopyBuffer((char *)buffer->data().data(), buffer->size()).ToLocalChecked()
      };
      buffer->close();
      delete buffer;
      callback->Call(2, argv);
    }

  private:
    QBuffer *buffer;
    WriteFieldsParams params;
};

// Asynchronous access to the `writePdfFields` function
NAN_METHOD(WriteAsync) {
  WriteFieldsParams params = v8ParamsToCpp(info);

  Nan::Callback *callback = new Nan::Callback(info[3].As<Function>());

  PdfWriteWorker *writeWorker = new PdfWriteWorker(callback, params);
  Nan::AsyncQueueWorker(writeWorker);
}

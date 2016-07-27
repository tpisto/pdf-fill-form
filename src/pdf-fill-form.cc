#include <nan.h>
#include "NodePoppler.h"        // NOLINT(build/include)
#include "NodePopplerAsync.h"   // NOLINT(build/include)

using v8::FunctionTemplate;

// Expose access to our function
NAN_MODULE_INIT(InitAll) {

  Nan::Set(target, Nan::New("readSync").ToLocalChecked(),
    Nan::GetFunction(Nan::New<FunctionTemplate>(ReadSync)).ToLocalChecked());

  Nan::Set(target, Nan::New("writeSync").ToLocalChecked(),
    Nan::GetFunction(Nan::New<FunctionTemplate>(WriteSync)).ToLocalChecked());

  Nan::Set(target, Nan::New("writeAsync").ToLocalChecked(),
    Nan::GetFunction(Nan::New<FunctionTemplate>(WriteAsync)).ToLocalChecked());

}

NODE_MODULE(pdf_fill_form, InitAll)

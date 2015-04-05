#include <nan.h>
#include "NodePoppler.h"   // NOLINT(build/include)
#include "NodePopplerAsync.h"   // NOLINT(build/include)

using v8::FunctionTemplate;
using v8::Handle;
using v8::Object;
using v8::String;

// Expose access to our function
void InitAll(Handle<Object> exports) {
  
  exports->Set(NanNew<String>("readSync"),
    NanNew<FunctionTemplate>(ReadSync)->GetFunction());

  exports->Set(NanNew<String>("writeSync"),
    NanNew<FunctionTemplate>(WriteSync)->GetFunction());

  exports->Set(NanNew<String>("writeAsync"),
    NanNew<FunctionTemplate>(WriteAsync)->GetFunction());

}

NODE_MODULE(pdf_fill_form, InitAll)
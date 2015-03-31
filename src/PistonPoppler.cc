#include <nan.h>
#include "NodePoppler.h"   // NOLINT(build/include)

using v8::FunctionTemplate;
using v8::Handle;
using v8::Object;
using v8::String;

// Expose access to our function
void InitAll(Handle<Object> exports) {
  
  exports->Set(NanNew<String>("readFields"),
    NanNew<FunctionTemplate>(ReadFields)->GetFunction());

}

NODE_MODULE(pistonpoppler, InitAll)
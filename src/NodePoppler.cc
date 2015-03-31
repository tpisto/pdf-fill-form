#include <nan.h>
#include <poppler/qt4/poppler-form.h>
#include <poppler/qt4/poppler-qt4.h>

#include "NodePoppler.h"    // NOLINT(build/include)

using v8::Number;
using v8::String;
using v8::Object;
using v8::Local;
using v8::Array;

// Simple synchronous access to the `Estimate()` function
NAN_METHOD(ReadFields) {  
  NanScope();

  // expect a number as the first argument
  NanUtf8String *fileName = new NanUtf8String(args[0]);
  int n = 0;

  // Poppler document !TODO! - proper error if file not found
  Poppler::Document *document = Poppler::Document::load(**fileName);
  if (document != NULL) {
    // Get field list
    n = document->numPages();
  }

  Local<Array> fieldArray = NanNew<Array>();

  for (int i = 0; i < n; i += 1) {
    Poppler::Page *page = document->page(i);
    int fieldNum = 0;
    foreach (Poppler::FormField *field, page->formFields()) {
      
      if (!field->isReadOnly() && field->isVisible()) {
      
        // Currently we only handle text fields - !TODO!
        if (field->type() == Poppler::FormField::FormText) {

          // Ookay, we now make JavaScript object out of the fieldnames
          Local<Object> obj = NanNew<Object>();
          obj->Set(NanNew<String>("name"), NanNew<String>(field->fullyQualifiedName().toStdString()));
          obj->Set(NanNew<String>("value"), NanNew<String>(((Poppler::FormFieldText *)field)->text().toStdString()));
          obj->Set(NanNew<String>("type"), NanNew<String>("text"));
          obj->Set(NanNew<String>("page"), NanNew<Number>(i));

          fieldArray->Set(fieldNum, obj);
          fieldNum++;
        }
      }
    }
  }

  NanReturnValue(fieldArray);
}
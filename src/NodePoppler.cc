#include <nan.h>
#include <iostream>
#include <poppler/qt4/poppler-form.h>
#include <poppler/qt4/poppler-qt4.h>

#include "NodePoppler.h"    // NOLINT(build/include)

using v8::Number;
using v8::String;
using v8::Object;
using v8::Local;
using v8::Array;


// Read PDF form fields
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

// Write PDF form fields
NAN_METHOD(WriteFields) {  
  NanScope();
  NanUtf8String *fileNameIn = new NanUtf8String(args[0]);
  NanUtf8String *fileNameOut = new NanUtf8String(args[1]);
  Local<Object> changeFields = args[2]->ToObject();

  // Go throught properties
  // Local<Array> properties = fields->GetOwnPropertyNames();
  // for(int i = 0; i < properties->Length(); i++) {
  //   std::cout << *v8::String::Utf8Value(properties->Get(i));
  //   std::cout << *v8::String::Utf8Value(fields->Get(properties->Get(i)));
  // }

  // Poppler document !TODO! - proper error if file not found
  Poppler::Document *document = Poppler::Document::load(**fileNameIn);

  // Fill form
  int n = document->numPages();

  for (int i = 0; i < n; i += 1) {
    Poppler::Page *page = document->page(i);

    foreach(Poppler::FormField *field, page->formFields()) {
      Local<String> name = NanNew<String>(field->fullyQualifiedName().toStdString());
      
      if (!field->isReadOnly() && field->isVisible() && changeFields->Has(name)) {
        if (field->type() == Poppler::FormField::FormText) {
          Poppler::FormFieldText *textField = (Poppler::FormFieldText *) field;
          textField->setText(*v8::String::Utf8Value(changeFields->Get(name)));
        }          
      }
    }
  }

  // Save file
  Poppler::PDFConverter *converter = document->pdfConverter();
  converter->setOutputFileName(**fileNameOut);
  converter->setPDFOptions(converter->pdfOptions() | Poppler::PDFConverter::WithChanges);
  if (!converter->convert()) {
      // Error
  }
}

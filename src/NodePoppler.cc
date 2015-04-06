#include <nan.h>
#include <iostream>
#include <string>
#include <sstream>
#include <map>
#include <stdlib.h>
#include <stdio.h>

#include <poppler/qt4/poppler-form.h>
#include <poppler/qt4/poppler-qt4.h>

#include <cairo/cairo.h>
#include <cairo/cairo-pdf.h>

#include <QtCore/QBuffer>
#include <QtCore/QFile>
#include <QtGui/QImage>

#include "NodePoppler.h"    // NOLINT(build/include)

using namespace std;
using v8::Number;
using v8::String;
using v8::Object;
using v8::Local;
using v8::Array;
using v8::Value;

inline bool fileExists (const std::string& name) {
    return ( access( name.c_str(), F_OK ) != -1 );
}

// Cairo write and read functions (to QBuffer)
static cairo_status_t readPngFromBuffer(void *closure, unsigned char *data, unsigned int length) {
  QBuffer *myBuffer = (QBuffer *)closure;
  size_t bytes_read;
  bytes_read = myBuffer->read((char *)data, length);
  if (bytes_read != length)
  return CAIRO_STATUS_READ_ERROR;

  return CAIRO_STATUS_SUCCESS;
}
static cairo_status_t writePngToBuffer(void *closure, const unsigned char *data, unsigned int length) {
  QBuffer *myBuffer = (QBuffer *)closure;
  size_t bytes_wrote;
  bytes_wrote = myBuffer->write((char *)data, length);
  if (bytes_wrote != length)
  return CAIRO_STATUS_READ_ERROR;

  return CAIRO_STATUS_SUCCESS;
}

void createPdf(QBuffer *buffer, Poppler::Document *document) {
  
  Poppler::PDFConverter *converter = document->pdfConverter();
  converter->setPDFOptions(converter->pdfOptions() | Poppler::PDFConverter::WithChanges);
  converter->setOutputDevice(buffer);
  if (!converter->convert()) {
    throw "Error occurred while trying to create form filled PDF";
  }
}

void createImgPdf(QBuffer *buffer, Poppler::Document *document) {

  // Initialize Cairo writing surface, and since we have images at 360 DPI, we'll set scaling. Width and Height are set for A4 dimensions.
  cairo_surface_t *surface = cairo_pdf_surface_create_for_stream(writePngToBuffer, buffer, 595.00, 842.00);
  cairo_t *cr = cairo_create(surface);
  cairo_scale(cr, 0.2, 0.2);

  int n = document->numPages();
  for (int i = 0; i < n; i += 1) {
    Poppler::Page *page = document->page(i);

    // Save the page as PNG image to buffer. (We could use QFile if we want to save images to files)
    QBuffer *pageImage = new QBuffer();
    pageImage->open(QIODevice::ReadWrite);
    QImage img = page->renderToImage(360,360);
    img.save(pageImage, "png");
    pageImage->seek(0);  // Reading does not work if we don't reset the pointer

    // Ookay, let's try to make new page out of the image
    cairo_surface_t *drawImageSurface = cairo_image_surface_create_from_png_stream(readPngFromBuffer, pageImage);
    cairo_set_source_surface(cr, drawImageSurface, 0, 0);
    cairo_paint(cr);

    // Create new page if multipage document
    if (n > 0) {
      cairo_surface_show_page(surface);
      cr = cairo_create(surface);
      cairo_scale(cr, 0.2, 0.2);
    }

    pageImage->close();
  }

  // Close Cairo
  cairo_destroy(cr);
  cairo_surface_finish(surface);
  cairo_surface_destroy (surface);
}

WriteFieldsParams v8ParamsToCpp(const v8::FunctionCallbackInfo<Value>& args) {
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

  // Create and return PDF
  struct WriteFieldsParams params(sourcePdfFileName, saveFormat, fields);
  return params;
}

// Pdf creator that is not dependent on V8 internals (safe at async?)
QBuffer *writePdfFields(struct WriteFieldsParams params) {

  ostringstream ss;

  // If source file does not exist, throw error and return false
  if(!fileExists(params.sourcePdfFileName)) {
     ss << "File \"" << params.sourcePdfFileName << "\" does not exist";
     throw ss.str();
  }

  // Open document and return false and throw error if something goes wrong
  Poppler::Document *document = Poppler::Document::load(QString::fromStdString(params.sourcePdfFileName));
  if (document == NULL) {
    ss << "Error occurred when reading \"" << params.sourcePdfFileName << "\"";
    throw ss.str();
  }

  // Fill form
  int n = document->numPages();

  for (int i = 0; i < n; i += 1) {
    Poppler::Page *page = document->page(i);

    foreach(Poppler::FormField *field, page->formFields()) {
      string fieldName = field->fullyQualifiedName().toStdString();      
      if (!field->isReadOnly() && field->isVisible() && params.fields.count(fieldName)) {
        if (field->type() == Poppler::FormField::FormText) {
          Poppler::FormFieldText *textField = (Poppler::FormFieldText *) field;
          textField->setText(QString::fromStdString(params.fields[fieldName]));
        }          
      }
    }
  }

  // Now save and return the document
  QBuffer *bufferDevice = new QBuffer();
  bufferDevice->open(QIODevice::ReadWrite);
  
  // Get save parameters
  if (params.saveFormat == "imgpdf") {
    createImgPdf(bufferDevice, document);
  } 
  else { 
    createPdf(bufferDevice, document);
  }

  return bufferDevice;
}


//***********************************
// 
// Node.js methods
// 
//***********************************

// Read PDF form fields
NAN_METHOD(ReadSync) {  
  NanScope();

  // expect a number as the first argument
  NanUtf8String *fileName = new NanUtf8String(args[0]);
  ostringstream ss;
  int n = 0;

  // If file does not exist, throw error and return false
  if(!fileExists(**fileName)) {
     ss << "File \"" << **fileName << "\" does not exist";
     NanThrowError(NanNew<String>(ss.str()));
     NanReturnValue(NanFalse());
  }

  // Open document and return false and throw error if something goes wrong
  Poppler::Document *document = Poppler::Document::load(**fileName);
  if (document != NULL) {
    // Get field list
    n = document->numPages();
  } else {    
     ss << "Error occurred when reading \"" << **fileName << "\"";
    NanThrowError(NanNew<String>(ss.str()));
    NanReturnValue(NanFalse());
  }

  // Store field value objects to v8 array
  Local<Array> fieldArray = NanNew<Array>();
  int fieldNum = 0;

  for (int i = 0; i < n; i += 1) {
    Poppler::Page *page = document->page(i);
    foreach (Poppler::FormField *field, page->formFields()) {
      
      if (!field->isReadOnly() && field->isVisible()) {
      
        // Currently we only handle text fields - !TODO!
        if (field->type() == Poppler::FormField::FormText) {

          // Make JavaScript object out of the fieldnames
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
NAN_METHOD(WriteSync) {  
  NanScope();

  // Check and return parameters given at JavaScript function call
  WriteFieldsParams params = v8ParamsToCpp(args);
  
  // Create and return pdf
  try 
  {
    QBuffer *buffer = writePdfFields(params);
    Local<Object> returnPdf = NanNewBufferHandle(buffer->data().data(), buffer->size());  
    NanReturnValue(returnPdf);    
  } 
  catch (string error) 
  {
    NanThrowError(NanNew<String>(error));
    NanReturnValue(NanNull());
  } 
}



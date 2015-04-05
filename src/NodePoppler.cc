#include <nan.h>
#include <iostream>
#include <string>
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

#define PAGE_WIDTH 595
#define PAGE_HEIGHT 842
#define BORDER 50

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

  cairo_surface_t *surface;
  cairo_t *cr;

  NanUtf8String *fileNameIn = new NanUtf8String(args[0]);
  Local<Object> changeFields = args[1]->ToObject();

  // Poppler template document !TODO! - proper error if file not found
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

  // Save first page as image, we open buffer instead of QFile
  QBuffer *pageImage = new QBuffer();
  pageImage->open(QIODevice::ReadWrite);
  Poppler::Page *page2 = document->page(0);
  QImage img = page2->renderToImage(360,360);
  img.save(pageImage, "png");
  // Reading does not work if we don't reset the pointer
  pageImage->seek(0);

  // Ookay, let's try to make new document out of the image
  // surface = cairo_pdf_surface_create("joo.pdf", 595.00, 842.00);  
  QBuffer *cairoFinalPdf = new QBuffer();
  cairoFinalPdf->open(QIODevice::ReadWrite);
  surface = cairo_pdf_surface_create_for_stream(writePngToBuffer, cairoFinalPdf, 595.00, 842.00);
  cr = cairo_create(surface);
  cairo_scale(cr, 0.2, 0.2);

  // Paint image
  cairo_surface_t *imgPage1 = cairo_image_surface_create_from_png_stream(readPngFromBuffer, pageImage);
  cairo_set_source_surface(cr, imgPage1, 0, 0);
  cairo_paint(cr);

  // cairo_surface_show_page(surface);
  // cr = cairo_create(surface);

  // Close Cairo
  cairo_destroy(cr);
  cairo_surface_finish(surface);
  cairo_surface_destroy (surface);

  // // Save file
  // QBuffer *bufferDevice = new QBuffer();

  // Poppler::PDFConverter *converter = document->pdfConverter();
  // converter->setPDFOptions(converter->pdfOptions() | Poppler::PDFConverter::WithChanges);
  // converter->setOutputDevice(bufferDevice);
  // if (!converter->convert()) {
  //     // Error
  // }
  Local<Object> returnPdf = NanNewBufferHandle(cairoFinalPdf->data().data(), cairoFinalPdf->size());  
  NanReturnValue(returnPdf);
}

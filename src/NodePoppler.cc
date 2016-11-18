#include <nan.h>
#include <iostream>
#include <string>
#include <sstream>
#include <map>
#include <stdlib.h>
#include <stdio.h>

#include <poppler/qt5/poppler-form.h>
#include <poppler/qt5/poppler-qt5.h>

#include <cairo/cairo.h>
#include <cairo/cairo-pdf.h>

#include <QtCore/QBuffer>
#include <QtCore/QFile>
#include <QtGui/QImage>

#include "NodePoppler.h"    // NOLINT(build/include)

using namespace std;
using v8::Number;
using v8::String;
using v8::Local;
using v8::Object;
using v8::Array;
using v8::Value;
using v8::Boolean;

inline bool fileExists (const std::string& name) {
  if (FILE *file = fopen(name.c_str(), "r")) {
    fclose(file);
    return true;
  } else {
    return false;
  }
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
  ostringstream ss;

  Poppler::PDFConverter *converter = document->pdfConverter();
  converter->setPDFOptions(converter->pdfOptions() | Poppler::PDFConverter::WithChanges);
  converter->setOutputDevice(buffer);
  if (!converter->convert()) {
    // enum Error
    // {
    //     NoError,
    //     FileLockedError,
    //     OpenOutputError,
    //     NotSupportedInputFileError
    // };
    ss << "Error occurred when converting PDF: " << converter->lastError();
    delete converter;
    throw ss.str();
  }
  delete converter;
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
    QImage img = page->renderToImage(360, 360);
    img.save(pageImage, "png");
    pageImage->seek(0);  // Reading does not work if we don't reset the pointer

    // Ookay, let's try to make new page out of the image
    cairo_surface_t *drawImageSurface = cairo_image_surface_create_from_png_stream(readPngFromBuffer, pageImage);
    cairo_set_source_surface(cr, drawImageSurface, 0, 0);
    cairo_paint(cr);
    cairo_surface_destroy(drawImageSurface);

    // Create new page if multipage document
    if (n > 0) {
      cairo_surface_show_page(surface);
      if (cr != NULL) {
        cairo_destroy(cr);
      }
      cr = cairo_create(surface);
      cairo_scale(cr, 0.2, 0.2);
    }

    pageImage->close();
    delete pageImage;

    delete page;
  }

  // Close Cairo
  cairo_destroy(cr);
  cairo_surface_finish(surface);
  cairo_surface_destroy (surface);
}

WriteFieldsParams v8ParamsToCpp(const Nan::FunctionCallbackInfo<v8::Value>& args) {
  Local<Object> parameters;
  string saveFormat = "imgpdf";
  map<string, string> fields;

  String::Utf8Value sourcePdfFileNameParam(args[0]->ToString());
  string sourcePdfFileName = string(*sourcePdfFileNameParam);

  Local<Object> changeFields = args[1]->ToObject();
  Local<String> saveStr = Nan::New("save").ToLocalChecked();

  // Check if any configuration parameters
  if (args.Length() > 2) {
    parameters = args[2]->ToObject();
    Local<Value> saveParam = Nan::Get(parameters, saveStr).ToLocalChecked();
    if (!saveParam->IsUndefined()) {
      String::Utf8Value saveFormatParam(Nan::Get(parameters, saveStr).ToLocalChecked());
      saveFormat = string(*saveFormatParam);
    }
  }

  // Convert form fields to c++ map
  Local<Array> fieldArray = changeFields->GetPropertyNames();
  for (uint32_t i = 0; i < fieldArray->Length(); i += 1) {
    Local<Value> name = fieldArray->Get(i);
    Local<Value> value = changeFields->Get(name);
    fields[std::string(*String::Utf8Value(name))] = std::string(*String::Utf8Value(value));
  }

  // Create and return PDF
  struct WriteFieldsParams params(sourcePdfFileName, saveFormat, fields);
  return params;
}

// Pdf creator that is not dependent on V8 internals (safe at async?)
QBuffer *writePdfFields(struct WriteFieldsParams params) {

  ostringstream ss;
  // If source file does not exist, throw error and return false
  if (!fileExists(params.sourcePdfFileName)) {
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
  stringstream idSS;
  for (int i = 0; i < n; i += 1) {
    Poppler::Page *page = document->page(i);
    QList<Poppler::FormField *> formFields = page->formFields();

    foreach (Poppler::FormField *field, formFields) {
      string fieldName = field->fullyQualifiedName().toStdString();
      // Support writing fields by both fieldName and id.
      // If fieldName is not present in params, try id.
      if (params.fields.count(fieldName) == 0) {
        idSS << field->id();
        fieldName = idSS.str();
        idSS.str("");
        idSS.clear();
      }
      if (!field->isReadOnly() && field->isVisible() && params.fields.count(fieldName) ) {
        // Text
        if (field->type() == Poppler::FormField::FormText) {
          Poppler::FormFieldText *textField = (Poppler::FormFieldText *) field;
          textField->setText(QString::fromUtf8(params.fields[fieldName].c_str()));
        }

        // Choice
        if (field->type() == Poppler::FormField::FormChoice) {
          Poppler::FormFieldChoice *choiceField = (Poppler::FormFieldChoice *) field;
          if (choiceField->isEditable()) {
            choiceField->setEditChoice(QString::fromUtf8(params.fields[fieldName].c_str()));
          }
          else {
            QStringList possibleChoices = choiceField->choices();
            QString proposedChoice = QString::fromUtf8(params.fields[fieldName].c_str());
            int index = possibleChoices.indexOf(proposedChoice);
            if (index >= 0) {
              QList<int> choiceList;
              choiceList << index;
              choiceField->setCurrentChoices(choiceList);
            }
          }
        }

        // Button
        // ! TODO ! Note. Poppler doesn't support checkboxes with hashtag names (aka using exportValue).
        if (field->type() == Poppler::FormField::FormButton) {
          Poppler::FormFieldButton *buttonField = (Poppler::FormFieldButton *) field;
          if (params.fields[fieldName].compare("true") == 0) {
            buttonField->setState(true);
          }
          else {
            buttonField->setState(false);
          }
        }
      }
    }

    qDeleteAll(formFields);
    delete page;
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

  delete document;

  return bufferDevice;
}


//***********************************
//
// Node.js methods
//
//***********************************

// Read PDF form fields
NAN_METHOD(ReadSync) {

  // expect a number as the first argument
  Nan::Utf8String *fileName = new Nan::Utf8String(info[0]);
  ostringstream ss;
  int n = 0;

  // If file does not exist, throw error and return false
  if (!fileExists(**fileName)) {
    ss << "File \"" << **fileName << "\" does not exist";
    Nan::ThrowError(Nan::New<String>(ss.str()).ToLocalChecked());
    info.GetReturnValue().Set(Nan::False());
  }

  // Open document and return false and throw error if something goes wrong
  Poppler::Document *document = Poppler::Document::load(**fileName);
  if (document != NULL) {
    // Get field list
    n = document->numPages();
  } else {
    ss << "Error occurred when reading \"" << **fileName << "\"";
    Nan::ThrowError(Nan::New<String>(ss.str()).ToLocalChecked());
    info.GetReturnValue().Set(Nan::False());
  }

  delete fileName;

  // Store field value objects to v8 array
  Local<Array> fieldArray = Nan::New<Array>();
  int fieldNum = 0;

  for (int i = 0; i < n; i += 1) {
    Poppler::Page *page = document->page(i);
    QList<Poppler::FormField *> formFields = page->formFields();

    foreach (Poppler::FormField *field, formFields) {

      if (!field->isReadOnly() && field->isVisible()) {

        // Make JavaScript object out of the fieldnames
        Local<Object> obj = Nan::New<Object>();
        Nan::Set(obj, Nan::New<String>("name").ToLocalChecked(), Nan::New<String>(field->fullyQualifiedName().toStdString()).ToLocalChecked());
        Nan::Set(obj, Nan::New<String>("page").ToLocalChecked(), Nan::New<Number>(i));

        // ! TODO ! Note. Poppler doesn't support checkboxes with hashtag names (aka using exportValue).
        string fieldType;
        // Set default value undefined
        Nan::Set(obj, Nan::New<String>("value").ToLocalChecked(), Nan::Undefined());
        Poppler::FormFieldButton *myButton;
        Poppler::FormFieldChoice *myChoice;

        Nan::Set(obj, Nan::New<String>("id").ToLocalChecked(), Nan::New<Number>(field->id()));
        switch (field->type()) {

          // FormButton
          case Poppler::FormField::FormButton:
            myButton = (Poppler::FormFieldButton *)field;
            switch (myButton->buttonType()) {
              // Push
              case Poppler::FormFieldButton::Push:        fieldType = "push_button";     break;
              case Poppler::FormFieldButton::CheckBox:
                fieldType = "checkbox";
                Nan::Set(obj, Nan::New<String>("value").ToLocalChecked(), Nan::New<Boolean>(myButton->state()));
                break;
              case Poppler::FormFieldButton::Radio:       fieldType = "radio";           break;
            }
            break;

          // FormText
          case Poppler::FormField::FormText:
            Nan::Set(obj, Nan::New<String>("value").ToLocalChecked(), Nan::New<String>(((Poppler::FormFieldText *)field)->text().toStdString()).ToLocalChecked());
            fieldType = "text";
            break;

          // FormChoice
          case Poppler::FormField::FormChoice: {
            Local<Array> choiceArray = Nan::New<Array>();
            myChoice = (Poppler::FormFieldChoice *)field;
            QStringList possibleChoices = myChoice->choices();
            for (int i = 0; i < possibleChoices.size(); i++) {
              Nan::Set(choiceArray, i, Nan::New<String>(possibleChoices.at(i).toStdString()).ToLocalChecked());
            }
            Nan::Set(obj, Nan::New<String>("choices").ToLocalChecked(), choiceArray);
            switch (myChoice->choiceType()) {
              case Poppler::FormFieldChoice::ComboBox:    fieldType = "combobox";       break;
              case Poppler::FormFieldChoice::ListBox:     fieldType = "listbox";        break;
            }
            break;
          }

          // FormSignature
          case Poppler::FormField::FormSignature:         fieldType = "formsignature";  break;

          default:
            fieldType = "undefined";
            break;
        }

        Nan::Set(obj, Nan::New<String>("type").ToLocalChecked(), Nan::New<String>(fieldType.c_str()).ToLocalChecked());

        fieldArray->Set(fieldNum, obj);
        fieldNum++;

      }
    }

    qDeleteAll(formFields);
    delete page;
  }

  info.GetReturnValue().Set(fieldArray);

  delete document;
}

// Write PDF form fields
NAN_METHOD(WriteSync) {

  // Check and return parameters given at JavaScript function call
  WriteFieldsParams params = v8ParamsToCpp(info);

  // Create and return pdf
  try
  {
    QBuffer *buffer = writePdfFields(params);
    Local<Object> returnPdf = Nan::CopyBuffer((char *)buffer->data().data(), buffer->size()).ToLocalChecked();
    buffer->close();
    delete buffer;
    info.GetReturnValue().Set(returnPdf);
  }
  catch (string error)
  {
    Nan::ThrowError(Nan::New<String>(error).ToLocalChecked());
    info.GetReturnValue().Set(Nan::Null());
  }
}

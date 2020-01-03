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
#include <QtCore/QMutex>
#include <QtCore/QRunnable>
#include <QtCore/QSemaphore>
#include <QtCore/QThreadPool>

#include "NodePoppler.h"    // NOLINT(build/include)

using namespace std;
using v8::Number;
using v8::String;
using v8::Local;
using v8::Object;
using v8::Array;
using v8::Value;
using v8::Boolean;
using v8::Isolate;
using v8::Context;

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

class RenderToBufferRunnable : public QRunnable
{
public:
  RenderToBufferRunnable(Poppler::Document *document, int pageNumber, double scale_factor, QHash<int, QPair<QBuffer *, cairo_surface_t *>> *pages, QMutex *pagesMutex, QSemaphore *pagesSemaphore)
   : m_document(document)
   , m_pageNumber(pageNumber)
   , m_scale_factor(scale_factor)
   , m_pages(pages)
   , m_pagesMutex(pagesMutex)
   , m_pagesSemaphore(pagesSemaphore)
  {
  }

  void run() override
  {
    Poppler::Page *page = m_document->page(m_pageNumber);

    // Save the page as PNG image to buffer. (We could use QFile if we want to save images to files)
    QBuffer *pageImage = new QBuffer();
    pageImage->open(QIODevice::ReadWrite);
    QImage img = page->renderToImage(72 / m_scale_factor, 72 / m_scale_factor);
    img.save(pageImage, "png");
    pageImage->seek(0);  // Reading does not work if we don't reset the pointer

    delete page;

    cairo_surface_t *drawImageSurface = cairo_image_surface_create_from_png_stream(readPngFromBuffer, pageImage);

    m_pagesMutex->lock();
    m_pages->insert(m_pageNumber, QPair<QBuffer*, cairo_surface_t *>(pageImage, drawImageSurface));
    m_pagesSemaphore->release();
    m_pagesMutex->unlock();
  }

private:
  Poppler::Document *m_document;
  const int m_pageNumber;
  const double m_scale_factor;
  QHash<int, QPair<QBuffer *, cairo_surface_t *>> *m_pages;
  QMutex *m_pagesMutex;
  QSemaphore *m_pagesSemaphore;
};

void createImgPdf(QBuffer *buffer, Poppler::Document *document, const struct WriteFieldsParams &params) {
  // Calculate max page sizes. The output PDF will have its size set to max width/height.
  double maxWidth = 0;
  double maxHeight = 0;

  int numPages = document->numPages();
  int startPage = params.startPage;
  int endPage = (params.endPage != -1) ? params.endPage + 1 : numPages;
  if ((startPage < 0) || (startPage >= numPages)) {
    startPage = 0;
  }
  if ((endPage <= startPage) || (endPage > numPages)) {
    endPage = numPages;
  }
  bool isMultipage = ((endPage - startPage) > 1);

  for (int i = startPage; i < endPage; i += 1) {
    Poppler::Page *page = document->page(i);

    QSizeF size = page->pageSizeF();

    if (size.height() > maxHeight)
      maxHeight = size.height();

    if (size.width() > maxWidth)
      maxWidth = size.width();
  }

  // Initialize Cairo writing surface, and since we have images at 360 DPI, we'll set scaling.
  cairo_surface_t *surface = cairo_pdf_surface_create_for_stream(writePngToBuffer, buffer, maxWidth, maxHeight);
  cairo_t *cr = cairo_create(surface);
  cairo_scale(cr, params.scale_factor, params.scale_factor);

  if (params.antialiasing) {
    document->setRenderHint(Poppler::Document::Antialiasing);
    document->setRenderHint(Poppler::Document::TextAntialiasing);
  }

  QThreadPool tp;
  tp.setMaxThreadCount(params.cores);

  QMutex pagesMutex;
  QSemaphore pagesSemaphore;
  QHash<int, QPair<QBuffer *, cairo_surface_t *>> pages;

  for (int i = startPage; i < endPage; i += 1) {
    auto task = new RenderToBufferRunnable(document, i, params.scale_factor, &pages, &pagesMutex, &pagesSemaphore);
    tp.start(task);
  }

  for (int i = startPage; i < endPage; i += 1) {
    pagesMutex.lock();
    QBuffer *pageImage = pages[i].first;
    cairo_surface_t *drawImageSurface = pages[i].second;
    pagesMutex.unlock();
    while (drawImageSurface == nullptr) {
      pagesSemaphore.acquire();

      pagesMutex.lock();
      pageImage = pages[i].first;
      drawImageSurface = pages[i].second;
      pagesMutex.unlock();
    }
    // Ookay, let's try to make new page out of the image
    cairo_set_source_surface(cr, drawImageSurface, 0, 0);
    cairo_paint(cr);
    cairo_surface_destroy(drawImageSurface);

    // Create new page if multipage document
    if (isMultipage) {
      cairo_surface_show_page(surface);
      if (cr != NULL) {
        cairo_destroy(cr);
      }
      cr = cairo_create(surface);
      cairo_scale(cr, params.scale_factor, params.scale_factor);
    }

    pageImage->close();
    delete pageImage;
  }

  // Close Cairo
  cairo_destroy(cr);
  cairo_surface_finish(surface);
  cairo_surface_destroy(surface);
}

WriteFieldsParams v8ParamsToCpp(const Nan::FunctionCallbackInfo<v8::Value>& args, bool isBuffer) {
  Isolate* isolate = args.GetIsolate();

  Local<Object> parameters;
  string saveFormat = "imgpdf";
  map<string, string> fields;
  int nCores = 1;
  double scale_factor = 0.2;
  bool antialiasing = false;
  int startPage = 0;
  int endPage = -1;
  string sourcePdfFileName = "";
  QByteArray sourceBuffer;

  if(isBuffer) {
    Local<Object> bufferObj = args[0]->ToObject(Nan::GetCurrentContext()).ToLocalChecked();
    char* bufferData = node::Buffer::Data(bufferObj);
    size_t bufferLength = node::Buffer::Length(bufferObj);

    sourceBuffer = sourceBuffer.append(bufferData, bufferLength);
  } else {
    Nan::Utf8String utf8_value(args[0]);
    sourcePdfFileName = string(*utf8_value);
  }

  Local<Object> changeFields = args[1]->ToObject(Nan::GetCurrentContext()).ToLocalChecked();

  // Check if any configuration parameters
  if (args.Length() > 2) {
    Local<String> saveStr = Nan::New("save").ToLocalChecked();
    Local<String> coresStr = Nan::New("cores").ToLocalChecked();
    Local<String> scaleStr = Nan::New("scale").ToLocalChecked();
    Local<String> antialiasStr = Nan::New("antialias").ToLocalChecked();
    Local<String> startPageStr = Nan::New("startPage").ToLocalChecked();
    Local<String> endPageStr = Nan::New("endPage").ToLocalChecked();

    parameters = args[2]->ToObject(Nan::GetCurrentContext()).ToLocalChecked();

    Local<Value> saveParam = Nan::Get(parameters, saveStr).ToLocalChecked();
    if (!saveParam->IsUndefined()) {
      Nan::Utf8String saveFormatParam(saveParam);
      saveFormat = string(*saveFormatParam);
    }

    Local<Value> coresParam = Nan::Get(parameters, coresStr).ToLocalChecked();
    if (coresParam->IsInt32()) {
      nCores = Nan::To<int32_t>(coresParam).FromJust();
    }

    Local<Value> scaleParam = Nan::Get(parameters, scaleStr).ToLocalChecked();
    if (scaleParam->IsNumber()) {
      scale_factor = scaleParam->NumberValue(Nan::GetCurrentContext()).ToChecked();
    }

    Local<Value> antialiasParam = Nan::Get(parameters, antialiasStr).ToLocalChecked();
    if (antialiasParam->IsBoolean()) {
      #if (NODE_MODULE_VERSION > NODE_11_0_MODULE_VERSION)
      antialiasing = antialiasParam->BooleanValue(isolate);
      #else
      antialiasing = antialiasParam->BooleanValue(isolate->GetCurrentContext()).ToChecked();
      #endif
    }

    Local<Value> startPageParam = Nan::Get(parameters, startPageStr).ToLocalChecked();
    if (startPageParam->IsInt32()) {
      startPage = startPageParam->Int32Value(Nan::GetCurrentContext()).ToChecked();
    }

    Local<Value> endPageParam = Nan::Get(parameters, endPageStr).ToLocalChecked();
    if (endPageParam->Int32Value(Nan::GetCurrentContext()).ToChecked()  ) {
      endPage = endPageParam->Int32Value(Nan::GetCurrentContext()).ToChecked();
    }
  }

  // Convert form fields to c++ map
  Local<Array> fieldArray = Local<Array>::Cast(Nan::GetPropertyNames(changeFields).ToLocalChecked());

  for (unsigned int i = 0; i < fieldArray->Length(); i++ ) {
    if (Nan::Has(fieldArray, i).FromJust()) {
      Local<Value> name = Nan::Get(fieldArray, i).ToLocalChecked();
      Local<Value> value = Nan::Get(changeFields, name).ToLocalChecked();

      v8::String::Utf8Value nameStr(isolate, name);
      v8::String::Utf8Value valueStr(isolate, value);

      fields[std::string(*nameStr)] = std::string(*valueStr);
    }
  }

  struct WriteFieldsParams params(sourcePdfFileName, saveFormat, fields);
  params.cores = nCores;
  params.scale_factor = scale_factor;
  params.antialiasing = antialiasing;
  params.startPage = startPage;
  params.endPage = endPage;
  params.sourceBuffer = sourceBuffer;
  return params;
}

// Pdf creator that is not dependent on V8 internals (safe at async?)
QBuffer *writePdfFields(const struct WriteFieldsParams &params, bool isBuffer) {

  ostringstream ss;
  Poppler::Document *document;

  if(isBuffer) {
    document = Poppler::Document::loadFromData(params.sourceBuffer);
    if (document == NULL) {
      ss << "Error occurred when reading buffer";
      throw ss.str();
    }
  } else {
    // If source file does not exist, throw error and return false
    if (!fileExists(params.sourcePdfFileName)) {
      ss << "File \"" << params.sourcePdfFileName << "\" does not exist";
      throw ss.str();
    }

    // Open document and return false and throw error if something goes wrong
    document = Poppler::Document::load(QString::fromStdString(params.sourcePdfFileName));
    if (document == NULL) {
      ss << "Error occurred when reading \"" << params.sourcePdfFileName << "\"";
      throw ss.str();
    }
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
          textField->setText(QString::fromUtf8(params.fields.at(fieldName).c_str()));
        }

        // Choice
        if (field->type() == Poppler::FormField::FormChoice) {
          Poppler::FormFieldChoice *choiceField = (Poppler::FormFieldChoice *) field;
          if (choiceField->isEditable()) {
            choiceField->setEditChoice(QString::fromUtf8(params.fields.at(fieldName).c_str()));
          }
          else {
            QStringList possibleChoices = choiceField->choices();
            QString proposedChoice = QString::fromUtf8(params.fields.at(fieldName).c_str());
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
          Poppler::FormFieldButton* myButton = (Poppler::FormFieldButton *)field;
          switch (myButton->buttonType()) {
            // Push
            case Poppler::FormFieldButton::Push:     break;
            case Poppler::FormFieldButton::CheckBox:
              if (params.fields.at(fieldName).compare("true") == 0) {
                myButton->setState(true);
              }
              else {
                myButton->setState(false);
              }
              break;
            case Poppler::FormFieldButton::Radio:
              if (params.fields.at(fieldName).compare(myButton->caption().toUtf8().constData()) == 0) {
                myButton->setState(true);
              }
              else {
                myButton->setState(false);
              }
              break;
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
    createImgPdf(bufferDevice, document, params);
  }
  else {
    createPdf(bufferDevice, document);
  }

  delete document;

  return bufferDevice;
}

Local<Array> readPdfFields(Poppler::Document *document) {
  int n = document->numPages();
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
              case Poppler::FormFieldButton::Radio:
                fieldType = "radio";
                Nan::Set(obj, Nan::New<String>("caption").ToLocalChecked(), Nan::New<String>(myButton->caption().toStdString()).ToLocalChecked());
                Nan::Set(obj, Nan::New<String>("value").ToLocalChecked(), Nan::New<Boolean>(myButton->state()));
                break;
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
        Nan::Set(fieldArray, fieldNum, obj);

        fieldNum++;

      }
    }

    qDeleteAll(formFields);
    delete page;
  }

  return fieldArray;
}

//***********************************
//
// Node.js methods
//
//***********************************

// Read PDF form fields
NAN_METHOD(ReadBufferSync) {
  Local<Object> bufferObj = info[0]->ToObject(Nan::GetCurrentContext()).ToLocalChecked();
  char* bufferData = node::Buffer::Data(bufferObj);
  size_t bufferLength = node::Buffer::Length(bufferObj);

  ostringstream ss;
  int n = 0;

  QByteArray buffer;
  buffer = buffer.append(bufferData, bufferLength);
  Poppler::Document *document = Poppler::Document::loadFromData(buffer);

  if (document != NULL) {
    // Get field list
    n = document->numPages();
  } else {
    ss << "Error occurred when reading buffer\"";
    Nan::ThrowError(Nan::New<String>(ss.str()).ToLocalChecked());
    info.GetReturnValue().Set(Nan::False());
  }

  info.GetReturnValue().Set(readPdfFields(document));

  delete document;
}

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

  info.GetReturnValue().Set(readPdfFields(document));

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

// Write PDF form fields
NAN_METHOD(WriteBufferSync) {

  // Check and return parameters given at JavaScript function call
  WriteFieldsParams params = v8ParamsToCpp(info, true);

  // Create and return pdf
  try
  {
    QBuffer *buffer = writePdfFields(params, true);
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

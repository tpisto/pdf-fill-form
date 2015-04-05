#ifndef PISTONPOPPLER_H_
#define PISTONPOPPLER_H_

#include <nan.h>
#include <QtCore/QBuffer>
#include <map>

// class Poppler : public node::ObjectWrap {

NAN_METHOD(ReadFields);
NAN_METHOD(WriteFields);

QBuffer *writePdfFields(std::string, std::string, std::map<std::string, std::string>);

// }

#endif  // EPISTONPOPPLER_H_
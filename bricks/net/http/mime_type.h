/*******************************************************************************
The MIT License (MIT)

Copyright (c) 2015 Ivan Babak <babak.john@gmail.com> https://github.com/sompylasar
                   Dmitry "Dima" Korolev <dmitry.korolev@gmail.com>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*******************************************************************************/

#ifndef BRICKS_NET_HTTP_MIME_TYPE_H
#define BRICKS_NET_HTTP_MIME_TYPE_H

#include <algorithm>
#include <cctype>
#include <map>
#include <string>

#include "../../file/file.h"

namespace current {
namespace net {

inline std::string GetFileMimeType(const std::string& file_name, const std::string& default_type = "text/plain") {
  static const std::map<std::string, std::string> file_extension_to_mime_type_map = {
      {"js", "application/javascript"},
      {"json", "application/json; charset=utf-8"},
      {"css", "text/css"},
      {"html", "text/html"},
      {"htm", "text/html"},
      {"txt", "text/plain"},
      // `.map` are frontend sourcemaps. http://ivanvanderbyl.com/debugging-sourcemaps/#fixingmiddlemanoption1
      {"map", "application/json; charset=utf-8"},
      {"png", "image/png"},
      {"jpg", "image/jpeg"},
      {"jpeg", "image/jpeg"},
      {"gif", "image/gif"},
      {"ico", "image/x-icon"},
      {"svg", "image/svg+xml"}};

  std::string extension = current::FileSystem::GetFileExtension(file_name);
  std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
  const auto cit = file_extension_to_mime_type_map.find(extension);
  if (cit != file_extension_to_mime_type_map.end()) {
    return cit->second;
  } else {
    return default_type;
  }
}

}  // namespace net
}  // namespace current

#endif  // BRICKS_NET_HTTP_MIME_TYPE_H

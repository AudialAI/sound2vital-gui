/* Audial Synth: HTTP client for the Audial API (sound2vital function).
 * GPLv3, same terms as the rest of this source. */

#include "audial_client.h"

#include "json/json.h"

using json = nlohmann::json;

String AudialClient::sanitizeFilename(const String& name) {
  String result;
  for (int i = 0; i < name.length(); ++i) {
    juce_wchar c = name[i];
    bool keep = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') ||
                c == '_' || c == '.' || c == '-';
    result += keep ? String::charToString(c) : String("_");
  }
  if (result.isEmpty() || result.startsWithChar('.'))
    result = "audio" + result;
  return result;
}

String AudialClient::buildRunBody(const String& user_id, const String& filename, const String& file_url) {
  json body;
  body["userId"] = user_id.toStdString();
  body["original"]["filename"] = filename.toStdString();
  body["original"]["url"] = file_url.toStdString();
  return String(body.dump());
}

String AudialClient::parseUrl(const String& body) {
  json parsed = json::parse(body.toStdString(), nullptr, false);
  if (parsed.is_discarded() || !parsed.is_object() || !parsed.count("url") || !parsed["url"].is_string())
    return "";
  return String(parsed["url"].get<std::string>());
}

AudialClient::ExecutionStatus AudialClient::parseExecution(const String& body) {
  ExecutionStatus status;
  json parsed = json::parse(body.toStdString(), nullptr, false);
  if (parsed.is_discarded() || !parsed.is_object()) {
    status.error = "Unreadable execution response";
    return status;
  }
  if (parsed.count("state") && parsed["state"].is_string())
    status.state = String(parsed["state"].get<std::string>());
  if (parsed.count("error") && parsed["error"].is_string())
    status.error = String(parsed["error"].get<std::string>());
  if (parsed.count("preset") && parsed["preset"].is_object()) {
    for (auto& entry : parsed["preset"].items()) {
      if (entry.value().is_object() && entry.value().count("url") && entry.value()["url"].is_string()) {
        status.preset_url = String(entry.value()["url"].get<std::string>());
        break;
      }
    }
  }
  if (status.state == "failed" && status.error.isEmpty())
    status.error = "sound2vital failed";
  return status;
}

String AudialClient::authHeaders() const {
  return "x-api-key: " + credentials_.api_key + "\r\nx-user-id: " + credentials_.user_id + "\r\n";
}

HttpResult AudialClient::request(URL url, bool post_like, const String& method, const String& extra_headers) {
  HttpResult result;
  std::unique_ptr<InputStream> stream = url.createInputStream(post_like, nullptr, nullptr, extra_headers,
                                                              kTimeoutMs, nullptr, &result.status, 5, method);
  if (stream != nullptr)
    result.body = stream->readEntireStreamAsString();
  return result;
}

HttpResult AudialClient::uploadReference(const File& file, const String& exe_id, const String& filename) {
  String path = credentials_.base_url + "/api/files/" + URL::addEscapeChars(credentials_.user_id, false) +
                "/execution/" + URL::addEscapeChars(exe_id, false) + "/reference/" +
                URL::addEscapeChars(filename, false);
  URL url = URL(path).withFileToUpload("file", file, "application/octet-stream");
  return request(url, true, "PUT", authHeaders());
}

HttpResult AudialClient::runSound2Vital(const String& filename, const String& file_url) {
  URL url = URL(credentials_.base_url + "/api/functions/run/sound2vital")
                .withPOSTData(buildRunBody(credentials_.user_id, filename, file_url));
  return request(url, true, "POST", authHeaders() + "Content-Type: application/json\r\n");
}

HttpResult AudialClient::getExecution(const String& exe_id) {
  URL url(credentials_.base_url + "/api/db/" + URL::addEscapeChars(credentials_.user_id, false) +
          "/execution/" + URL::addEscapeChars(exe_id, false));
  return request(url, false, "GET", authHeaders());
}

bool AudialClient::downloadToFile(const String& url, const File& destination) {
  int status = 0;
  std::unique_ptr<InputStream> stream = URL(url).createInputStream(false, nullptr, nullptr, "", kTimeoutMs,
                                                                   nullptr, &status, 5, "GET");
  if (stream == nullptr || status < 200 || status >= 300)
    return false;
  destination.getParentDirectory().createDirectory();
  destination.deleteFile();
  FileOutputStream output(destination);
  if (!output.openedOk())
    return false;
  output.writeFromInputStream(*stream, -1);
  output.flush();
  return destination.existsAsFile() && destination.getSize() > 0;
}

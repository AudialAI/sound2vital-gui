/* Audial Synth: HTTP client for the Audial API (sound2vital function).
 * GPLv3, same terms as the rest of this source. */

#include "audial_client.h"

#include "json/json.h"

using json = nlohmann::json;

namespace {
  // One entry of /api/functions/status: {"function": "sound2vital", "warm": true, "queued": 0,
  // "expectedColdStartMs": ..., "expectedRunMs": ...}. Only "warm" is used here; the estimates
  // are deliberately not shown in the plugin, where the progress bar already carries one.
  bool readFunctionStatusEntry(const json& entry, const String& function,
                               AudialClient::FunctionStatus& status) {
    if (!entry.is_object() || !entry.count("warm") || !entry["warm"].is_boolean())
      return false;
    if (function.isNotEmpty() && entry.count("function") && entry["function"].is_string() &&
        String(entry["function"].get<std::string>()) != function)
      return false;
    status.known = true;
    status.warm = entry["warm"].get<bool>();
    return true;
  }
} // namespace

String HttpResult::describe() const {
  if (status == 0)
    return "no connection (check base URL / network)";
  json parsed = json::parse(body.toStdString(), nullptr, false);
  if (parsed.is_object() && parsed.count("error") && parsed["error"].is_string()) {
    String message = String(parsed["error"].get<std::string>()).trim();
    if (message.isNotEmpty())
      return message;
  }
  return "HTTP " + String(status) + ": " + (body.isEmpty() ? String("<empty>") : body.substring(0, 160));
}

String AudialClient::sanitizeFilename(const String& name) {
  // Same rule as the text2vox plugin (RenderController::runUploadAudio): the Audial API
  // uses the uploaded filename as a Firebase Realtime Database key, which cannot contain
  // ". # $ [ ]", and Ableton-consolidated clips always carry "[timestamp]" in their names.
  // Keep only [A-Za-z0-9_-] in the stem and [.a-z0-9] in the extension.
  int dot = name.lastIndexOfChar('.');
  String stem = dot > 0 ? name.substring(0, dot) : (dot == 0 ? String() : name);
  String extension = dot >= 0 ? name.substring(dot).toLowerCase() : String();
  String result = stem.retainCharacters("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-") +
                  extension.retainCharacters(".abcdefghijklmnopqrstuvwxyz0123456789");
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

String AudialClient::parseExeId(const String& body) {
  json parsed = json::parse(body.toStdString(), nullptr, false);
  if (parsed.is_discarded() || !parsed.is_object() || !parsed.count("exeId") || !parsed["exeId"].is_string())
    return "";
  return String(parsed["exeId"].get<std::string>());
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
  if (parsed.count("warmup") && parsed["warmup"].is_object()) {
    const json& warmup = parsed["warmup"];
    if (warmup.count("state") && warmup["state"].is_string())
      status.warmup_state = String(warmup["state"].get<std::string>());
  }
  if (parsed.count("preset") && parsed["preset"].is_object()) {
    for (auto& entry : parsed["preset"].items()) {
      if (entry.value().is_object() && entry.value().count("url") && entry.value()["url"].is_string()) {
        status.preset_url = String(entry.value()["url"].get<std::string>());
        break;
      }
    }
  }
  if (status.state == "failed" && status.error.isEmpty())
    status.error = "Audial job failed";
  return status;
}

AudialClient::FunctionStatus AudialClient::parseFunctionStatus(const String& body, const String& function) {
  FunctionStatus status;
  json parsed = json::parse(body.toStdString(), nullptr, false);
  if (parsed.is_discarded())
    return status;

  // ?function=<name> is expected to answer with that one function's object. A list (the
  // unfiltered form of the same route) or a map keyed by function name is read too, so the
  // indicator keeps working if the route's shape grows; anything else leaves known = false.
  if (readFunctionStatusEntry(parsed, function, status))
    return status;

  const json* list = nullptr;
  if (parsed.is_array())
    list = &parsed;
  else if (parsed.is_object() && parsed.count("functions") && parsed["functions"].is_array())
    list = &parsed["functions"];
  if (list != nullptr) {
    for (const json& entry : *list) {
      if (readFunctionStatusEntry(entry, function, status))
        return status;
    }
    return status;
  }

  if (parsed.is_object() && function.isNotEmpty() && parsed.count(function.toStdString()))
    readFunctionStatusEntry(parsed[function.toStdString()], String(), status);
  return status;
}

String AudialClient::warmupMessage(const String& warmup_state) {
  // Same sentences as the website pill and the SDK/MCP progress lines. "\xe2\x80\x94" is the
  // em dash, written as UTF-8 bytes so no compiler has to guess this file's encoding.
  if (warmup_state == "initializing")
    return String(CharPointer_UTF8("Warming up the machine \xe2\x80\x94 the first request takes a few "
                                   "minutes longer than usual; every run after this is faster"));
  if (warmup_state == "working")
    return "Working on your job";
  // "cold" is written at dispatch, before any worker is seen starting, and "done" means the
  // job finished; neither is worth a line of its own in a one-line status.
  return "";
}

String AudialClient::clientHeaders() {
  String tag = String(kClientName) + "/" + kClientVersion;
  return "x-audial-client: " + tag + "\r\nUser-Agent: " + tag + " (JUCE)\r\n";
}

String AudialClient::authHeaders() const {
  return "x-api-key: " + credentials_.api_key + "\r\nx-user-id: " + credentials_.user_id + "\r\n";
}

bool AudialClient::onStreamProgress(void* context, int, int) {
  std::atomic<bool>* flag = static_cast<std::atomic<bool>*>(context);
  return !(flag && flag->load());
}

HttpResult AudialClient::request(URL url, bool post_like, const String& method, const String& extra_headers) {
  HttpResult result;
  // Every request to the Audial API identifies the plugin: the backend attributes executions
  // to it by x-audial-client, and JUCE would otherwise send its own default User-Agent.
  std::unique_ptr<InputStream> stream = url.createInputStream(post_like, onStreamProgress, cancel_flag_,
                                                              clientHeaders() + extra_headers, kTimeoutMs,
                                                              nullptr, &result.status, 5, method);
  if (stream != nullptr)
    result.body = stream->readEntireStreamAsString();
  return result;
}

HttpResult AudialClient::uploadReference(const File& file, const String& exe_id, const String& filename) {
  String path = credentials_.base_url + "/api/files/" + URL::addEscapeChars(credentials_.user_id, false) +
                "/execution/" + URL::addEscapeChars(exe_id, false) + "/reference/" +
                URL::addEscapeChars(filename, false);
  // The API records the *multipart* filename (not the URL's) as a Firebase key, and JUCE
  // puts the local file's real name in the multipart body. As in the text2vox plugin,
  // upload via a temp copy bearing the sanitised name.
  File staged = File::getSpecialLocation(File::tempDirectory)
                    .getChildFile("audialsynth_upload_" + exe_id)
                    .getChildFile(filename);
  staged.getParentDirectory().createDirectory();
  staged.deleteFile();
  if (!file.copyFileTo(staged)) {
    HttpResult failed;
    failed.body = "could not stage " + file.getFileName() + " for upload";
    return failed;
  }
  URL url = URL(path).withFileToUpload("file", staged, "application/octet-stream");
  HttpResult result = request(url, true, "PUT", authHeaders());
  staged.deleteFile();
  staged.getParentDirectory().deleteFile();
  return result;
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

HttpResult AudialClient::getFunctionStatus(const String& function) {
  URL url(credentials_.base_url + "/api/functions/status?function=" +
          URL::addEscapeChars(function, true));
  return request(url, false, "GET", authHeaders());
}

bool AudialClient::downloadToFile(const String& url, const File& destination) {
  int status = 0;
  std::unique_ptr<InputStream> stream = URL(url).createInputStream(false, onStreamProgress, cancel_flag_,
                                                                   "", kTimeoutMs, nullptr, &status, 5, "GET");
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

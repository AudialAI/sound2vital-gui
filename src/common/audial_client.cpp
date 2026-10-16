/* Audial Synth: HTTP client for the Audial API (sound2vital function).
 * GPLv3, same terms as the rest of this source. */

#include "audial_client.h"

#include "json/json.h"

#if !JUCE_WINDOWS
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

using json = nlohmann::json;

namespace {
  // The form the Audial SDK stores: the API root including "/api", no trailing slash.
  String storedBaseUrl(const String& base_url) {
    return base_url.trimCharactersAtEnd("/") + "/api";
  }

  String stringField(const json& object, const char* name) {
    if (object.count(name) && object[name].is_string())
      return String(object[name].get<std::string>());
    return String();
  }

  HttpResult postJson(const String& url, const json& body, const String& extra_headers = String()) {
    HttpResult result;
    std::unique_ptr<InputStream> stream = URL(url).withPOSTData(String(body.dump())).createInputStream(
        true, nullptr, nullptr, "Content-Type: application/json\r\n" + extra_headers,
        AudialDeviceLogin::kTimeoutMs, nullptr, &result.status, 5, "POST");
    if (stream != nullptr)
      result.body = stream->readEntireStreamAsString();
    return result;
  }
} // namespace

File AudialSignIn::defaultFile() {
#if JUCE_WINDOWS
  return File::getSpecialLocation(File::userApplicationDataDirectory).getChildFile("Audial").getChildFile("credentials.json");
#else
  return File::getSpecialLocation(File::userHomeDirectory).getChildFile(".audial").getChildFile("credentials.json");
#endif
}

AudialSignIn AudialSignIn::load(const String& base_url, const File& file) {
  AudialSignIn sign_in;
  if (!file.existsAsFile())
    return sign_in;
  json parsed = json::parse(file.loadFileAsString().toStdString(), nullptr, false);
  if (parsed.is_discarded() || !parsed.is_object() || stringField(parsed, "api_base_url") != storedBaseUrl(base_url))
    return sign_in;
  sign_in.api_key = stringField(parsed, "api_key");
  sign_in.user_id = stringField(parsed, "user_id");
  sign_in.email = stringField(parsed, "email");
  sign_in.key_name = stringField(parsed, "key_name");
  if (!sign_in.valid())
    return AudialSignIn();
  return sign_in;
}

bool AudialSignIn::save(const String& base_url, const File& file) const {
  json data;
  data["api_key"] = api_key.toStdString();
  data["user_id"] = user_id.toStdString();
  if (email.isNotEmpty())
    data["email"] = email.toStdString();
  else
    data["email"] = nullptr;
  if (key_name.isNotEmpty())
    data["key_name"] = key_name.toStdString();
  else
    data["key_name"] = nullptr;
  data["api_base_url"] = storedBaseUrl(base_url).toStdString();
  std::string text = data.dump(2);

  if (!file.getParentDirectory().createDirectory())
    return false;
#if JUCE_WINDOWS
  return file.replaceWithText(String(text));  // %APPDATA% is already private to the user
#else
  // Created private from the start, so the key is never readable by other users.
  int fd = ::open(file.getFullPathName().toRawUTF8(), O_WRONLY | O_CREAT | O_TRUNC, 0600);
  if (fd < 0)
    return false;
  bool written = ::write(fd, text.data(), text.size()) == (ssize_t)text.size();
  ::fchmod(fd, 0600);
  ::close(fd);
  return written;
#endif
}

void AudialSignIn::clear(const File& file) {
  file.deleteFile();
}

AudialCredentials AudialSignIn::inUse(const AudialCredentials& typed, const File& file) {
  AudialCredentials credentials = typed;
  if (credentials.base_url.isEmpty())
    credentials.base_url = AudialClient::kAudialApiBaseUrl;
  AudialSignIn sign_in = load(credentials.base_url, file);
  if (sign_in.valid()) {
    credentials.user_id = sign_in.user_id;
    credentials.api_key = sign_in.api_key;
  }
  return credentials;
}

AudialDeviceLogin::Pending AudialDeviceLogin::parseStart(int status, const String& body) {
  Pending pending;
  json parsed = json::parse(body.toStdString(), nullptr, false);
  if (status != 200 || parsed.is_discarded() || !parsed.is_object())
    return pending;
  pending.device_code = stringField(parsed, "device_code");
  pending.user_code = stringField(parsed, "user_code");
  pending.verification_url = stringField(parsed, "verification_uri_complete");
  if (parsed.count("expires_in") && parsed["expires_in"].is_number())
    pending.expires_in_seconds = jmax(1, parsed["expires_in"].get<int>());
  if (parsed.count("interval") && parsed["interval"].is_number())
    pending.interval_seconds = jlimit(1, 60, parsed["interval"].get<int>());
  return pending;
}

AudialDeviceLogin::PollResult AudialDeviceLogin::parsePoll(int status, const String& body) {
  PollResult result;
  json parsed = json::parse(body.toStdString(), nullptr, false);
  if (parsed.is_discarded() || !parsed.is_object())
    return result;
  if (status == 200) {
    result.sign_in.api_key = stringField(parsed, "api_key");
    result.sign_in.user_id = stringField(parsed, "user_id");
    result.sign_in.email = stringField(parsed, "email");
    result.sign_in.key_name = stringField(parsed, "key_name");
    result.state = result.sign_in.valid() ? State::kApproved : State::kFailed;
    return result;
  }
  String error = stringField(parsed, "error");
  if (error == "authorization_pending" || error == "slow_down")
    result.state = State::kPending;
  else if (error == "access_denied")
    result.state = State::kDenied;
  else if (error == "expired_token")
    result.state = State::kExpired;
  return result;
}

AudialDeviceLogin::Pending AudialDeviceLogin::start(const String& base_url, const String& client_name) {
  json body;
  body["client_name"] = client_name.toStdString();
  body["device_name"] = deviceName().toStdString();
  HttpResult response = postJson(base_url + "/api/auth/device/code", body);
  return parseStart(response.status, response.body);
}

AudialDeviceLogin::PollResult AudialDeviceLogin::poll(const String& base_url, const String& device_code) {
  json body;
  body["device_code"] = device_code.toStdString();
  HttpResult response = postJson(base_url + "/api/auth/device/token", body);
  // No answer at all (offline for a moment) is not a refusal: keep waiting.
  if (response.status == 0) {
    PollResult waiting;
    waiting.state = State::kPending;
    return waiting;
  }
  return parsePoll(response.status, response.body);
}

void AudialDeviceLogin::revoke(const String& base_url, const String& api_key) {
  postJson(base_url + "/api/auth/revoke", json::object(), "x-api-key: " + api_key + "\r\n");
}

String AudialDeviceLogin::deviceName() {
  String name = SystemStats::getComputerName().upToFirstOccurrenceOf(".", false, false).trim();
  return name.isNotEmpty() ? name : String("this computer");
}

String HttpResult::describe() const {
  if (status == 0)
    return "no connection (check base URL / network)";
  json parsed = json::parse(body.toStdString(), nullptr, false);
  if (parsed.is_object() && parsed.count("error") && parsed["error"].is_string()) {
    String message = String(parsed["error"].get<std::string>()).trim();
    if (message.isNotEmpty())
      return message;
  }
  if (status == 401 || status == 403)
    return "Audial did not accept the sign-in. Sign in again (or check your API key)";
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

String AudialClient::authHeaders() const {
  return "x-api-key: " + credentials_.api_key + "\r\nx-user-id: " + credentials_.user_id + "\r\n";
}

bool AudialClient::onStreamProgress(void* context, int, int) {
  std::atomic<bool>* flag = static_cast<std::atomic<bool>*>(context);
  return !(flag && flag->load());
}

HttpResult AudialClient::request(URL url, bool post_like, const String& method, const String& extra_headers) {
  HttpResult result;
  std::unique_ptr<InputStream> stream = url.createInputStream(post_like, onStreamProgress, cancel_flag_,
                                                              extra_headers, kTimeoutMs, nullptr,
                                                              &result.status, 5, method);
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

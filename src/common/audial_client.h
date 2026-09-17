/* Audial Synth: HTTP client for the Audial API (sound2vital function).
 * GPLv3, same terms as the rest of this source. */
#pragma once

#include "JuceHeader.h"

struct AudialCredentials {
  String base_url;
  String user_id;
  String api_key;

  bool complete() const {
    return base_url.isNotEmpty() && user_id.isNotEmpty() && api_key.isNotEmpty();
  }
};

struct HttpResult {
  int status = 0;
  String body;

  bool ok() const { return status >= 200 && status < 300; }

  String describe() const {
    if (status == 0)
      return "no connection (check base URL / network)";
    return "HTTP " + String(status) + ": " + (body.isEmpty() ? String("<empty>") : body.substring(0, 160));
  }
};

class AudialClient {
  public:
    struct ExecutionStatus {
      String state;
      String preset_url;
      String error;
    };

    static constexpr int kTimeoutMs = 60000;

    explicit AudialClient(AudialCredentials credentials) : credentials_(std::move(credentials)) { }

    static String sanitizeFilename(const String& name);
    static String buildRunBody(const String& user_id, const String& filename, const String& file_url);
    static String parseUrl(const String& body);
    static ExecutionStatus parseExecution(const String& body);

    HttpResult uploadReference(const File& file, const String& exe_id, const String& filename);
    HttpResult runSound2Vital(const String& filename, const String& file_url);
    HttpResult getExecution(const String& exe_id);
    bool downloadToFile(const String& url, const File& destination);

  private:
    HttpResult request(URL url, bool post_like, const String& method, const String& extra_headers);
    String authHeaders() const;

    AudialCredentials credentials_;
};

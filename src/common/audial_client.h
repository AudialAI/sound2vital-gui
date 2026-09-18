/* Audial Synth: HTTP client for the Audial API (sound2vital function).
 * GPLv3, same terms as the rest of this source. */
#pragma once

#include "JuceHeader.h"

#include <atomic>

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

    // Bounds every blocking network call: ~ResynthSection waits kTimeoutMs + 2 s for the
    // job thread, so this is what a host sees in the worst case when a window is closed
    // mid-transfer.
    static constexpr int kTimeoutMs = 15000;

    explicit AudialClient(AudialCredentials credentials) : credentials_(std::move(credentials)) { }

    // Optional cancellation flag, owned by the caller and polled while data is uploaded.
    // May be null, in which case nothing is cancellable and only kTimeoutMs applies.
    void setCancelFlag(std::atomic<bool>* flag) { cancel_flag_ = flag; }

    static String sanitizeFilename(const String& name);
    static String buildRunBody(const String& user_id, const String& filename, const String& file_url);
    static String parseUrl(const String& body);
    static String parseExeId(const String& body);
    static ExecutionStatus parseExecution(const String& body);

    HttpResult uploadReference(const File& file, const String& exe_id, const String& filename);
    HttpResult runSound2Vital(const String& filename, const String& file_url);
    HttpResult getExecution(const String& exe_id);
    bool downloadToFile(const String& url, const File& destination);

  private:
    static bool onStreamProgress(void* context, int bytes_sent, int total_bytes);

    HttpResult request(URL url, bool post_like, const String& method, const String& extra_headers);
    String authHeaders() const;

    AudialCredentials credentials_;
    std::atomic<bool>* cancel_flag_ = nullptr;
};

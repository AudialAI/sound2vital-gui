/* Audial Synth: HTTP client for the Audial API (sound2vital function).
 * GPLv3, same terms as the rest of this source. */
#pragma once

#include "JuceHeader.h"

#include <atomic>

// The API base URL is baked in at build time (scripts/build_macos.sh selects it via
// AUDIAL_ENV); users sign in through their browser (AudialDeviceLogin below) or, failing
// that, enter a user id and API key. See docs/build-notes.md.
#ifndef AUDIAL_API_BASE_URL
#define AUDIAL_API_BASE_URL "https://api.audialmusic.ai"
#endif

struct AudialCredentials {
  // Always kAudialApiBaseUrl in practice (see LoadSave::loadAudialCredentials()); kept as a
  // field, rather than dropped, so AudialClient and tests can still construct credentials
  // directly. Left empty, AudialClient substitutes kAudialApiBaseUrl.
  String base_url;
  String user_id;
  String api_key;

  bool complete() const {
    return user_id.isNotEmpty() && api_key.isNotEmpty();
  }
};

// The Audial sign-in for this computer.
//
// Signing in from any Audial client (this synth, the text2vox plugin, the `audial` command
// line, the Audial MCP server) writes one file that all of them read:
//
//   macOS / Linux   ~/.audial/credentials.json
//   Windows         %APPDATA%\Audial\credentials.json
//
//   { "api_key", "user_id", "email", "key_name", "api_base_url" }
//
// "api_base_url" is the API the key was issued by ("<base>/api"). A sign-in is only used
// with that API, so a dev or mock build never sends its key to production or vice versa.
struct AudialSignIn {
  String api_key;
  String user_id;
  String email;
  String key_name;

  bool valid() const {
    return api_key.isNotEmpty() && user_id.isNotEmpty();
  }

  static File defaultFile();
  // The saved sign-in for base_url, or an empty value when there is none or it belongs to
  // another API.
  static AudialSignIn load(const String& base_url, const File& file = defaultFile());
  // Writes the sign-in, readable only by the current user.
  bool save(const String& base_url, const File& file = defaultFile()) const;
  static void clear(const File& file = defaultFile());

  // What requests are made with: the sign-in when there is one, otherwise the user id and
  // API key typed into the Resynth panel.
  static AudialCredentials inUse(const AudialCredentials& typed, const File& file = defaultFile());
};

// "Sign in with Audial": the OAuth device authorization flow.
//
//   POST {base}/api/auth/device/code   { client_name, device_name }
//        -> { device_code, user_code, verification_uri_complete, interval, expires_in }
//   (the user opens verification_uri_complete in their browser and approves)
//   POST {base}/api/auth/device/token  { device_code }      polled every `interval` s
//        -> 200 { api_key, user_id, email, key_name }
//        -> 400 { error: authorization_pending | slow_down | access_denied | expired_token }
//
// The network calls block; use them from a background thread.
struct AudialDeviceLogin {
  struct Pending {
    String device_code;
    String user_code;         // shown in the panel and on the approval page
    String verification_url;  // opens the approval page with the code filled in
    int expires_in_seconds = 600;
    int interval_seconds = 5;

    bool ok() const {
      return device_code.isNotEmpty() && verification_url.isNotEmpty();
    }
  };

  enum class State { kPending, kApproved, kDenied, kExpired, kFailed };

  struct PollResult {
    State state = State::kFailed;
    AudialSignIn sign_in;  // filled in when state == kApproved
  };

  static constexpr int kTimeoutMs = 10000;

  static Pending parseStart(int status, const String& body);
  static PollResult parsePoll(int status, const String& body);

  static Pending start(const String& base_url, const String& client_name);
  static PollResult poll(const String& base_url, const String& device_code);
  // Signing out: asks the API to revoke api_key. Best effort.
  static void revoke(const String& base_url, const String& api_key);
  static String deviceName();
};

struct HttpResult {
  int status = 0;
  String body;

  bool ok() const { return status >= 200 && status < 300; }

  // Human-readable failure text for the overlay. The Audial API answers errors with
  // JSON {"error": "...", "code": "..."}; when the body parses as that, the message is
  // shown on its own (a 402 SUBSCRIPTION_REQUIRED reads as "This feature needs an
  // active Audial subscription..."). Anything else falls back to "HTTP <status>: <body>".
  String describe() const;
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

    // Baked in at build time; see the AUDIAL_API_BASE_URL macro above.
    static constexpr const char* kAudialApiBaseUrl = AUDIAL_API_BASE_URL;

    explicit AudialClient(AudialCredentials credentials) : credentials_(std::move(credentials)) {
      if (credentials_.base_url.isEmpty())
        credentials_.base_url = kAudialApiBaseUrl;
    }

    // Optional cancellation flag, owned by the caller and polled while data is uploaded.
    // May be null, in which case nothing is cancellable and only kTimeoutMs applies.
    void setCancelFlag(std::atomic<bool>* flag) { cancel_flag_ = flag; }

    // The base URL every request is actually composed against (kAudialApiBaseUrl when the
    // credentials this client was built with left base_url empty). Exposed for tests.
    const String& effectiveBaseUrl() const { return credentials_.base_url; }

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

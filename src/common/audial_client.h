/* Audial Synth: HTTP client for the Audial API (sound2vital function).
 * GPLv3, same terms as the rest of this source. */
#pragma once

#include "JuceHeader.h"

#include <atomic>

// The API base URL is baked in at build time (scripts/build_macos.sh selects it via
// AUDIAL_ENV); users only ever enter a user id and API key. See docs/build-notes.md.
#ifndef AUDIAL_API_BASE_URL
#define AUDIAL_API_BASE_URL "https://api.audialmusic.ai"
#endif

// The Audial release version of this plugin, sent to the API on every request so the backend
// can attribute executions to the synth (x-audial-client / User-Agent). JUCE_APP_VERSION is the
// version define every exporter carries (the osx/vs19/linux projects, generated from the
// .jucer "version" attribute, which is the single place to bump). It is deliberately not
// ProjectInfo::versionString: that one stays on upstream Vital's 1.0.6 because it is written
// into presets as "synth_version" and compared against when loading them.
#define AUDIAL_STRINGIFY_HELPER(x) #x
#define AUDIAL_STRINGIFY(x) AUDIAL_STRINGIFY_HELPER(x)
#ifdef JUCE_APP_VERSION
#define AUDIAL_SYNTH_VERSION AUDIAL_STRINGIFY(JUCE_APP_VERSION)
#else
// No exporter: keep this in step with the .jucer version.
#define AUDIAL_SYNTH_VERSION "1.0.1"
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
      // execution.warmup.state as written by the API: "cold", "initializing", "working" or
      // "done"; empty when the API left the object off (the worker was already warm) or when
      // this build talks to an API that predates it.
      String warmup_state;
    };

    // GET /api/functions/status?function=... , which the resynth panel reads when it opens so
    // the user is told a cold start is coming before they drop a file. "known" is false when
    // the endpoint is missing (404 on an older API), unreachable, or answers something
    // unparseable; the indicator stays hidden in that case rather than guessing.
    struct FunctionStatus {
      bool known = false;
      bool warm = false;
    };

    // Bounds every blocking network call: ~ResynthSection waits kTimeoutMs + 2 s for the
    // job thread, so this is what a host sees in the worst case when a window is closed
    // mid-transfer.
    static constexpr int kTimeoutMs = 15000;

    // Baked in at build time; see the AUDIAL_API_BASE_URL macro above.
    static constexpr const char* kAudialApiBaseUrl = AUDIAL_API_BASE_URL;

    // How this plugin names itself to the API; the backend maps "audial-synth" to the
    // "resynth" client in its adoption figures.
    static constexpr const char* kClientName = "audial-synth";
    static constexpr const char* kClientVersion = AUDIAL_SYNTH_VERSION;

    // The function whose readiness the resynth panel shows.
    static constexpr const char* kResynthFunction = "sound2vital";

    // The one copy of the warm-up notice, so plugin, website, SDK and MCP all say the same
    // thing. Empty for any state that should not be announced ("cold" before dispatch,
    // "done", an unknown state, or no warmup object at all).
    static String warmupMessage(const String& warmup_state);

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
    static FunctionStatus parseFunctionStatus(const String& body, const String& function);

    // "x-audial-client: audial-synth/<version>" + "User-Agent: audial-synth/<version> (JUCE)",
    // CRLF-terminated, as every request to the Audial API carries them. Exposed for tests.
    static String clientHeaders();

    HttpResult uploadReference(const File& file, const String& exe_id, const String& filename);
    HttpResult runSound2Vital(const String& filename, const String& file_url);
    HttpResult getExecution(const String& exe_id);
    HttpResult getFunctionStatus(const String& function);
    bool downloadToFile(const String& url, const File& destination);

  private:
    static bool onStreamProgress(void* context, int bytes_sent, int total_bytes);

    HttpResult request(URL url, bool post_like, const String& method, const String& extra_headers);
    String authHeaders() const;

    AudialCredentials credentials_;
    std::atomic<bool>* cancel_flag_ = nullptr;
};

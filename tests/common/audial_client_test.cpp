/* Audial Synth: unit tests for the pure helpers of the Audial API client.
 * GPLv3, same terms as the rest of this source. */

#include "audial_client.h"
#include "resynth_section.h"

class AudialClientTest : public UnitTest {
  public:
    AudialClientTest() : UnitTest("Audial Client", "Audial") { }

    void runTest() override {
      beginTest("sanitize filename (text2vox rule: stem [A-Za-z0-9_-], extension [.a-z0-9])");
      expectEquals(AudialClient::sanitizeFilename("My Kick [2026].wav"), String("MyKick2026.wav"));
      expectEquals(AudialClient::sanitizeFilename("other (5) [2026-09-24 100306].wav"), String("other52026-09-24100306.wav"));
      expectEquals(AudialClient::sanitizeFilename("a.b.c.WAV"), String("abc.wav"));
      expectEquals(AudialClient::sanitizeFilename(".hidden"), String("audio.hidden"));
      expectEquals(AudialClient::sanitizeFilename(""), String("audio"));

      beginTest("run body");
      expectEquals(AudialClient::buildRunBody("u1", "kick.wav", "https://cdn/k"),
                   String("{\"original\":{\"filename\":\"kick.wav\",\"url\":\"https://cdn/k\"},\"userId\":\"u1\"}"));

      beginTest("parse upload url");
      expectEquals(AudialClient::parseUrl("{\"url\":\"https://cdn/x\"}"), String("https://cdn/x"));
      expectEquals(AudialClient::parseUrl("not json"), String());
      expectEquals(AudialClient::parseUrl("{\"other\":1}"), String());

      beginTest("parse exe id");
      expectEquals(AudialClient::parseExeId("{\"exeId\":\"e1\",\"state\":\"created\"}"), String("e1"));
      expectEquals(AudialClient::parseExeId("{\"state\":\"created\"}"), String());
      expectEquals(AudialClient::parseExeId("<html>"), String());

      beginTest("describe http results");
      HttpResult no_connection;
      expectEquals(no_connection.describe(), String("no connection (check base URL / network)"));
      HttpResult subscription;
      subscription.status = 402;
      subscription.body = "{\"error\":\"This feature needs an active Audial subscription. Subscribe at audialmusic.ai and try again.\",\"code\":\"SUBSCRIPTION_REQUIRED\"}";
      expectEquals(subscription.describe(),
                   String("This feature needs an active Audial subscription. Subscribe at audialmusic.ai and try again."));
      HttpResult unauthorized;
      unauthorized.status = 403;
      unauthorized.body = "Unauthorized";
      expectEquals(unauthorized.describe(), String("HTTP 403: Unauthorized"));
      HttpResult empty_error;
      empty_error.status = 500;
      empty_error.body = "{\"error\":\"\"}";
      expectEquals(empty_error.describe(), String("HTTP 500: {\"error\":\"\"}"));
      HttpResult empty;
      empty.status = 502;
      expectEquals(empty.describe(), String("HTTP 502: <empty>"));

      beginTest("job time estimate grows with sample length");
      expectWithinAbsoluteError(ResynthSection::estimateJobSeconds(0.0), 90.0, 1e-9);
      expectWithinAbsoluteError(ResynthSection::estimateJobSeconds(10.0), 225.0, 1e-9);
      expect(ResynthSection::estimateJobSeconds(-1.0) >= ResynthSection::estimateJobSeconds(0.0));

      beginTest("parse execution");
      AudialClient::ExecutionStatus done = AudialClient::parseExecution(
          "{\"state\":\"completed\",\"preset\":{\"presetvital\":{\"filename\":\"preset.vital\",\"url\":\"https://cdn/p\"}}}");
      expectEquals(done.state, String("completed"));
      expectEquals(done.preset_url, String("https://cdn/p"));
      expect(done.error.isEmpty());

      AudialClient::ExecutionStatus failed = AudialClient::parseExecution("{\"state\":\"failed\",\"error\":\"Input is 25.0 s\"}");
      expectEquals(failed.state, String("failed"));
      expectEquals(failed.error, String("Input is 25.0 s"));

      AudialClient::ExecutionStatus processing = AudialClient::parseExecution("{\"state\":\"processing\"}");
      expectEquals(processing.state, String("processing"));
      expect(processing.preset_url.isEmpty());

      AudialClient::ExecutionStatus garbage = AudialClient::parseExecution("<html>");
      expectEquals(garbage.state, String());
      expect(garbage.error.isNotEmpty());

      AudialClient::ExecutionStatus failed_silently = AudialClient::parseExecution("{\"state\":\"failed\"}");
      expectEquals(failed_silently.error, String("Audial job failed"));

      beginTest("parse execution warm-up phase");
      expectEquals(AudialClient::parseExecution(
                       "{\"state\":\"processing\",\"warmup\":{\"state\":\"initializing\",\"delayMs\":12}}").warmup_state,
                   String("initializing"));
      expectEquals(AudialClient::parseExecution(
                       "{\"state\":\"processing\",\"warmup\":{\"state\":\"working\"}}").warmup_state,
                   String("working"));
      // A warm job carries no warmup object, and an older API none at all.
      expect(AudialClient::parseExecution("{\"state\":\"processing\"}").warmup_state.isEmpty());
      expect(AudialClient::parseExecution("{\"state\":\"processing\",\"warmup\":null}").warmup_state.isEmpty());
      expect(AudialClient::parseExecution("{\"state\":\"processing\",\"warmup\":{}}").warmup_state.isEmpty());

      beginTest("warm-up messages are the shared copy, and only for the two live phases");
      expect(AudialClient::warmupMessage("initializing").startsWith("Warming up the machine"));
      expect(AudialClient::warmupMessage("initializing").contains("every run after this is faster"));
      expectEquals(AudialClient::warmupMessage("working"), String("Working on your job"));
      expect(AudialClient::warmupMessage("cold").isEmpty());
      expect(AudialClient::warmupMessage("done").isEmpty());
      expect(AudialClient::warmupMessage("").isEmpty());
      expect(AudialClient::warmupMessage("something-new").isEmpty());

      beginTest("every request identifies the plugin");
      String headers = AudialClient::clientHeaders();
      String tag = String("audial-synth/") + AudialClient::kClientVersion;
      expectEquals(headers, String("x-audial-client: " + tag + "\r\nUser-Agent: " + tag + " (JUCE)\r\n"));
      // The version comes from the exporters' JUCE_APP_VERSION define, i.e. from the .jucer
      // projects; it is not Vital's ProjectInfo::versionString, which presets are stamped with.
      expect(String(AudialClient::kClientVersion).containsChar('.'));
      expect(String(AudialClient::kClientVersion) != String(ProjectInfo::versionString));

      beginTest("parse function status");
      // The shape the route answers a ?function= query with.
      AudialClient::FunctionStatus warm = AudialClient::parseFunctionStatus(
          "{\"function\":\"sound2vital\",\"warm\":true,\"queued\":0,\"expectedColdStartMs\":180000}",
          "sound2vital");
      expect(warm.known);
      expect(warm.warm);
      AudialClient::FunctionStatus cold = AudialClient::parseFunctionStatus(
          "{\"function\":\"sound2vital\",\"warm\":false}", "sound2vital");
      expect(cold.known);
      expect(!cold.warm);
      // A list or a map keyed by function name is read too, so the indicator survives the
      // route answering with every function.
      AudialClient::FunctionStatus from_list = AudialClient::parseFunctionStatus(
          "{\"functions\":[{\"function\":\"text2vox\",\"warm\":false},{\"function\":\"sound2vital\",\"warm\":true}]}",
          "sound2vital");
      expect(from_list.known);
      expect(from_list.warm);
      AudialClient::FunctionStatus from_map = AudialClient::parseFunctionStatus(
          "{\"sound2vital\":{\"warm\":false}}", "sound2vital");
      expect(from_map.known);
      expect(!from_map.warm);
      // Unknown stays unknown: another function, no "warm", a 404 body, or nothing at all.
      expect(!AudialClient::parseFunctionStatus("{\"function\":\"text2vox\",\"warm\":true}", "sound2vital").known);
      expect(!AudialClient::parseFunctionStatus("{\"function\":\"sound2vital\"}", "sound2vital").known);
      expect(!AudialClient::parseFunctionStatus("{\"error\":\"Not found\"}", "sound2vital").known);
      expect(!AudialClient::parseFunctionStatus("<html>", "sound2vital").known);
      expect(!AudialClient::parseFunctionStatus("", "sound2vital").known);

      beginTest("network timeout is bounded");
      // ~ResynthSection waits kTimeoutMs + 2000 ms for the job thread, so this constant is what a
      // host is blocked for in the worst case when its window is closed mid-transfer.
      expectEquals(AudialClient::kTimeoutMs, 15000);

      beginTest("credentials completeness");
      AudialCredentials creds { "https://api.audialmusic.ai", "u1", "k" };
      expect(creds.complete());
      creds.api_key = "";
      expect(!creds.complete());
      // base_url plays no part in completeness: the URL is baked in at build time now.
      AudialCredentials no_base_url { "", "u1", "k" };
      expect(no_base_url.complete());

      beginTest("baked-in base URL");
      expect(String(AudialClient::kAudialApiBaseUrl).isNotEmpty());
      expect(String(AudialClient::kAudialApiBaseUrl).startsWith("https://"));

      beginTest("a client built with an empty base_url composes URLs from kAudialApiBaseUrl");
      AudialClient client(AudialCredentials { "", "u1", "k" });
      expectEquals(client.effectiveBaseUrl(), String(AudialClient::kAudialApiBaseUrl));

      beginTest("a client built with an explicit base_url keeps it");
      AudialClient explicit_client(AudialCredentials { "https://example.test", "u1", "k" });
      expectEquals(explicit_client.effectiveBaseUrl(), String("https://example.test"));
    }
};

static AudialClientTest audial_client_test;

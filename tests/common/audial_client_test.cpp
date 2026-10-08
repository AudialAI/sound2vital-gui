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
      expect(unauthorized.describe().contains("Sign in again"));
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

      beginTest("device sign-in: start response");
      AudialDeviceLogin::Pending pending = AudialDeviceLogin::parseStart(200,
          "{\"device_code\":\"dc\",\"user_code\":\"BCDF-GHJK\","
          "\"verification_uri_complete\":\"https://audialmusic.ai/activate?code=BCDF-GHJK\","
          "\"expires_in\":600,\"interval\":5}");
      expect(pending.ok());
      expectEquals(pending.user_code, String("BCDF-GHJK"));
      expectEquals(pending.interval_seconds, 5);
      expectEquals(pending.expires_in_seconds, 600);
      expect(!AudialDeviceLogin::parseStart(500, "oops").ok());
      expectEquals(AudialDeviceLogin::parseStart(200,
          "{\"device_code\":\"d\",\"verification_uri_complete\":\"u\",\"interval\":0}").interval_seconds, 1);

      beginTest("device sign-in: poll responses");
      expect(AudialDeviceLogin::parsePoll(400, "{\"error\":\"authorization_pending\"}").state ==
             AudialDeviceLogin::State::kPending);
      expect(AudialDeviceLogin::parsePoll(400, "{\"error\":\"slow_down\"}").state == AudialDeviceLogin::State::kPending);
      expect(AudialDeviceLogin::parsePoll(400, "{\"error\":\"access_denied\"}").state == AudialDeviceLogin::State::kDenied);
      expect(AudialDeviceLogin::parsePoll(400, "{\"error\":\"expired_token\"}").state == AudialDeviceLogin::State::kExpired);
      expect(AudialDeviceLogin::parsePoll(502, "bad gateway").state == AudialDeviceLogin::State::kFailed);
      AudialDeviceLogin::PollResult approved = AudialDeviceLogin::parsePoll(200,
          "{\"api_key\":\"aud_k\",\"user_id\":\"u1\",\"email\":\"a@b.c\",\"key_name\":\"Audial Synth on Mac\"}");
      expect(approved.state == AudialDeviceLogin::State::kApproved);
      expectEquals(approved.sign_in.user_id, String("u1"));
      expect(AudialDeviceLogin::parsePoll(200, "{\"api_key\":\"aud_k\"}").state == AudialDeviceLogin::State::kFailed);

      beginTest("the sign-in file is shared with the other Audial clients and tied to one API");
      // A temporary file, never the real ~/.audial/credentials.json.
      File sign_in_file = File::getSpecialLocation(File::tempDirectory).getChildFile("audialsynth_sign_in_test.json");
      sign_in_file.deleteFile();
      expect(!AudialSignIn::load("https://api.audialmusic.ai", sign_in_file).valid());
      expect(approved.sign_in.save("https://api.audialmusic.ai/", sign_in_file));
      AudialSignIn loaded = AudialSignIn::load("https://api.audialmusic.ai", sign_in_file);
      expect(loaded.valid());
      expectEquals(loaded.api_key, String("aud_k"));
      expectEquals(loaded.email, String("a@b.c"));
      expect(sign_in_file.loadFileAsString().contains("\"api_base_url\": \"https://api.audialmusic.ai/api\""));
      expect(!AudialSignIn::load("http://localhost:3000", sign_in_file).valid());

      beginTest("a sign-in is used in place of a typed API key; without one the typed key is used");
      AudialCredentials typed { "https://api.audialmusic.ai", "typed-user", "typed-key" };
      AudialCredentials in_use = AudialSignIn::inUse(typed, sign_in_file);
      expectEquals(in_use.user_id, String("u1"));
      expectEquals(in_use.api_key, String("aud_k"));
      AudialSignIn::clear(sign_in_file);
      expect(!sign_in_file.existsAsFile());
      expectEquals(AudialSignIn::inUse(typed, sign_in_file).api_key, String("typed-key"));

      beginTest("a client built with an explicit base_url keeps it");
      AudialClient explicit_client(AudialCredentials { "https://example.test", "u1", "k" });
      expectEquals(explicit_client.effectiveBaseUrl(), String("https://example.test"));
    }
};

static AudialClientTest audial_client_test;

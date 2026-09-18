/* Audial Synth: unit tests for the pure helpers of the Audial API client.
 * GPLv3, same terms as the rest of this source. */

#include "audial_client.h"

class AudialClientTest : public UnitTest {
  public:
    AudialClientTest() : UnitTest("Audial Client") { }

    void runTest() override {
      beginTest("sanitize filename keeps [A-Za-z0-9_.-]");
      expectEquals(AudialClient::sanitizeFilename("My Kick [2026].wav"), String("My_Kick__2026_.wav"));
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
    }
};

static AudialClientTest audial_client_test;

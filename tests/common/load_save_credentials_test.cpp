#include "load_save.h"
#include "audial_client.h"

class LoadSaveCredentialsTest : public UnitTest {
  public:
    LoadSaveCredentialsTest() : UnitTest("LoadSave Audial credentials", "Audial") { }

    void runTest() override {
      // LoadSave has no config-path override (LoadSave::getConfigFile() is fixed), so this test
      // has to write the real user config. It saves the current values first and restores them at
      // the end, and the last block asserts the restore actually took.
      AudialCredentials previous = LoadSave::loadAudialCredentials();

      beginTest("roundtrip");
      LoadSave::saveAudialCredentials("user-1", "key-1");
      AudialCredentials loaded = LoadSave::loadAudialCredentials();
      expectEquals(loaded.base_url, String(AudialClient::kAudialApiBaseUrl));
      expectEquals(loaded.user_id, String("user-1"));
      expectEquals(loaded.api_key, String("key-1"));

      beginTest("base url is always the build's baked-in URL, regardless of any stored value");
      // Simulate an old config that still carries a user-entered "audial_base_url": save
      // credentials (which now drops that key), then poke it back in directly to prove
      // loadAudialCredentials() ignores it rather than reading it back out.
      json data = LoadSave::getConfigJson();
      data["audial_base_url"] = "https://stale.example.test";
      LoadSave::saveJsonToConfig(data);
      expectEquals(LoadSave::loadAudialCredentials().base_url, String(AudialClient::kAudialApiBaseUrl));

      beginTest("saving credentials deletes any stored audial_base_url");
      LoadSave::saveAudialCredentials("user-1", "key-1");
      expect(!LoadSave::getConfigJson().count("audial_base_url"));

      beginTest("complete() ignores base_url");
      AudialCredentials creds = LoadSave::loadAudialCredentials();
      creds.user_id = "user-1";
      creds.api_key = "key-1";
      creds.base_url = "";
      expect(creds.complete());

      LoadSave::saveAudialCredentials(previous.user_id.toStdString(), previous.api_key.toStdString());

      beginTest("the user's own credentials are restored");
      AudialCredentials restored = LoadSave::loadAudialCredentials();
      expectEquals(restored.base_url, String(AudialClient::kAudialApiBaseUrl));
      expectEquals(restored.user_id, previous.user_id);
      expectEquals(restored.api_key, previous.api_key);
    }
};

static LoadSaveCredentialsTest load_save_credentials_test;

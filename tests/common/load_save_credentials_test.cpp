#include "load_save.h"

class LoadSaveCredentialsTest : public UnitTest {
  public:
    LoadSaveCredentialsTest() : UnitTest("LoadSave Audial credentials") { }

    void runTest() override {
      // LoadSave has no config-path override (LoadSave::getConfigFile() is fixed), so this test
      // has to write the real user config. It saves the current values first and restores them at
      // the end, and the last block asserts the restore actually took.
      AudialCredentials previous = LoadSave::loadAudialCredentials();

      beginTest("roundtrip");
      LoadSave::saveAudialCredentials("https://example.test", "user-1", "key-1");
      AudialCredentials loaded = LoadSave::loadAudialCredentials();
      expectEquals(loaded.base_url, String("https://example.test"));
      expectEquals(loaded.user_id, String("user-1"));
      expectEquals(loaded.api_key, String("key-1"));

      beginTest("default base url");
      LoadSave::saveAudialCredentials("", "user-1", "key-1");
      expectEquals(LoadSave::loadAudialCredentials().base_url, String("https://api.audialmusic.ai"));

      beginTest("trailing slash is trimmed");
      LoadSave::saveAudialCredentials("https://example.test/", "user-1", "key-1");
      expectEquals(LoadSave::loadAudialCredentials().base_url, String("https://example.test"));

      LoadSave::saveAudialCredentials(previous.base_url.toStdString(), previous.user_id.toStdString(),
                                      previous.api_key.toStdString());

      beginTest("the user's own credentials are restored");
      AudialCredentials restored = LoadSave::loadAudialCredentials();
      expectEquals(restored.base_url, previous.base_url);
      expectEquals(restored.user_id, previous.user_id);
      expectEquals(restored.api_key, previous.api_key);
    }
};

static LoadSaveCredentialsTest load_save_credentials_test;

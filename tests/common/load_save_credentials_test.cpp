#include "load_save.h"

class LoadSaveCredentialsTest : public UnitTest {
  public:
    LoadSaveCredentialsTest() : UnitTest("LoadSave Audial credentials") { }

    void runTest() override {
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

      LoadSave::saveAudialCredentials(previous.base_url.toStdString(), previous.user_id.toStdString(),
                                      previous.api_key.toStdString());
    }
};

static LoadSaveCredentialsTest load_save_credentials_test;

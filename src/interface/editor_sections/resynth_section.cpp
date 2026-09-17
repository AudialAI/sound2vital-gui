/* Audial Synth: drop a sample, run sound2vital, load the returned preset. GPLv3. */
#include "resynth_section.h"

#include "fonts.h"
#include "load_save.h"
#include "skin.h"
#include "synth_base.h"
#include "synth_gui_interface.h"

ResynthSection::ResynthSection(String name) : Overlay(name), body_(Shaders::kRoundedRectangleFragment),
                                              drop_zone_(Shaders::kRoundedRectangleFragment), job_(this) {
  format_manager_.registerBasicFormats();
  addOpenGlComponent(&body_);
  addOpenGlComponent(&drop_zone_);

  title_text_ = std::make_unique<PlainTextComponent>("title", "Resynth a sample");
  title_text_->setTextSize(20.0f);
  title_text_->setFontType(PlainTextComponent::kLight);
  addOpenGlComponent(title_text_.get());

  help_text_ = std::make_unique<PlainTextComponent>("help",
      "Drop a one-shot (20 s max). Audial builds an editable patch from it.");
  help_text_->setTextSize(13.0f);
  help_text_->setFontType(PlainTextComponent::kLight);
  addOpenGlComponent(help_text_.get());

  drop_text_ = std::make_unique<PlainTextComponent>("drop", "Drop a .wav / .aif / .flac / .mp3 here, or click Browse");
  drop_text_->setTextSize(14.0f);
  drop_text_->setFontType(PlainTextComponent::kLight);
  addOpenGlComponent(drop_text_.get());

  status_text_ = std::make_unique<PlainTextComponent>("status", "");
  status_text_->setTextSize(13.0f);
  status_text_->setFontType(PlainTextComponent::kLight);
  addOpenGlComponent(status_text_.get());

  credentials_text_ = std::make_unique<PlainTextComponent>("credentials", "Audial API credentials");
  credentials_text_->setTextSize(13.0f);
  credentials_text_->setFontType(PlainTextComponent::kLight);
  addOpenGlComponent(credentials_text_.get());

  base_url_ = std::make_unique<OpenGlTextEditor>("Base URL");
  user_id_ = std::make_unique<OpenGlTextEditor>("User ID");
  api_key_ = std::make_unique<OpenGlTextEditor>("API Key", L'*');
  for (OpenGlTextEditor* editor : { base_url_.get(), user_id_.get(), api_key_.get() }) {
    editor->setMultiLine(false);
    addAndMakeVisible(editor);
    addOpenGlComponent(editor->getImageComponent());
  }

  browse_button_ = std::make_unique<OpenGlToggleButton>(TRANS("Browse"));
  save_button_ = std::make_unique<OpenGlToggleButton>(TRANS("Save credentials"));
  cancel_button_ = std::make_unique<OpenGlToggleButton>(TRANS("Cancel"));
  close_button_ = std::make_unique<OpenGlToggleButton>(TRANS("Close"));
  browse_button_->setUiButton(true);
  save_button_->setUiButton(false);
  cancel_button_->setUiButton(false);
  close_button_->setUiButton(false);
  for (OpenGlToggleButton* button : { browse_button_.get(), save_button_.get(), cancel_button_.get(), close_button_.get() }) {
    button->addListener(this);
    addAndMakeVisible(button);
    addOpenGlComponent(button->getGlComponent());
  }
  cancel_button_->setVisible(false);
}

ResynthSection::~ResynthSection() {
  chooser_.reset();
  job_.stopThread(1000);
}

Rectangle<int> ResynthSection::getPanelRect() {
  int x = (getWidth() - kPanelWidth) / 2;
  int y = (getHeight() - kPanelHeight) / 2;
  return Rectangle<int>(x, y, kPanelWidth, kPanelHeight);
}

Rectangle<int> ResynthSection::getDropRect() {
  Rectangle<int> panel = getPanelRect();
  return Rectangle<int>(panel.getX() + kPaddingX, panel.getY() + kPaddingY + 64,
                        panel.getWidth() - 2 * kPaddingX, kDropZoneHeight);
}

void ResynthSection::setTextColors(OpenGlTextEditor* editor, const String& empty_text) {
  editor->setColour(CaretComponent::caretColourId, findColour(Skin::kTextEditorCaret, true));
  editor->setColour(TextEditor::textColourId, findColour(Skin::kPresetText, true));
  editor->setColour(TextEditor::highlightedTextColourId, findColour(Skin::kBodyText, true));
  editor->setColour(TextEditor::highlightColourId, findColour(Skin::kTextEditorSelection, true));
  Colour empty_color = findColour(Skin::kBodyText, true);
  editor->setTextToShowWhenEmpty(empty_text, empty_color.withAlpha(0.5f * empty_color.getFloatAlpha()));
  editor->applyFontToAllText(Fonts::instance()->proportional_light().withPointHeight(14.0f), true);
  editor->redoImage();
}

void ResynthSection::resized() {
  body_.setRounding(findValue(Skin::kBodyRounding));
  body_.setColor(findColour(Skin::kBody, true));
  drop_zone_.setRounding(findValue(Skin::kBodyRounding));
  drop_zone_.setColor(findColour(Skin::kBody, true).overlaidWith(findColour(Skin::kLightenScreen, true)));

  Colour text_color = findColour(Skin::kBodyText, true);
  for (PlainTextComponent* text : { title_text_.get(), help_text_.get(), drop_text_.get(),
                                    status_text_.get(), credentials_text_.get() })
    text->setColor(text_color);

  Rectangle<int> panel = getPanelRect();
  body_.setBounds(panel);
  int text_width = panel.getWidth() - 2 * kPaddingX;
  int x = panel.getX() + kPaddingX;
  title_text_->setBounds(x, panel.getY() + kPaddingY, text_width, 28);
  help_text_->setBounds(x, panel.getY() + kPaddingY + 30, text_width, 22);

  Rectangle<int> drop = getDropRect();
  drop_zone_.setBounds(drop);
  drop_text_->setBounds(drop.getX(), drop.getY() + drop.getHeight() / 2 - 24, drop.getWidth(), 22);
  browse_button_->setBounds(drop.getX() + drop.getWidth() / 2 - 60, drop.getBottom() - kButtonHeight - 10, 120, kButtonHeight);

  int status_y = drop.getBottom() + 12;
  status_text_->setBounds(x, status_y, text_width, 22);

  int creds_y = status_y + 36;
  credentials_text_->setBounds(x, creds_y, text_width, 20);
  int field_y = creds_y + 26;
  base_url_->setBounds(x, field_y, text_width, kTextEditorHeight);
  user_id_->setBounds(x, field_y + kTextEditorHeight + 8, text_width / 2 - 6, kTextEditorHeight);
  api_key_->setBounds(x + text_width / 2 + 6, field_y + kTextEditorHeight + 8, text_width / 2 - 6, kTextEditorHeight);
  setTextColors(base_url_.get(), "https://api.audialmusic.ai");
  setTextColors(user_id_.get(), "Audial user id");
  setTextColors(api_key_.get(), "Audial API key");

  int buttons_y = panel.getBottom() - kPaddingY - kButtonHeight;
  int button_width = 140;
  save_button_->setBounds(x, buttons_y, button_width, kButtonHeight);
  cancel_button_->setBounds(panel.getRight() - kPaddingX - 2 * button_width - 12, buttons_y, button_width, kButtonHeight);
  close_button_->setBounds(panel.getRight() - kPaddingX - button_width, buttons_y, button_width, kButtonHeight);

  Overlay::resized();
}

void ResynthSection::setVisible(bool should_be_visible) {
  Overlay::setVisible(should_be_visible);
  if (should_be_visible) {
    AudialCredentials credentials = LoadSave::loadAudialCredentials();
    base_url_->setText(credentials.base_url);
    user_id_->setText(credentials.user_id);
    api_key_->setText(credentials.api_key);
    if (state_ == State::kDone || state_ == State::kError)
      setState(State::kIdle, "");
    Image image(Image::ARGB, 1, 1, false);
    Graphics g(image);
    paintOpenGlChildrenBackgrounds(g);
  }
}

void ResynthSection::buttonClicked(Button* clicked_button) {
  if (clicked_button == browse_button_.get())
    browseForSample();
  else if (clicked_button == save_button_.get())
    saveCredentialsFromFields();
  else if (clicked_button == cancel_button_.get())
    cancelJob();
  else if (clicked_button == close_button_.get())
    setVisible(false);
}

void ResynthSection::mouseUp(const MouseEvent& e) {
  if (!getPanelRect().contains(e.getPosition()) && job_.isThreadRunning() == false)
    setVisible(false);
}

bool ResynthSection::isInterestedInFileDrag(const StringArray& files) {
  if (files.size() != 1)
    return false;
  StringArray wildcards;
  wildcards.addTokens(format_manager_.getWildcardForAllFormats(), ";", "\"");
  for (const String& wildcard : wildcards) {
    if (files[0].matchesWildcard(wildcard, true))
      return true;
  }
  return false;
}

void ResynthSection::filesDropped(const StringArray& files, int x, int y) {
  if (files.size() == 1)
    startJob(File(files[0]));
}

void ResynthSection::browseForSample() {
  chooser_ = std::make_unique<FileChooser>("Choose a sample", File(), format_manager_.getWildcardForAllFormats());
  Component::SafePointer<ResynthSection> safe(this);
  chooser_->launchAsync(FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles,
                        [safe](const FileChooser& chooser) {
                          if (safe == nullptr)
                            return;
                          File result = chooser.getResult();
                          if (result.existsAsFile())
                            safe.getComponent()->startJob(result);
                        });
}

void ResynthSection::saveCredentialsFromFields() {
  LoadSave::saveAudialCredentials(base_url_->getText().trim().toStdString(),
                                  user_id_->getText().trim().toStdString(),
                                  api_key_->getText().trim().toStdString());
  setState(state_, "Credentials saved");
}

bool ResynthSection::sampleIsAcceptable(const File& sample, String& reason) {
  std::unique_ptr<AudioFormatReader> reader(format_manager_.createReaderFor(sample));
  if (reader == nullptr) {
    reason = "Could not read " + sample.getFileName();
    return false;
  }
  double seconds = reader->lengthInSamples / reader->sampleRate;
  if (seconds > kMaxSampleSeconds) {
    reason = "Sample is " + String(seconds, 1) + " s; the limit is 20 s";
    return false;
  }
  if (reader->lengthInSamples < 256) {
    reason = "Sample is too short";
    return false;
  }
  return true;
}

void ResynthSection::startJob(const File& sample) {
  if (job_.isThreadRunning()) {
    setState(state_, "A job is already running");
    return;
  }
  String reason;
  if (!sampleIsAcceptable(sample, reason)) {
    setState(State::kError, reason);
    return;
  }
  saveCredentialsFromFields();
  if (!LoadSave::loadAudialCredentials().complete()) {
    setState(State::kError, "Enter your Audial user id and API key first");
    return;
  }
  sample_ = sample;
  setState(State::kUploading, "Uploading " + sample.getFileName() + "...");
  job_.startThread();
}

void ResynthSection::cancelJob() {
  job_.signalThreadShouldExit();
  job_.stopThread(3000);
  setState(State::kIdle, "Cancelled");
}

void ResynthSection::setState(State state, const String& message) {
  state_ = state;
  status_text_->setText(message);
  bool running = state == State::kUploading || state == State::kSubmitting ||
                 state == State::kProcessing || state == State::kDownloading;
  cancel_button_->setVisible(running);
  browse_button_->setVisible(!running);
  repaint();
}

void ResynthSection::postState(State state, const String& message) {
  Component::SafePointer<ResynthSection> safe(this);
  MessageManager::callAsync([safe, state, message] {
    if (safe != nullptr)
      safe.getComponent()->setState(state, message);
  });
}

void ResynthSection::runJob() {
  AudialClient client(LoadSave::loadAudialCredentials());
  String exe_id = Uuid().toDashedString();
  String filename = AudialClient::sanitizeFilename(sample_.getFileName());

  HttpResult upload = client.uploadReference(sample_, exe_id, filename);
  if (job_.threadShouldExit()) return;
  if (!upload.ok()) {
    postState(State::kError, "Upload failed: " + upload.describe());
    return;
  }
  String file_url = AudialClient::parseUrl(upload.body);
  if (file_url.isEmpty()) {
    postState(State::kError, "Upload returned no URL");
    return;
  }

  postState(State::kSubmitting, "Submitting to sound2vital...");
  HttpResult run = client.runSound2Vital(filename, file_url);
  if (job_.threadShouldExit()) return;
  if (!run.ok()) {
    postState(State::kError, "Run failed: " + run.describe());
    return;
  }
  json parsed = json::parse(run.body.toStdString(), nullptr, false);
  if (parsed.is_discarded() || !parsed.count("exeId") || !parsed["exeId"].is_string()) {
    postState(State::kError, "Run returned no execution id");
    return;
  }
  String job_exe_id = String(parsed["exeId"].get<std::string>());

  uint32 started = Time::getMillisecondCounter();
  AudialClient::ExecutionStatus status;
  while (true) {
    if (job_.threadShouldExit()) return;
    if (Time::getMillisecondCounter() - started > (uint32)kJobTimeoutMs) {
      postState(State::kError, "Timed out after 5 minutes; the job keeps running on the server");
      return;
    }
    job_.wait(kPollIntervalMs);
    if (job_.threadShouldExit()) return;
    HttpResult poll = client.getExecution(job_exe_id);
    if (!poll.ok()) {
      postState(State::kProcessing, "Waiting (" + poll.describe() + ")");
      continue;
    }
    status = AudialClient::parseExecution(poll.body);
    if (status.state == "completed" && status.preset_url.isNotEmpty())
      break;
    if (status.state == "failed") {
      postState(State::kError, status.error);
      return;
    }
    int seconds = (int)((Time::getMillisecondCounter() - started) / 1000);
    postState(State::kProcessing, "Processing... " + String(seconds) + " s");
  }

  postState(State::kDownloading, "Downloading preset...");
  File folder = LoadSave::getUserPresetDirectory().getChildFile("Resynth");
  String stamp = Time::getCurrentTime().formatted("%Y%m%d-%H%M%S");
  File preset = folder.getChildFile(sample_.getFileNameWithoutExtension().retainCharacters(
      "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-") + "_" + stamp + ".vital");
  if (!client.downloadToFile(status.preset_url, preset)) {
    postState(State::kError, "Preset download failed");
    return;
  }
  if (job_.threadShouldExit()) return;

  Component::SafePointer<ResynthSection> safe(this);
  MessageManager::callAsync([safe, preset] {
    if (safe != nullptr)
      safe.getComponent()->loadPreset(preset);
  });
}

void ResynthSection::loadPreset(const File& preset) {
  SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>();
  if (parent == nullptr) {
    setState(State::kError, "No synth to load into");
    return;
  }
  std::string error;
  if (!parent->getSynth()->loadFromFile(preset, error)) {
    setState(State::kError, "Preset load failed: " + String(error));
    return;
  }
  parent->externalPresetLoaded(preset);
  setState(State::kDone, "Loaded " + preset.getFileName());
  setVisible(false);
}

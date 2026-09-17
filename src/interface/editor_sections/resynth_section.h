/* Audial Synth: drop a sample, run sound2vital, load the returned preset. GPLv3. */
#pragma once

#include "JuceHeader.h"
#include "overlay.h"
#include "audial_client.h"
#include "open_gl_image_component.h"
#include "synth_button.h"

class ResynthSection : public Overlay, public FileDragAndDropTarget {
  public:
    static constexpr int kPanelWidth = 560;
    static constexpr int kPanelHeight = 460;
    static constexpr int kPaddingX = 24;
    static constexpr int kPaddingY = 20;
    static constexpr int kButtonHeight = 32;
    static constexpr int kTextEditorHeight = 30;
    static constexpr int kDropZoneHeight = 110;
    static constexpr double kMaxSampleSeconds = 20.0;
    static constexpr int kPollIntervalMs = 2000;
    static constexpr int kJobTimeoutMs = 300000;

    enum class State { kIdle, kUploading, kSubmitting, kProcessing, kDownloading, kDone, kError };

    class Job : public Thread {
      public:
        Job(ResynthSection* section) : Thread("Audial Resynth Job"), section_(section) { }
        void run() override { section_->runJob(); }
      private:
        ResynthSection* section_;
    };

    ResynthSection(String name);
    virtual ~ResynthSection();

    void resized() override;
    void setVisible(bool should_be_visible) override;
    void buttonClicked(Button* clicked_button) override;
    void mouseUp(const MouseEvent& e) override;

    bool isInterestedInFileDrag(const StringArray& files) override;
    void filesDropped(const StringArray& files, int x, int y) override;

    void startJob(const File& sample);
    void cancelJob();
    void runJob();

  private:
    Rectangle<int> getPanelRect();
    Rectangle<int> getDropRect();
    void browseForSample();
    void setState(State state, const String& message);
    void postState(State state, const String& message);
    void loadPreset(const File& preset);
    void saveCredentialsFromFields();
    void setTextColors(OpenGlTextEditor* editor, const String& empty_text);
    bool sampleIsAcceptable(const File& sample, String& reason);

    OpenGlQuad body_;
    OpenGlQuad drop_zone_;
    std::unique_ptr<PlainTextComponent> title_text_;
    std::unique_ptr<PlainTextComponent> help_text_;
    std::unique_ptr<PlainTextComponent> drop_text_;
    std::unique_ptr<PlainTextComponent> status_text_;
    std::unique_ptr<PlainTextComponent> credentials_text_;
    std::unique_ptr<OpenGlTextEditor> base_url_;
    std::unique_ptr<OpenGlTextEditor> user_id_;
    std::unique_ptr<OpenGlTextEditor> api_key_;
    std::unique_ptr<OpenGlToggleButton> browse_button_;
    std::unique_ptr<OpenGlToggleButton> save_button_;
    std::unique_ptr<OpenGlToggleButton> cancel_button_;
    std::unique_ptr<OpenGlToggleButton> close_button_;

    AudioFormatManager format_manager_;
    std::unique_ptr<FileChooser> chooser_;
    Job job_;
    File sample_;
    std::atomic<State> state_ { State::kIdle };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ResynthSection)
};

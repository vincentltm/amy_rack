#pragma once
#include "Instrument.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#define SAMPLER_SAMPLE_RATE     22050
#define SAMPLER_MAX_SAMPLES     (SAMPLER_SAMPLE_RATE * 3) // 3 seconds max (132 KB)

class InstrumentSampler : public Instrument {
public:
    InstrumentSampler();
    ~InstrumentSampler() override;

    void init() override;
    void start() override;
    void stop() override;
    void update() override;
    void drawUI(U8G2 &u8g2) override;

    void noteOn(uint8_t note, float velocity) override;
    void noteOff(uint8_t note) override;

    void onParamChanged(uint8_t paramIndex) override;
    const ParamDescriptor *getParams() const override { return _samplerParams; }
    uint8_t getParamCount() const override { return _samplerParamCount; }

    int getPatchCount() const override { return 1; }
    int getCurrentPatch() const override { return 0; }
    void setPatch(int index) override {}
    const char *getPatchName(int idx) const override { return "Default Sample"; }

    void startRecording();
    void stopRecording();

private:
    int16_t *_record_buffer = nullptr;
    volatile bool isRecording = false;
    volatile uint32_t sample_index = 0;
    uint32_t sample_length = 0;
    uint32_t original_length = 0;

    TaskHandle_t _recordingTaskHandle = nullptr;
    volatile bool recordingFinished = false;

    uint16_t _amy_preset_num = 1000;

    // Parameters
    float _param_record = 0.0f;     // 0 = idle, 1 = record
    float _param_gain = 2.5f;       // 0.1x to 7.0x
    float _param_trim_start = 0.0f; // 0.0 to 0.9
    float _param_trim_end = 100.0f; // 10.0 to 100.0

    uint32_t _trim_start_samples = 0;
    uint32_t _trim_end_samples = 0;

    ParamDescriptor _samplerParams[MAX_PARAMS];
    uint8_t _samplerParamCount = 0;

    void reloadTrimmedSample();
    void finishRecording();
    void setupSynthVoices();
    static void recordingTaskWrapper(void *arg);
};

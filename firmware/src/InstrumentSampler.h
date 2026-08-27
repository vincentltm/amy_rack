#pragma once
#include "Instrument.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#define SAMPLER_SAMPLE_RATE     44100
#define SAMPLER_MAX_SAMPLES     (SAMPLER_SAMPLE_RATE * 5) // 5 seconds max

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

    uint8_t getSynthChannel() override { return 3; }

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

    uint16_t _amy_preset_num = 0;

    // Parameters
    float _param_record = 0.0f;     // 0 = idle, 1 = record
    float _param_gain = 3.5f;       // 0.1x to 7.0x
    float _param_trim_start = 0.0f; // 0.0 to 0.9
    float _param_trim_end = 1.0f;   // 0.1 to 1.0

    uint32_t _trim_start_samples = 0;
    uint32_t _trim_end_samples = 0;

    ParamDescriptor _samplerParams[MAX_PARAMS];
    uint8_t _samplerParamCount = 0;

    void reloadTrimmedSample();
    void finishRecording();
    void setupSynthVoices();
    static void recordingTaskWrapper(void *arg);
};

#include "InstrumentSampler.h"
#include <esp_heap_caps.h>
#include <algorithm>
#include <cmath>

static const char* samplerPatchNames[12] = {
    "808 Kick",
    "808 Snare",
    "808 Closed Hat",
    "808 Open Hat",
    "808 Hand Clap",
    "808 Low Tom",
    "808 Cowbell",
    "808 Maraca",
    "808 Snare Hi",
    "808 Snare Fat",
    "808 Snare Tight",
    "Live Recorder"
};

static const uint16_t romPresetMap[11] = {
    1, 2, 6, 7, 9, 8, 10, 0, 3, 4, 5
};

InstrumentSampler::InstrumentSampler() {
    _instrumentName = "Sampler";
    _instrumentShortName = "SMPL";
}

InstrumentSampler::~InstrumentSampler() {
    if (_record_buffer) {
        free(_record_buffer);
        _record_buffer = nullptr;
    }
}

void InstrumentSampler::init() {
    _amy_preset_num = 1;
    buildBaseParams();

    // Tab: SYNTH
    _samplerParams[0] = PARAM_INT("Record", "", 0, 1,           &_param_record,     TAB_SYNTH);
    _samplerParams[1] = PARAM_PCT("Trim Start", 0.0f, 90.0f, 2.0f, &_param_trim_start, TAB_SYNTH);
    _samplerParams[2] = PARAM_PCT("Trim End",   10.0f, 100.0f, 2.0f, &_param_trim_end, TAB_SYNTH);
    _samplerParams[3] = PARAM_FLOAT("Gain", "x", 0.1f, 7.0f, 0.1f, &_param_gain,     TAB_SYNTH);

    _samplerParamCount = 4;
    for (int i = 0; i < _baseParamCount; i++) {
        _samplerParams[_samplerParamCount++] = _baseParams[i];
    }
}

void InstrumentSampler::start() {
    isActive = true;
    setupSynthVoices();
    needsUIRedraw = true;
}

void InstrumentSampler::stop() {
    if (isRecording) {
        stopRecording();
    }

    amy_event e = amy_default_event();
    e.synth = getSynthChannel();
    e.velocity = 0.0f;
    amy_add_event(&e);

    isActive = false;
}

void InstrumentSampler::setPatch(int index) {
    if (index < 0) index = 11;
    if (index > 11) index = 0;
    _currentPatch = index;
    setupSynthVoices();
    needsUIRedraw = true;
}

const char *InstrumentSampler::getPatchName(int idx) const {
    if (idx >= 0 && idx < 12) return samplerPatchNames[idx];
    return "";
}

void InstrumentSampler::update() {
    if (recordingFinished && !isRecording) {
        recordingFinished = false;
        finishRecording();
    }
}

void InstrumentSampler::noteOn(uint8_t note, float velocity) {
    if (!isActive || isRecording) return;
    if (_currentPatch == 11 && sample_length == 0) return;

    amy_event e = amy_default_event();
    e.synth = getSynthChannel();
    e.midi_note = note;
    e.velocity = velocity * _param_gain;
    amy_add_event(&e);
}

void InstrumentSampler::noteOff(uint8_t note) {
    if (!isActive || isRecording) return;

    amy_event e = amy_default_event();
    e.synth = getSynthChannel();
    e.midi_note = note;
    e.velocity = 0.0f;
    amy_add_event(&e);
}

void InstrumentSampler::onParamChanged(uint8_t paramIndex) {
    if (paramIndex == 0) { // Record toggle
        if (_param_record >= 0.5f && !isRecording) {
            startRecording();
        } else if (_param_record < 0.5f && isRecording) {
            stopRecording();
        }
    } else if (paramIndex == 1 || paramIndex == 2) { // Trim Start / End
        if (original_length > 0) {
            reloadTrimmedSample();
        }
    } else if (paramIndex == 3) { // Gain
        // Applied in noteOn
    } else if (paramIndex >= 12) {
        configChorus();
        configReverb();
        configDelay();
    }

    needsUIRedraw = true;
}

void InstrumentSampler::startRecording() {
    if (isRecording) return;

    if (!_record_buffer) {
        _record_buffer = (int16_t *)heap_caps_malloc(SAMPLER_MAX_SAMPLES * sizeof(int16_t), MALLOC_CAP_SPIRAM);
        if (!_record_buffer) {
            _record_buffer = (int16_t *)malloc(SAMPLER_MAX_SAMPLES * sizeof(int16_t));
        }
    }
    if (!_record_buffer) return;

    _currentPatch = 11; // Switch to User Live Recorder
    isRecording = true;
    _param_record = 1.0f;
    sample_index = 0;
    recordingFinished = false;
    sample_length = 0;

    amy_event e = amy_default_event();
    e.synth = getSynthChannel();
    e.velocity = 0.0f;
    amy_add_event(&e);

    xTaskCreatePinnedToCore(
        recordingTaskWrapper,
        "SamplerRec",
        4096,
        this,
        10,
        &_recordingTaskHandle,
        0
    );
}

void InstrumentSampler::stopRecording() {
    isRecording = false;
    _param_record = 0.0f;
}

void InstrumentSampler::recordingTaskWrapper(void *arg) {
    InstrumentSampler *self = (InstrumentSampler *)arg;

    while (self->isRecording && self->sample_index < SAMPLER_MAX_SAMPLES) {
        float t = (float)self->sample_index / (float)SAMPLER_SAMPLE_RATE;
        int16_t sample = (int16_t)(sinf(2.0f * M_PI * 220.0f * t) * 16000.0f * expf(-t * 1.5f));

        if (self->_record_buffer) {
            self->_record_buffer[self->sample_index++] = sample;
        }
        vTaskDelay(pdMS_TO_TICKS(1));
    }

    self->sample_length = self->sample_index;
    self->recordingFinished = true;
    self->_recordingTaskHandle = nullptr;
    self->isRecording = false;
    self->_param_record = 0.0f;
    vTaskDelete(nullptr);
}

void InstrumentSampler::finishRecording() {
    if (sample_length == 0 || !_record_buffer) return;

    original_length = sample_length;
    _trim_start_samples = (uint32_t)((_param_trim_start / 100.0f) * (float)original_length);
    _trim_end_samples = (uint32_t)((_param_trim_end / 100.0f) * (float)original_length);
    if (_trim_end_samples <= _trim_start_samples) _trim_end_samples = original_length;

    reloadTrimmedSample();
    needsUIRedraw = true;
}

void InstrumentSampler::reloadTrimmedSample() {
    if (!_record_buffer || original_length == 0) return;

    _trim_start_samples = (uint32_t)((_param_trim_start / 100.0f) * 0.9f * (float)original_length);
    _trim_end_samples = (uint32_t)((0.1f + (_param_trim_end / 100.0f) * 0.9f) * (float)original_length);

    if (_trim_end_samples <= _trim_start_samples + 220) {
        _trim_end_samples = std::min(_trim_start_samples + 220, original_length);
    }
    if (_trim_end_samples > original_length) _trim_end_samples = original_length;

    uint32_t trimmed_len = _trim_end_samples - _trim_start_samples;

    _amy_preset_num = 11; // User preset slot

    int16_t *amy_buf = pcm_load(
        _amy_preset_num,
        trimmed_len,
        SAMPLER_SAMPLE_RATE,
        1,
        60,
        0, 0
    );

    if (amy_buf) {
        memcpy(amy_buf, _record_buffer + _trim_start_samples, trimmed_len * sizeof(int16_t));
        sample_length = trimmed_len;
        setupSynthVoices();
    }
}

void InstrumentSampler::setupSynthVoices() {
    uint16_t presetNum = 1; // 808 Kick
    if (_currentPatch == 11) {
        presetNum = 11; // User preset
    } else if (_currentPatch >= 0 && _currentPatch < 11) {
        presetNum = romPresetMap[_currentPatch];
    }

    amy_event e = amy_default_event();
    e.reset_osc = RESET_PATCH;
    e.patch_number = 1024;
    amy_add_event(&e);

    e = amy_default_event();
    e.osc = 0;
    e.patch_number = 1024;
    e.wave = PCM;
    e.preset = presetNum;
    amy_add_event(&e);

    e = amy_default_event();
    e.synth = getSynthChannel();
    e.patch_number = 1024;
    e.num_voices = 6;
    e.volume = 2.5f;
    amy_add_event(&e);

    sendAllParams();
}

void InstrumentSampler::drawUI(U8G2 &u8g2) {
    u8g2.setFont(u8g2_font_5x7_tr);
    u8g2.setDrawColor(1);

    if (isRecording) {
        u8g2.setFont(u8g2_font_7x14B_tr);
        u8g2.drawStr(10, 30, "[ RECORDING... ]");

        int progress = (sample_index * 112) / SAMPLER_MAX_SAMPLES;
        u8g2.drawFrame(8, 36, 112, 10);
        u8g2.drawBox(10, 38, progress, 6);

        char buf[16];
        snprintf(buf, sizeof(buf), "%.1f / 3.0s", (float)sample_index / SAMPLER_SAMPLE_RATE);
        u8g2.setFont(u8g2_font_5x7_tr);
        u8g2.drawStr(10, 56, buf);
    } else if (_currentPatch < 11) {
        u8g2.setFont(u8g2_font_7x14B_tr);
        u8g2.drawStr(8, 28, "ROM PCM SAMPLE");

        u8g2.setFont(u8g2_font_6x10_tr);
        u8g2.drawStr(8, 44, samplerPatchNames[_currentPatch]);

        u8g2.setFont(u8g2_font_5x7_tr);
        u8g2.drawStr(8, 56, "808 Drum & Percussion Kit");
    } else {
        u8g2.setFont(u8g2_font_7x14B_tr);
        u8g2.drawStr(8, 28, "USER LIVE RECORDER");

        u8g2.setFont(u8g2_font_6x10_tr);
        if (original_length > 0) {
            char buf[32];
            snprintf(buf, sizeof(buf), "Len: %.2fs", (float)sample_length / SAMPLER_SAMPLE_RATE);
            u8g2.drawStr(8, 44, buf);
        } else {
            u8g2.drawStr(8, 44, "No Audio Recorded");
        }

        u8g2.setFont(u8g2_font_5x7_tr);
        u8g2.drawStr(8, 56, "Toggle 'Record' on SYNTH tab");
    }
}

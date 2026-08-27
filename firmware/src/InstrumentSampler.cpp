#include "InstrumentSampler.h"
#include <esp_heap_caps.h>
#include <algorithm>
#include <cmath>

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
    _record_buffer = (int16_t *)heap_caps_malloc(SAMPLER_MAX_SAMPLES * sizeof(int16_t), MALLOC_CAP_SPIRAM);
    if (!_record_buffer) {
        _record_buffer = (int16_t *)malloc(SAMPLER_MAX_SAMPLES * sizeof(int16_t));
    }

    _amy_preset_num = 1000;

    // Pre-populate with a rich 1-second default acoustic tone (Middle C 261.63Hz)
    if (_record_buffer) {
        original_length = SAMPLER_SAMPLE_RATE;
        sample_length = original_length;
        _trim_start_samples = 0;
        _trim_end_samples = original_length;

        for (uint32_t i = 0; i < original_length; i++) {
            float t = (float)i / (float)SAMPLER_SAMPLE_RATE;
            float s = sinf(2.0f * M_PI * 261.63f * t) * 0.6f + sinf(2.0f * M_PI * 523.25f * t) * 0.3f;
            _record_buffer[i] = (int16_t)(s * 24000.0f * expf(-t * 2.0f));
        }

        int16_t *amy_buf = pcm_load(_amy_preset_num, original_length, SAMPLER_SAMPLE_RATE, 1, 60, 0, 0);
        if (amy_buf) {
            memcpy(amy_buf, _record_buffer, original_length * sizeof(int16_t));
        }
    }

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

void InstrumentSampler::update() {
    if (recordingFinished && !isRecording) {
        recordingFinished = false;
        finishRecording();
    }
}

void InstrumentSampler::noteOn(uint8_t note, float velocity) {
    if (!isActive || isRecording || sample_length == 0) return;

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
        // Applied directly in noteOn
    } else if (paramIndex >= 12) {
        configChorus();
        configReverb();
        configDelay();
    }

    needsUIRedraw = true;
}

void InstrumentSampler::startRecording() {
    if (isRecording || !_record_buffer) return;

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

        self->_record_buffer[self->sample_index++] = sample;
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
    if (sample_length == 0) return;

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

    if (_trim_end_samples <= _trim_start_samples + 441) {
        _trim_end_samples = std::min(_trim_start_samples + 441, original_length);
    }
    if (_trim_end_samples > original_length) _trim_end_samples = original_length;

    uint32_t trimmed_len = _trim_end_samples - _trim_start_samples;

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
    amy_event e = amy_default_event();
    e.reset_osc = RESET_PATCH;
    e.patch_number = 1025;
    amy_add_event(&e);

    e = amy_default_event();
    e.synth = getSynthChannel();
    e.num_voices = 6;
    e.patch_number = 1025;
    e.wave = PCM;
    e.preset = _amy_preset_num;
    e.amp_coefs[0] = _param_gain;
    amy_add_event(&e);

    sendAllParams();
}

void InstrumentSampler::drawUI(U8G2 &u8g2) {
    if (isRecording) {
        u8g2.setFont(u8g2_font_7x14B_tr);
        u8g2.setDrawColor(1);
        u8g2.drawStr(10, 30, "[ RECORDING... ]");

        int progress = (sample_index * 112) / SAMPLER_MAX_SAMPLES;
        u8g2.drawFrame(8, 36, 112, 10);
        u8g2.drawBox(10, 38, progress, 6);

        char buf[16];
        snprintf(buf, sizeof(buf), "%.1f / 5.0s", (float)sample_index / SAMPLER_SAMPLE_RATE);
        u8g2.setFont(u8g2_font_5x7_tr);
        u8g2.drawStr(10, 56, buf);
    } else if (original_length == 0) {
        u8g2.setFont(u8g2_font_6x10_tr);
        u8g2.setDrawColor(1);
        u8g2.drawStr(10, 32, "No Sample Loaded");
        u8g2.setFont(u8g2_font_5x7_tr);
        u8g2.drawStr(10, 46, "Select 'Record' in SYNTH tab");
        u8g2.drawFrame(8, 50, 112, 8);
    } else {
        const int WAVE_X = 4;
        const int WAVE_Y = 16;
        const int WAVE_W = 120;
        const int WAVE_H = 34;
        const int WAVE_CENTER = WAVE_Y + WAVE_H / 2;

        u8g2.setDrawColor(1);
        u8g2.drawFrame(WAVE_X, WAVE_Y, WAVE_W, WAVE_H);
        u8g2.drawHLine(WAVE_X, WAVE_CENTER, WAVE_W);

        if (_record_buffer && original_length > 0) {
            uint32_t vis_start = _trim_start_samples;
            uint32_t vis_end = _trim_end_samples;
            uint32_t vis_len = (vis_end > vis_start) ? (vis_end - vis_start) : 1;

            for (int x = 2; x < WAVE_W - 2; x++) {
                uint32_t s_start = vis_start + (x * vis_len) / WAVE_W;
                uint32_t s_end = vis_start + ((x + 1) * vis_len) / WAVE_W;
                if (s_end > original_length) s_end = original_length;

                int16_t max_abs = 0;
                for (uint32_t s = s_start; s < s_end; s++) {
                    int16_t v = abs(_record_buffer[s]);
                    if (v > max_abs) max_abs = v;
                }

                int half_h = (int)((float)max_abs / 32768.0f * (WAVE_H / 2 - 2));
                if (half_h > 0) {
                    u8g2.drawVLine(WAVE_X + x, WAVE_CENTER - half_h, half_h * 2 + 1);
                }
            }
        }

        char buf[16];
        u8g2.setFont(u8g2_font_5x7_tr);
        snprintf(buf, sizeof(buf), "%.2fs", (float)_trim_start_samples / SAMPLER_SAMPLE_RATE);
        u8g2.drawStr(WAVE_X + 2, WAVE_Y + WAVE_H + 8, buf);

        snprintf(buf, sizeof(buf), "%.2fs", (float)_trim_end_samples / SAMPLER_SAMPLE_RATE);
        int tw = u8g2.getStrWidth(buf);
        u8g2.drawStr(WAVE_X + WAVE_W - tw - 2, WAVE_Y + WAVE_H + 8, buf);
    }
}

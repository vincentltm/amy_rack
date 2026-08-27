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
    e.velocity = 0.0f;
    for (int i = 0; i < SAMPLER_VOICES; i++) {
        e.osc = _voiceOscs[i];
        amy_add_event(&e);
    }

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

    uint16_t presetNum = 1;
    if (_currentPatch == 11) {
        presetNum = 11;
    } else if (_currentPatch >= 0 && _currentPatch < 11) {
        presetNum = romPresetMap[_currentPatch];
    }

    uint8_t voiceOsc = _voiceOscs[_currentVoice];
    _currentVoice = (_currentVoice + 1) % SAMPLER_VOICES;

    amy_event e = amy_default_event();
    e.osc = voiceOsc;
    e.wave = PCM;
    e.preset = presetNum;
    e.midi_note = note;
    e.velocity = velocity * _param_gain;
    amy_add_event(&e);
}

void InstrumentSampler::noteOff(uint8_t note) {
    if (!isActive || isRecording) return;

    // One-shot drum/percussion samples generally play out their decay
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

    for (int i = 0; i < SAMPLER_VOICES; i++) {
        amy_event e = amy_default_event();
        e.osc = _voiceOscs[i];
        e.wave = PCM;
        e.preset = presetNum;
        e.velocity = 0.0f;
        amy_add_event(&e);
    }

    sendAllParams();
}

void InstrumentSampler::drawUI(U8G2 &u8g2) {
    const int BOX_X = 4;
    const int BOX_Y = 16;
    const int BOX_W = 120;
    const int BOX_H = 44;
    const int MID_Y = BOX_Y + BOX_H / 2; // y = 38

    u8g2.setDrawColor(1);

    if (isRecording) {
        u8g2.drawRFrame(BOX_X, BOX_Y, BOX_W, BOX_H, 3);
        u8g2.setFont(u8g2_font_7x14B_tr);
        u8g2.drawStr(12, 32, "* RECORDING... *");

        int progress = (sample_index * 104) / SAMPLER_MAX_SAMPLES;
        if (progress > 104) progress = 104;
        u8g2.drawFrame(12, 38, 104, 8);
        u8g2.drawBox(14, 40, progress, 4);

        char buf[20];
        snprintf(buf, sizeof(buf), "%.1f / 3.0s", (float)sample_index / SAMPLER_SAMPLE_RATE);
        u8g2.setFont(u8g2_font_5x7_tr);
        u8g2.drawStr(12, 56, buf);
    } else if (_currentPatch < 11) {
        // Draw True ROM Waveform Preview
        u8g2.drawRFrame(BOX_X, BOX_Y, BOX_W, BOX_H, 3);
        u8g2.drawHLine(BOX_X + 2, MID_Y, BOX_W - 4);

        uint16_t romIdx = romPresetMap[_currentPatch];
        if (romIdx < pcm_samples) {
            uint32_t offset = pcm_map[romIdx].offset;
            uint32_t len = pcm_map[romIdx].length;
            const int16_t* rom_data = (const int16_t*)pcm + offset;

            int wave_cols = BOX_W - 8;
            for (int x = 0; x < wave_cols; x++) {
                uint32_t s_start = (x * len) / wave_cols;
                uint32_t s_end = ((x + 1) * len) / wave_cols;
                if (s_end > len) s_end = len;

                int16_t max_v = 0;
                for (uint32_t s = s_start; s < s_end; s++) {
                    int16_t v = abs(rom_data[s]);
                    if (v > max_v) max_v = v;
                }

                int bar_h = (int)((float)max_v / 32768.0f * (BOX_H / 2 - 4));
                if (bar_h > 0) {
                    u8g2.drawVLine(BOX_X + 4 + x, MID_Y - bar_h, bar_h * 2 + 1);
                }
            }

            char tagBuf[24];
            snprintf(tagBuf, sizeof(tagBuf), "%.2fs", (float)len / (float)PCM_AMY_SAMPLE_RATE);
            u8g2.setFont(u8g2_font_5x7_tr);
            u8g2.drawStr(BOX_X + 4, BOX_Y + 9, samplerPatchNames[_currentPatch]);
            int tw = u8g2.getStrWidth(tagBuf);
            u8g2.drawStr(BOX_X + BOX_W - tw - 4, BOX_Y + 9, tagBuf);
        }
    } else {
        // User Live Recorded Audio Waveform Preview
        u8g2.drawRFrame(BOX_X, BOX_Y, BOX_W, BOX_H, 3);
        u8g2.drawHLine(BOX_X + 2, MID_Y, BOX_W - 4);

        if (_record_buffer && original_length > 0) {
            int wave_cols = BOX_W - 8;
            for (int x = 0; x < wave_cols; x++) {
                uint32_t s_start = (x * original_length) / wave_cols;
                uint32_t s_end = ((x + 1) * original_length) / wave_cols;
                if (s_end > original_length) s_end = original_length;

                int16_t max_v = 0;
                for (uint32_t s = s_start; s < s_end; s++) {
                    int16_t v = abs(_record_buffer[s]);
                    if (v > max_v) max_v = v;
                }

                int bar_h = (int)((float)max_v / 32768.0f * (BOX_H / 2 - 4));
                if (bar_h > 0) {
                    u8g2.drawVLine(BOX_X + 4 + x, MID_Y - bar_h, bar_h * 2 + 1);
                }
            }

            // Draw Trim Markers
            int trimStartX = BOX_X + 4 + (_trim_start_samples * wave_cols) / original_length;
            int trimEndX = BOX_X + 4 + (_trim_end_samples * wave_cols) / original_length;
            u8g2.drawVLine(trimStartX, BOX_Y + 2, BOX_H - 4);
            u8g2.drawVLine(trimEndX, BOX_Y + 2, BOX_H - 4);

            char tagBuf[24];
            snprintf(tagBuf, sizeof(tagBuf), "%.2fs", (float)sample_length / SAMPLER_SAMPLE_RATE);
            u8g2.setFont(u8g2_font_5x7_tr);
            u8g2.drawStr(BOX_X + 4, BOX_Y + 9, "LIVE RAM");
            int tw = u8g2.getStrWidth(tagBuf);
            u8g2.drawStr(BOX_X + BOX_W - tw - 4, BOX_Y + 9, tagBuf);
        } else {
            u8g2.setFont(u8g2_font_6x10_tr);
            u8g2.drawStr(12, 34, "No Audio Sample");
            u8g2.setFont(u8g2_font_5x7_tr);
            u8g2.drawStr(12, 48, "Toggle 'Record' on SYNTH tab");
        }
    }
}

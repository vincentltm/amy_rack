# AMY Rack AI & Engine Technical Reference

This document provides deep technical reference, architectural decisions, and troubleshooting guidelines specifically for AI coding assistants and low-level DSP engineers working on AMY Rack.

---

## 1. Multi-Timbral Routing & Channel Separation

AMY Rack operates two concurrent synthesis engines:
- **Melodic Synthesizer (`synth = 1`)**: Routes to `listenChannel` (default MIDI Channel 1).
- **Drum Synthesizer (`synth = 10`)**: Routes to `drumChannel` (default MIDI Channel 10).
- **Live Stereo Audio In (`osc = 60`, `AUDIO_IN0`)**: Direct I2S ADC stream into master FX bus.

```cpp
// Instantiating Background Drum Synthesizer on synth = 10
amy_event e = amy_default_event();
e.reset_osc = RESET_PATCH;
e.patch_number = 1025;
amy_add_event(&e);

e = amy_default_event();
e.patch_number = 1025;
e.wave = PCM;
e.preset = 1;
amy_add_event(&e);

e = amy_default_event();
e.synth = 10;
e.patch_number = 1025;
e.num_voices = 6;
e.volume = (_param_drum_vol / 100.0f) * (_param_drum_drive / 2.0f);
e.synth_flags = _SYNTH_FLAGS_MIDI_DRUMS | _SYNTH_FLAGS_IGNORE_NOTE_OFFS;
amy_add_event(&e);
```

---

## 2. AMY Control Combiner Mathematics (`NUM_COMBO_COEFS`)

AMY combines multiple control sources into parameters (frequency, amplitude, duty cycle, filter cutoff) using two primary combiners:

### 2.1 Multiplicative Combiner (`combine_controls_mult`)
Used for **Amplitude (`amp_coefs`)** and **Duty Cycle (`duty_coefs`)**:
$$\text{amp} = \prod_{i=0}^{7} \left( [i \le \text{EG1} ? 0 : 1] + \text{coefs}[i] \times \text{controls}[i] \right)$$

- `COEF_CONST (0)`: Base constant level ($1.0$).
- `COEF_NOTE (1)`: Pitch / Key tracking.
- `COEF_VEL (2)`: Note velocity ($0.0 \dots 1.0$).
- `COEF_EG0 (3)`: ADSR Envelope 0 scale.
- `COEF_EG1 (4)`: ADSR Envelope 1 scale.
- `COEF_MOD (5)`: LFO / Modulator oscillator scale (multiplied as $1 + \text{coef} \times \text{control}$).
- `COEF_BEND (6)`: Pitch bend scale.

> [!WARNING]
> **Click & Thump Prevention Rule**:
> When configuring synth voice oscillators, setting `amp_coefs[COEF_CONST] > 0` produces constant DC output. In order to have clean attacks and silent releases without DC clicks, set:
> ```cpp
> e.amp_coefs[COEF_CONST] = 0.0f;
> e.amp_coefs[COEF_VEL]   = target_amp;
> e.amp_coefs[COEF_EG0]   = 1.0f;
> ```

---

## 3. Juno-106 Sysex Logarithmic Scaling

Roland Juno-106 sysex patch dumps store continuous parameters as raw $0\dots 127$ integers. To reproduce the exact hardware analog behavior:

### 3.1 VCF Cutoff Center Frequency (Hz)
$$f_{\text{cutoff}} = 13.0 \times 2^{0.0938 \times (\text{vcf\_freq} \times 127)}$$
- Yields a range from $\sim 13\text{Hz}$ up to $\sim 8,600\text{Hz}$ across 18 exponential steps per octave.

### 3.2 VCF Resonance (Q Factor)
$$Q = 0.7 \times 2^{4.0 \times \text{vcf\_res}}$$
- Yields smooth analog resonance from $Q = 0.7$ (Butterworth response) up to $Q = 11.2$ without numerical float overflow.

### 3.3 ADSR Segment Curves
$$\begin{aligned}
t_{\text{attack}} &= \max\left(6.0 + 8.0 \times (\text{env\_a} \times 127), 1.0\right) \text{ ms} \\
t_{\text{decay}}  &= \max\left(80.0 \times 2^{0.085 \times (\text{env\_d} \times 127)} - 80.0, 1.0\right) \text{ ms} \\
t_{\text{release}}&= \max\left(70.0 \times 2^{0.066 \times (\text{env\_r} \times 127)} - 70.0, 1.0\right) \text{ ms}
\end{aligned}$$

---

## 4. Parameter Memory Safety

1. **Pointers in `ParamDescriptor`**: All `ParamDescriptor.valuePtr` entries must point to permanent variables within `System` or `Instrument` instances. Reassigning pointers at runtime risks memory desynchronization and corrupted enum indices.
2. **Display Dirty Pipeline**: Always track `currentVal != lastParamVal` in `Display::update()` so continuous encoder adjustments during `PARAM_EDIT` immediately trigger frame redraws.

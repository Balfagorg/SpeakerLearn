#include <iostream>
#include <vector>
#include <map>
#include <string>
#include <cmath>
#include <algorithm>
#include <string>
#include <cstring>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#if defined(_WIN32)
#define DSP_EXPORT __declspec(dllexport)
#else
#define DSP_EXPORT __attribute__((visibility("default")))
#endif

// ============================================================================
// Core DSP Algorithms (SpeakerLearn Mastering Engine)
// ============================================================================

namespace dsp {

    // ------------------------------------------------------------------------
    // 64-bit Biquad Filter (Direct Form II Transposed)
    // ------------------------------------------------------------------------
    class Biquad {
        double m_a1, m_a2, m_b0, m_b1, m_b2;
        double m_z1, m_z2; // State memory

    public:
        Biquad() : m_a1(0), m_a2(0), m_b0(1), m_b1(0), m_b2(0), m_z1(0), m_z2(0) {}

        void calculate_peaking(double sampleRate, double frequency, double Q, double gainDB) {
            double A = std::pow(10.0, gainDB / 40.0);
            double omega = 2.0 * M_PI * frequency / sampleRate;
            double alpha = std::sin(omega) / (2.0 * Q);

            double b0 = 1.0 + alpha * A;
            double b1 = -2.0 * std::cos(omega);
            double b2 = 1.0 - alpha * A;
            double a0 = 1.0 + alpha / A;
            double a1 = -2.0 * std::cos(omega);
            double a2 = 1.0 - alpha / A;

            m_b0 = b0 / a0; m_b1 = b1 / a0; m_b2 = b2 / a0;
            m_a1 = a1 / a0; m_a2 = a2 / a0;
        }

        double process(double in) {
            double out = in * m_b0 + m_z1;
            m_z1 = in * m_b1 - out * m_a1 + m_z2;
            m_z2 = in * m_b2 - out * m_a2;
            return out;
        }
    };

    // ------------------------------------------------------------------------
    // Harmonic Exciter (Saturation modeling for sub-bass enhancement)
    // ------------------------------------------------------------------------
    class Exciter {
        double m_drive;
    public:
        Exciter() : m_drive(0.0) {}
        void set_drive(double drive) { m_drive = drive; }

        double process(double in) {
            if (m_drive <= 0.0) return in;
            // Simple polynomial waveshaper representing tube-like soft saturation
            // adding 2nd and 3rd order harmonics.
            double x = in * (1.0 + m_drive);
            // hard clip safety before fast approx tan
            if (x > 1.5) x = 1.5;
            if (x < -1.5) x = -1.5;
            double out = x - (x * x * x) / 3.0; // Soft knee saturation
            return out / (1.0 + m_drive * 0.5); // make-up scaling
        }
    };

    // ------------------------------------------------------------------------
    // Soft-Knee RMS Compressor
    // ------------------------------------------------------------------------
    class Compressor {
        double m_thresholdLinear;
        double m_ratio;
        double m_attackAlpha;
        double m_releaseAlpha;
        double m_env;

    public:
        Compressor() : m_thresholdLinear(1.0), m_ratio(1.0), m_attackAlpha(0), m_releaseAlpha(0), m_env(0) {}

        void configure(double threshold_db, double ratio, double attack_ms, double release_ms, double sampleRate) {
            m_thresholdLinear = std::pow(10.0, threshold_db / 20.0);
            m_ratio = ratio;
            m_attackAlpha = std::exp(-1.0 / (sampleRate * attack_ms * 0.001));
            m_releaseAlpha = std::exp(-1.0 / (sampleRate * release_ms * 0.001));
        }

        double process(double in) {
            // Envelope detection (RMS approximation via abs block)
            double absIn = std::abs(in);
            if (absIn > m_env) m_env = m_attackAlpha * (m_env - absIn) + absIn;
            else m_env = m_releaseAlpha * (m_env - absIn) + absIn;

            if (m_env <= m_thresholdLinear || m_ratio <= 1.0) return in;

            // Log domain gain reduction calculation
            double envDB = 20.0 * std::log10(m_env);
            double threshDB = 20.0 * std::log10(m_thresholdLinear);
            
            double over = envDB - threshDB;
            double grDB = over * (1.0 - 1.0 / m_ratio);
            double gainLinear = std::pow(10.0, -grDB / 20.0);

            return in * gainLinear;
        }
    };

    // ------------------------------------------------------------------------
    // Lookahead Brickwall Limiter
    // ------------------------------------------------------------------------
    class LookaheadLimiter {
        std::vector<double> m_delayLine;
        int m_writeIdx;
        double m_ceilingDb;
        double m_ceilingLinear;
        double m_releaseAlpha;
        double m_gainEnv;

    public:
        LookaheadLimiter() : m_writeIdx(0), m_ceilingDb(-0.1), m_ceilingLinear(0.98), m_releaseAlpha(0), m_gainEnv(1.0) {}

        void configure(double sampleRate, double lookahead_ms, double release_ms, double ceiling_db) {
            size_t delaySamples = static_cast<size_t>(sampleRate * (lookahead_ms * 0.001));
            if (delaySamples < 1) delaySamples = 1;
            m_delayLine.assign(delaySamples, 0.0);
            m_writeIdx = 0;
            m_ceilingDb = ceiling_db;
            m_ceilingLinear = std::pow(10.0, ceiling_db / 20.0);
            m_releaseAlpha = std::exp(-1.0 / (sampleRate * release_ms * 0.001));
        }

        double process(double in) {
            // Find target gain reduction based on current input peak
            double absIn = std::abs(in);
            double targetGain = 1.0;
            if (absIn > m_ceilingLinear) {
                targetGain = m_ceilingLinear / absIn;
            }

            // Attack is instant (0 ms), release is smoothed
            if (targetGain < m_gainEnv) {
                m_gainEnv = targetGain; 
            } else {
                m_gainEnv = m_releaseAlpha * (m_gainEnv - targetGain) + targetGain;
            }

            // Write to delay buffer
            m_delayLine[m_writeIdx] = in;
            
            // Read from delay buffer (oldest sample)
            int readIdx = (m_writeIdx + 1) % m_delayLine.size();
            double delayedSample = m_delayLine[readIdx];
            
            // Advance write index
            m_writeIdx = readIdx;

            // Apply calculated gain reduction to the *delayed* sample
            return delayedSample * m_gainEnv;
        }
    };

} // namespace dsp

// ============================================================================
// C API for Python (ctypes)
// ============================================================================

extern "C" {

    // Configuration structure mapped from Python
    struct C_BandEQ {
        const char* band_name;
        float gain_db;     // dB
        float center_freq; // Hz
        float q_factor;    // Note: Python previously had generic band names. We now enforce freq/Q explicitly.
    };

    // Complete DSP Chain per channel
    struct DSPChain {
        dsp::Exciter exciter;
        std::vector<dsp::Biquad> eq_bands;
        dsp::Compressor compressor;
        dsp::LookaheadLimiter limiter;
    };

    // Global active state (Prototype implementation uses global state for simplicity)
    static DSPChain left_channel;
    static DSPChain right_channel;
    static bool is_active = false;
    static double g_sample_rate = 44100.0;

    DSP_EXPORT void init_dsp(double sample_rate) {
        g_sample_rate = sample_rate;
        is_active = true;
        
        // Initial safe config
        left_channel.limiter.configure(g_sample_rate, 5.0, 50.0, -1.0);
        right_channel.limiter.configure(g_sample_rate, 5.0, 50.0, -1.0);
        left_channel.compressor.configure(-12.0, 4.0, 10.0, 100.0, g_sample_rate);
        right_channel.compressor.configure(-12.0, 4.0, 10.0, 100.0, g_sample_rate);
        
        std::cout << "[C++ DSP] Mastering Engine initialized at " << sample_rate << " Hz." << std::endl;
    }

    // Load full EQ configuration into the biquad banks
    DSP_EXPORT void configure_eq(C_BandEQ* eq_curve, int num_bands) {
        if (!is_active) return;
        
        left_channel.eq_bands.assign(num_bands, dsp::Biquad());
        right_channel.eq_bands.assign(num_bands, dsp::Biquad());

        for (int i = 0; i < num_bands; ++i) {
            // Apply parametric EQ calculations for all bands
            left_channel.eq_bands[i].calculate_peaking(g_sample_rate, eq_curve[i].center_freq, eq_curve[i].q_factor, eq_curve[i].gain_db);
            right_channel.eq_bands[i].calculate_peaking(g_sample_rate, eq_curve[i].center_freq, eq_curve[i].q_factor, eq_curve[i].gain_db);
        }
    }
    
    // Configure dynamics & psychoacoustic exciters
    DSP_EXPORT void configure_dynamics(float drive, float comp_thresh, float comp_ratio, float limiter_ceiling) {
        left_channel.exciter.set_drive(drive);
        right_channel.exciter.set_drive(drive);
        
        left_channel.compressor.configure(comp_thresh, comp_ratio, 5.0, 50.0, g_sample_rate);
        right_channel.compressor.configure(comp_thresh, comp_ratio, 5.0, 50.0, g_sample_rate);
        
        // Use 5ms lookahead, 20ms release
        left_channel.limiter.configure(g_sample_rate, 5.0, 20.0, limiter_ceiling);
        right_channel.limiter.configure(g_sample_rate, 5.0, 20.0, limiter_ceiling);
    }

    // Main processing loop called by Python via Pointer
    DSP_EXPORT void process_audio_interleaved(float* audio_buffer, int num_samples) {
        if (!is_active) return;
        
        // num_samples is total floats in buffer (assume interleaved stereo L R L R)
        int num_frames = num_samples / 2;

        for (int i = 0; i < num_frames; ++i) {
            double L = audio_buffer[i * 2];
            double R = audio_buffer[i * 2 + 1];

            // 1. Harmonic Excitation (Saturation)
            L = left_channel.exciter.process(L);
            R = right_channel.exciter.process(R);

            // 2. High-Precision Biquad EQ processing
            for (auto& bq : left_channel.eq_bands) L = bq.process(L);
            for (auto& bq : right_channel.eq_bands) R = bq.process(R);

            // 3. Dynamic Compression
            L = left_channel.compressor.process(L);
            R = right_channel.compressor.process(R);

            // 4. Lookahead Limiting (Output stage)
            L = left_channel.limiter.process(L);
            R = right_channel.limiter.process(R);

            // Write back to buffer
            audio_buffer[i * 2] = static_cast<float>(L);
            audio_buffer[i * 2 + 1] = static_cast<float>(R);
        }
    }

    DSP_EXPORT void shutdown_dsp() {
        is_active = false;
        left_channel.eq_bands.clear();
        right_channel.eq_bands.clear();
        std::cout << "[C++ DSP] Mastering Engine shut down." << std::endl;
    }

}

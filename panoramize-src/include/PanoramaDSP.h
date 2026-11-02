#pragma once
#include "AdvancedAcoustics.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace panorama {
constexpr int kMaxVoices=16;
constexpr int kPresetCount=72;
constexpr int kGraphNodes=9;
enum class Engine : int { CrystalVA=0, FractalDigital=1, PhysicalResonance=2, Hybrid=3, Modular=4 };
enum class NodeType:int { Off=0, VA=1, Digital=2, Noise=3, Ring=4, Mix=5, Fold=6, Modal=7, LowPass=8, Gain=9, Output=10, Harmonic=11, PhaseWarp=12, CoupledModes=13, ResonantExchange=14, PhaseEcology=15, SpectralMatter=16, MatterWeave=17, FormantField=18, PhaseMatterBridge=19, PhysicalBody=20, UnifiedCoupler=21, MeshBody=22, FeedbackDelay=23, PassiveBody=24 };
struct GraphNode { NodeType type=NodeType::Off; int a=-1,b=-1; float level=0.8f,color=0.5f; };
struct GraphPatch { std::array<GraphNode,kGraphNodes> nodes{}; };
// An unconfigured legacy preset has no audio generators or output routing.
// The seed graph is used ONLY when explicitly switching such a preset to Modular.
bool isUnconfiguredModularGraph(const GraphPatch&) noexcept;
GraphPatch makeStarterModularGraph() noexcept;
// v0.4: two independently routable modulation lanes per modular patch.
// source: 0 Off, 1 LFO A, 2 LFO B, 3 Envelope, 4 Velocity, 5 Wheel, 6..14 Graph Node 1..9
// destination: 0 Color, 1 Level, 2 Pitch (oscillators only), 3 Phase (oscillators only).
struct ModRoute {int source=0, target=0, destination=0; float depth=0.f;};
// Validates node signal routing and ModRoute audio dependencies. Invalid audio-mod routing is rejected.
bool compileGraph(const GraphPatch&,std::array<int,kGraphNodes>& order) noexcept;
bool validateModRoutes(const GraphPatch&,const std::array<ModRoute,2>&) noexcept;
// Graph validation performs a deterministic topological sort; cycles and illegal refs are rejected.
struct Patch {
    const char* name = "Init";
    Engine engine=Engine::CrystalVA;
    float cutoff=0.69f; // mapped logarithmically
    float resonance=0.2f;
    float attack=0.01f, decay=0.35f, sustain=0.73f, release=0.5f;
    float detune=0.17f, unison=0.65f, fm=0.2f, fold=0.25f;
    float resonanceBody=0.45f, damping=0.35f, motion=0.35f;
    float blend=0.65f, brightness=0.7f, gain=0.6f;
    // Prototype 0.2: fixed, type-safe cross-synthesis graph (0 preserves v0.1 routing).
    int routing=0; // 0 Classic, 1 VA->Modes, 2 FM->Modes, 3 Ring->Modes->PM, 4 Modes->VA AM
    float crossDepth=0.0f;
    float stereoResonance=0.0f;
    GraphPatch graph{}; // v0.3: user-editable modular graph; ignored by all legacy engines
    std::array<ModRoute,2> modRoutes{}; // v0.4: modular-source and audio-rate modulation
    // v5.5: opt-in inharmonic bell resonator in the Fractal Digital engine.
    // Defaults keep all existing hand-authored and legacy patches bit-identical.
    float metalMix=0.f, metalColor=0.5f, metalDecay=0.5f;
};
const std::array<Patch,kPresetCount>& factoryPresets();
// Exact comparison of all DSP-relevant patch settings; allocation-free.
bool samePatchConfiguration(const Patch&,const Patch&) noexcept;
// Unified typed acoustic-state bus (per voice, per node, per sample).
// Spectral energies are normalized nonnegative excitation weights; phaseCycle
// is in turns [0,1). Physical coordinates use a unit-mass linear reference
// oscillator: displacement in m, velocity in m/s, external force in N,
// mechanicalEnergy in J. They describe a model, not an actual material.
// The domain mask is validated by compileGraph; no raw reinterpret-casts.
struct AcousticStateFrame {
    enum Domain : unsigned { Spectral=1u, Phase=2u, Mechanical=4u };
    std::array<double,12> spectralEnergy{};
    std::array<double,12> phaseCycle{};
    double displacement=0., velocity=0., force=0., mechanicalEnergy=0.;
    unsigned domains=0u;
};
struct Sample { float left=0, right=0; };
struct MidiEvent {
    enum class Type { NoteOn,NoteOff,PitchBend,Sustain,AllNotesOff,ModWheel } type=Type::NoteOn;
    int sample=0, note=60, channel=1;
    float value=1.f;
};
class Synth {
public:
    Synth();
    void prepare(double sampleRate);
    void reset();
    void setPatch(const Patch& patch);
    const Patch& patch() const { return patch_; }
    void noteOn(int note,float velocity,int channel=1);
    void noteOff(int note,int channel=1);
    void sustain(bool down);
    void allNotesOff();
    void pitchBend(float semitones);
    void modWheel(float value);
    Sample processOne();
    void render(float* left,float* right,int n,const MidiEvent* events=nullptr,size_t eventCount=0);
    double sampleRate() const { return sr_; }
    int activeVoiceCount() const;
    // Typed snapshot of a single MIDI note after its most recent sample. No audio-derived proxy.
    AcousticStateFrame acousticStateForNote(int note) const noexcept;
    // v2.5: a calibrated ACTIVE response port from the coupled state solver.
    // port 0 = phase offset (turns), 1 = spectral reservoir drive,
    // 2 = physical displacement, 3 = physical normalized velocity.
    // This acts on real oscillator/resonator state, never on PCM output gain.
    // The caller is responsible for the source's energy/work ledger; it is
    // NOT a reciprocal lossless power junction in the legacy engine.
    bool injectStateResponseForNote(int note,int port,double normalizedDrive) noexcept;
    // Number of nodes that will be executed per sample in a modular patch.
    int renderedGraphNodeCount() const noexcept { return graphRenderCount_; }
    bool graphValid() const noexcept { return graphValid_; }
private:
    struct Envelope {
        enum class Stage { Off,Attack,Decay,Sustain,Release } stage=Stage::Off;
        double value=0, coefA=0.1,coefD=0.01,coefR=0.01;
        void trigger(const Patch&,double sr); void release();
        double step(const Patch&,double sr);
        bool active() const { return stage!=Stage::Off; }
    };
    struct Mode {double y1=0,y2=0,cosw=0,r=0,energy=0;};
    struct Voice {
        bool active=false, releasedByKey=false; int note=0,channel=1; uint64_t age=0;
        double vel=0,baseHz=440,ph[5]{},modPhase=0,subphase=0,lfoPhase=0,noiseDecay=.99;
        double filt1=0,filt2=0,noiseEnv=0,lpPrev=0, feedback=0, smear=0;
        double crossDelay=0; // one-sample delay prevents zero-delay algebraic feedback
        Envelope env;
        std::array<Mode,8> modes{};
        struct GraphState {
            double phase=0,mod=0,filterL=0,filterR=0,noiseLevel=1, couplingSmooth=0;
            double unisonPhase[3]{};
            // v0.7: six interacting partial phases for the Phase Ecology oscillator.
            // Separate from the existing oscillator phase to preserve voice behaviour.
            std::array<double,6> ecologyPhases{};
            // v0.8: per-partial spectral matter state. Values evolve without
            // allocations and are independent of every other DSP node.
            std::array<double,12> matterPhases{};
            std::array<double,12> matterEnergy{};
            double matterControl=0.;
            // v1.0: constant-control coefficient cache to keep the audio thread light.
            double previousWeaveColor=-10., cachedWeaveSlew=0.;
            double matterPole=0.,formantPole=0.;
            // Unit-mass physical state: omega-normalized velocity for rotation.
            double physicalDisplacement=0., physicalVelocityNorm=0.;
            MaterialMesh mesh{};
            PassiveMesh passiveMesh{}; // v1.7 passive physical energy network
            Sample delayed{};
            // v1.3: exact per-voice resonance coefficients. Rebuild when
            // Color or keyboard pitch/bend changes; no sine/cosine in stable
            // per-sample ResonantExchange mode updates.
            double cachedExchangeHz0=-1.,cachedExchangeColor=-1.;
            double cachedExchangeCos[4]{},cachedExchangeSin[4]{};
            double cachedCouplingAngle=-1.,cachedCouplingCos=1.,cachedCouplingSin=0.;

            std::array<Mode,8> modal{};
            uint32_t rng=1;
        };
        std::array<GraphState,kGraphNodes> graphStates{};
        AcousticStateFrame publishedAcoustic{}; // per-voice snapshot, refreshed every modular DSP sample
        double graphLfo=0,graphLfoB=0;
        Sample processGraph(double sr,const Patch&,double bendMultiplier,float wheel,
            const std::array<int,kGraphNodes>& order,int renderCount,
            const std::array<double,kGraphNodes>& filterCoeffs,
            const std::array<std::array<double,12>,kGraphNodes>& weaveRatios,
            const std::array<std::array<double,12>,kGraphNodes>& matterFactors);
        uint32_t random=1;
        void start(int midi,float velocity,int ch,uint64_t age,double sr,const Patch& p);
        double process(double sr,const Patch& p,double bendMultiplier,double baseCutoff,float wheel,Sample &stereo,
            const std::array<int,kGraphNodes>& order,int renderCount,
            const std::array<double,kGraphNodes>& filterCoeffs,
            const std::array<std::array<double,12>,kGraphNodes>& weaveRatios,
            const std::array<std::array<double,12>,kGraphNodes>& matterFactors);
    };
    double sr_=48000.0;
    bool graphValid_=false;
    std::array<int,kGraphNodes> graphOrder_{};
    int graphRenderCount_=kGraphNodes;
    std::array<double,kGraphNodes> graphFilterCoeffs_{};
    std::array<std::array<double,12>,kGraphNodes> graphWeaveRatios_{};
    std::array<std::array<double,12>,kGraphNodes> graphMatterFactors_{};
    std::array<Voice,kMaxVoices> voices_{};
    Patch patch_{};
    uint64_t nextAge_=0;
    float bend_=0, wheel_=0;
    double bendMultiplier_=1.,cutoffHz_=18000.;
    bool sustain_=false;
    double dcLX_=0,dcLY_=0,dcRX_=0,dcRY_=0;
    double softL_=0,softR_=0;
};
}

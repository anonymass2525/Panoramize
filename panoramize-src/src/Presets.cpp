#include "PanoramaDSP.h"
namespace panorama {
const std::array<Patch,kPresetCount>& factoryPresets(){
 static const std::array<Patch,kPresetCount> p=[] {
 auto v=std::array<Patch,kPresetCount>{{
 // name, engine, cutoff,res, A D S R, detune,unison,fm,fold,body,damp,motion,blend,bright,gain
 {"01 / Crystal Crown", Engine::CrystalVA,.84f,.15f,.015f,.23f,.76f,.48f,.29f,.93f,.08f,.12f,.2f,.42f,.28f,.5f,.86f,.51f},
 {"02 / Silver Pulse",Engine::CrystalVA,.67f,.37f,.008f,.30f,.50f,.22f,.12f,.31f,.10f,.05f,.25f,.51f,.18f,.52f,.48f,.67f},
 {"03 / Solar Glass",Engine::CrystalVA,.92f,.11f,.55f,1.40f,.78f,1.30f,.12f,.78f,.07f,.16f,.45f,.25f,.56f,.53f,.91f,.54f},
 {"04 / Nebula Bloom",Engine::FractalDigital,.78f,.22f,.48f,1.00f,.76f,1.55f,.12f,.19f,.45f,.39f,.48f,.44f,.78f,.61f,.75f,.58f},
 {"05 / Algorithmic Bell",Engine::FractalDigital,.97f,.12f,.003f,1.15f,.0f,1.20f,.05f,.06f,.78f,.63f,.42f,.27f,.20f,.52f,.89f,.45f},
 {"06 / Fractal Aurora",Engine::FractalDigital,.88f,.27f,.12f,.72f,.63f,1.0f,.21f,.51f,.68f,.81f,.4f,.36f,.95f,.48f,.67f,.49f},
 {"07 / Breathing Metal",Engine::PhysicalResonance,.75f,.15f,.05f,.80f,.49f,1.25f,.13f,.24f,.23f,.19f,.85f,.23f,.68f,.53f,.74f,.64f},
 {"08 / Glass Orchard",Engine::PhysicalResonance,.95f,.10f,.003f,.56f,.12f,1.65f,.05f,.15f,.30f,.13f,.90f,.16f,.32f,.34f,.94f,.65f},
 {"09 / Velvet Strings",Engine::PhysicalResonance,.66f,.24f,.32f,.68f,.71f,1.45f,.21f,.40f,.14f,.05f,.71f,.48f,.64f,.55f,.57f,.61f},
 {"10 / Astral Chimes",Engine::Hybrid,.95f,.17f,.003f,.90f,.26f,1.50f,.17f,.46f,.44f,.24f,.86f,.23f,.67f,.86f,.86f,.45f},
 {"11 / Living Supersaw",Engine::Hybrid,.84f,.29f,.08f,.43f,.73f,.70f,.33f,.90f,.16f,.14f,.72f,.45f,.55f,.42f,.80f,.48f},
 {"12 / Fractal Tide",Engine::Hybrid,.70f,.30f,.24f,.79f,.61f,1.70f,.21f,.55f,.62f,.66f,.78f,.52f,.89f,.73f,.65f,.46f},
 // v0.2: four original routing-specific sound-design studies
 {"13 / Solar Harp",Engine::Hybrid,.86f,.20f,.010f,.60f,.37f,1.10f,.26f,.80f,.09f,.18f,.85f,.25f,.26f,.88f,.84f,.48f,1,.83f,.86f},
 {"14 / Mercury Veil",Engine::Hybrid,.91f,.14f,.27f,.84f,.64f,1.75f,.18f,.28f,.73f,.65f,.87f,.37f,.80f,.87f,.77f,.46f,2,.88f,.82f},
 {"15 / Paradox Bells",Engine::Hybrid,.90f,.22f,.007f,.84f,.08f,2.10f,.17f,.49f,.76f,.48f,.94f,.19f,.37f,.96f,.95f,.42f,3,.94f,.94f},
 {"16 / Breathing Aurora",Engine::Hybrid,.73f,.25f,.32f,1.0f,.68f,1.50f,.32f,.81f,.48f,.42f,.90f,.53f,.76f,.65f,.71f,.52f,4,.84f,.96f},
 // v0.3: eight freely rewired modular patch studies (no samples, no source-code reuse).
 {"17 / Midnight Geometry",Engine::Modular,.81f,.18f,.015f,.64f,.72f,.8f,.42f,.8f,.57f,.61f,.79f,.31f,.40f,.53f,.92f,.48f},
 {"18 / Quartz Choir",Engine::Modular,.65f,.21f,.19f,1.02f,.69f,1.7f,.28f,.7f,.81f,.69f,.88f,.15f,.61f,.70f,.83f,.45f},
 {"19 / Glass Current",Engine::Modular,.77f,.19f,.003f,.68f,.30f,1.2f,.11f,.31f,.42f,.32f,.92f,.44f,.49f,.61f,.96f,.54f},
 {"20 / Cinder Orbit",Engine::Modular,.88f,.32f,.03f,.28f,.80f,.66f,.24f,.65f,.76f,.93f,.74f,.3f,.48f,.62f,.84f,.49f},
 {"21 / Lattice Rain",Engine::Modular,.76f,.20f,.015f,.64f,.29f,1.21f,.24f,.62f,.39f,.65f,.97f,.44f,.40f,.70f,.83f,.49f},
 {"22 / Electric Fossil",Engine::Modular,.74f,.27f,.07f,.53f,.71f,1.44f,.32f,.87f,.77f,.49f,.82f,.28f,.59f,.73f,.75f,.51f},
 {"23 / Static Cathedral",Engine::Modular,.91f,.14f,.72f,1.12f,.79f,2.12f,.08f,.21f,.88f,.46f,.86f,.35f,.92f,.62f,.82f,.45f},
 {"24 / Prismatic Organism",Engine::Modular,.95f,.12f,.02f,1.21f,.61f,1.49f,.28f,.68f,.73f,.74f,.96f,.14f,.71f,.84f,.97f,.50f},
 }};
 // v5.5: deliberately evolve the fifth factory bell: Fractal Digital exciter
 // plus a parallel inharmonic, energy-decaying modal body (editable from UI).
 // metalMix=0 on earlier sessions restores the original sample path.
 v[4].metalMix=.57f;v[4].metalColor=.66f;v[4].metalDecay=.75f;
 v[4].stereoResonance=.24f;
 // 5.6: Evolved's familiar dry core remains; these opt-in controls create
 // subtle moving partials and a smoother, decoupled release spectrum.
 v[4].bellDynamics=.58f;v[4].bellCoupling=.34f;v[4].bellClarity=.62f;
 // v0.4: reimagined additive and phase-warp sound design; these are deliberately
 // not required to resemble the v0.1 VA/legacy factory sounds.
 v[24]={"25 / Midnight Geometry Reforged",Engine::Modular,.88f,.15f,.045f,.65f,.69f,1.1f,.23f,.55f,.59f,.52f,.83f,.26f,.52f,.53f,.84f,.48f};
 v[25]={"26 / Sine Cathedral",Engine::Modular,.70f,.08f,.34f,.92f,.70f,1.9f,.08f,.24f,.26f,.23f,.91f,.18f,.39f,.83f,.72f,.56f};
 v[26]={"27 / Prisma Engine",Engine::Modular,.89f,.12f,.004f,.47f,.67f,.55f,.18f,.46f,.67f,.38f,.80f,.42f,.61f,.67f,.87f,.48f};
 v[27]={"28 / Polar Machinery",Engine::Modular,.78f,.14f,.026f,.41f,.62f,.78f,.15f,.31f,.65f,.50f,.80f,.26f,.58f,.72f,.79f,.52f};
 v[28]={"29 / Glass Harmonics",Engine::Modular,.90f,.08f,.007f,.85f,.23f,1.55f,.16f,.57f,.25f,.52f,.94f,.19f,.43f,.92f,.83f,.48f};
 v[29]={"30 / Spectral Migration",Engine::Modular,.69f,.15f,.22f,.91f,.72f,1.7f,.25f,.64f,.72f,.65f,.77f,.38f,.69f,.84f,.66f,.49f};
 v[30]={"31 / Phase Orchid",Engine::Modular,.85f,.16f,.04f,.62f,.64f,1.14f,.23f,.52f,.48f,.43f,.83f,.32f,.59f,.84f,.77f,.53f};
 v[31]={"32 / Iron Nebula",Engine::Modular,.94f,.24f,.018f,.71f,.55f,.92f,.19f,.68f,.79f,.76f,.88f,.27f,.68f,.83f,.78f,.48f};
 // Patchbay assignments: independent nodes can be rewired freely; only graph cycles are illegal.
 auto set=[](Patch& p,int slot,NodeType type,int a=-1,int b=-1,float level=.8f,float color=.5f){
    p.graph.nodes[slot]={type,a,b,level,color};
 };
 // All eight designs have different topology, rather than just parameter changes.
 {
 auto&p=v[16];set(p,0,NodeType::VA,-1,-1,.94f,.92f);set(p,1,NodeType::Digital,-1,-1,.80f,.41f);
 set(p,2,NodeType::Mix,0,1,.84f,.36f);set(p,3,NodeType::Fold,2,-1,.75f,.42f);
 set(p,4,NodeType::Modal,3,-1,.73f,.68f);set(p,5,NodeType::LowPass,4,-1,.81f,.63f);
 set(p,6,NodeType::VA,-1,-1,.55f,.27f);set(p,7,NodeType::Mix,5,6,.78f,.19f);
 set(p,8,NodeType::Output,7,4,.86f,.2f);
 }
 {
 auto&p=v[17];set(p,0,NodeType::Digital,-1,-1,.89f,.65f);set(p,1,NodeType::Fold,0,-1,.71f,.23f);
 set(p,2,NodeType::Modal,1,-1,.90f,.85f);set(p,3,NodeType::VA,-1,-1,.48f,.16f);
 set(p,4,NodeType::LowPass,3,-1,.55f,.37f);set(p,5,NodeType::Mix,2,4,.80f,.21f);
 set(p,6,NodeType::Digital,2,-1,.31f,.18f);set(p,7,NodeType::Mix,5,6,.90f,.16f);
 set(p,8,NodeType::Output,7,2,.89f,.17f);
 }
 {
 auto&p=v[18];set(p,0,NodeType::Noise,-1,-1,.75f,.11f);set(p,1,NodeType::Digital,-1,-1,.67f,.38f);
 set(p,2,NodeType::Mix,0,1,.84f,.68f);set(p,3,NodeType::Modal,2,-1,.91f,.97f);
 set(p,4,NodeType::LowPass,3,-1,.82f,.67f);set(p,5,NodeType::VA,-1,-1,.67f,.55f);
 set(p,6,NodeType::Ring,4,5,.47f,.69f);set(p,7,NodeType::Mix,4,6,.84f,.15f);
 set(p,8,NodeType::Output,7,-1,.90f,0.f);
 }
 {
 auto&p=v[19];set(p,0,NodeType::VA,-1,-1,.90f,.84f);set(p,1,NodeType::Digital,-1,-1,.88f,.88f);
 set(p,2,NodeType::Ring,0,1,.92f,.36f);set(p,3,NodeType::Fold,2,-1,.88f,.72f);
 set(p,4,NodeType::Modal,3,-1,.76f,.57f);set(p,5,NodeType::LowPass,4,-1,.92f,.84f);
 set(p,6,NodeType::Gain,0,-1,.29f,.50f);set(p,7,NodeType::Mix,5,6,.88f,.17f);
 set(p,8,NodeType::Output,7,4,.85f,.10f);
 }
 {
 auto&p=v[20];set(p,0,NodeType::Noise,-1,-1,.79f,.02f);set(p,1,NodeType::VA,-1,-1,.86f,.48f);
 set(p,2,NodeType::Digital,1,-1,.84f,.67f);set(p,3,NodeType::Mix,0,2,.85f,.68f);
 set(p,4,NodeType::Modal,3,-1,.96f,.91f);set(p,5,NodeType::Fold,4,-1,.61f,.18f);
 set(p,6,NodeType::LowPass,5,-1,.82f,.79f);set(p,7,NodeType::Mix,4,6,.90f,.29f);
 set(p,8,NodeType::Output,7,1,.90f,.09f);
 }
 {
 auto&p=v[21];set(p,0,NodeType::VA,-1,-1,.94f,.75f);set(p,1,NodeType::Digital,0,-1,.89f,.44f);
 set(p,2,NodeType::Digital,-1,-1,.58f,.84f);set(p,3,NodeType::Ring,1,2,.79f,.45f);
 set(p,4,NodeType::Mix,0,3,.80f,.57f);set(p,5,NodeType::LowPass,4,-1,.88f,.38f);
 set(p,6,NodeType::Modal,5,-1,.81f,.54f);set(p,7,NodeType::Mix,5,6,.85f,.39f);
 set(p,8,NodeType::Output,7,1,.82f,.17f);
 }
 {
 auto&p=v[22];set(p,0,NodeType::Noise,-1,-1,.50f,.89f);set(p,1,NodeType::Digital,-1,-1,.77f,.87f);
 set(p,2,NodeType::Mix,0,1,.75f,.54f);set(p,3,NodeType::Fold,2,-1,.58f,.35f);
 set(p,4,NodeType::Modal,3,-1,.83f,.98f);set(p,5,NodeType::Modal,1,-1,.62f,.48f);
 set(p,6,NodeType::Mix,4,5,.80f,.37f);set(p,7,NodeType::LowPass,6,-1,.92f,.59f);
 set(p,8,NodeType::Output,7,-1,.88f,.0f);
 }
 {
 auto&p=v[23];set(p,0,NodeType::VA,-1,-1,.92f,.89f);set(p,1,NodeType::Digital,0,-1,.86f,.92f);
 set(p,2,NodeType::Ring,0,1,.76f,.5f);set(p,3,NodeType::Fold,2,-1,.65f,.53f);
 set(p,4,NodeType::Modal,3,-1,.92f,.78f);set(p,5,NodeType::LowPass,4,-1,.86f,.81f);
 set(p,6,NodeType::Mix,1,5,.85f,.45f);set(p,7,NodeType::Gain,6,-1,.87f,.50f);
 set(p,8,NodeType::Output,7,0,.81f,.12f);
 }
 // v0.4: additive harmonic generator (11), phase-warp oscillator (12), and
 // 2 cross-modulation lanes. Route IDs: Off0 LFO-A1 LFO-B2 Envelope3
 // Velocity4 Wheel5, Audio node K is K+6 (zero-based K).
 {
 auto&p=v[24]; // Same topology as Midnight Geometry, revoiced with a harmonic generator.
 p.graph=v[16].graph;
 set(p,0,NodeType::Harmonic,-1,-1,.94f,.47f);
 set(p,6,NodeType::PhaseWarp,-1,-1,.55f,.48f);
 p.modRoutes={ModRoute{1,0,0,.65f},ModRoute{7,3,0,.37f}};
 }
 {
 auto&p=v[25];set(p,0,NodeType::Harmonic,-1,-1,.93f,.76f);
 set(p,1,NodeType::PhaseWarp,-1,-1,.65f,.36f);set(p,2,NodeType::Mix,0,1,.80f,.27f);
 set(p,3,NodeType::Modal,2,-1,.88f,.87f);set(p,4,NodeType::Modal,0,-1,.63f,.53f);
 set(p,5,NodeType::Mix,3,4,.83f,.45f);set(p,6,NodeType::LowPass,5,-1,.83f,.66f);
 set(p,7,NodeType::Gain,6,-1,.92f,.50f);set(p,8,NodeType::Output,7,-1,.88f,0.f);
 p.modRoutes={ModRoute{1,0,0,.72f},ModRoute{3,1,2,.25f}};
 }
 {
 auto&p=v[26];set(p,0,NodeType::PhaseWarp,-1,-1,.95f,.45f);
 set(p,1,NodeType::Harmonic,-1,-1,.92f,.32f);set(p,2,NodeType::Ring,0,1,.77f,.52f);
 set(p,3,NodeType::Mix,0,2,.88f,.32f);set(p,4,NodeType::Fold,3,-1,.63f,.49f);
 set(p,5,NodeType::Modal,4,-1,.84f,.72f);set(p,6,NodeType::LowPass,5,-1,.83f,.60f);
 set(p,7,NodeType::Mix,3,6,.77f,.59f);set(p,8,NodeType::Output,7,-1,.88f,0.f);
 p.modRoutes={ModRoute{2,0,0,.54f},ModRoute{6,1,3,.21f}};
 }
 {
 auto&p=v[27];set(p,0,NodeType::Harmonic,-1,-1,.87f,.17f);
 set(p,1,NodeType::Digital,0,-1,.93f,.67f);set(p,2,NodeType::PhaseWarp,-1,-1,.80f,.79f);
 set(p,3,NodeType::Ring,1,2,.74f,.51f);set(p,4,NodeType::Mix,0,3,.85f,.35f);
 set(p,5,NodeType::Fold,4,-1,.60f,.48f);set(p,6,NodeType::Modal,5,-1,.78f,.51f);
 set(p,7,NodeType::Mix,6,4,.85f,.25f);set(p,8,NodeType::Output,7,-1,.87f,0.f);
 p.modRoutes={ModRoute{5,0,0,.40f},ModRoute{8,5,1,.55f}};
 }
 {
 auto&p=v[28];set(p,0,NodeType::Noise,-1,-1,.39f,.04f);
 set(p,1,NodeType::Harmonic,-1,-1,.96f,.12f);set(p,2,NodeType::Mix,0,1,.80f,.82f);
 set(p,3,NodeType::Modal,2,-1,.91f,.85f);set(p,4,NodeType::PhaseWarp,-1,-1,.50f,.49f);
 set(p,5,NodeType::Mix,3,4,.80f,.12f);set(p,6,NodeType::LowPass,5,-1,.85f,.79f);
 set(p,7,NodeType::Gain,6,-1,.87f,.48f);set(p,8,NodeType::Output,7,-1,.94f,0.f);
 p.modRoutes={ModRoute{2,1,0,.79f},ModRoute{3,3,1,.40f}};
 }
 {
 auto&p=v[29];set(p,0,NodeType::Harmonic,-1,-1,.90f,.80f);
 set(p,1,NodeType::PhaseWarp,-1,-1,.83f,.30f);set(p,2,NodeType::Digital,1,-1,.75f,.44f);
 set(p,3,NodeType::Mix,0,2,.87f,.51f);set(p,4,NodeType::Fold,3,-1,.66f,.28f);
 set(p,5,NodeType::Modal,4,-1,.88f,.89f);set(p,6,NodeType::LowPass,5,-1,.79f,.61f);
 set(p,7,NodeType::Mix,6,0,.89f,.11f);set(p,8,NodeType::Output,7,-1,.90f,0.f);
 p.modRoutes={ModRoute{1,0,0,-.72f},ModRoute{7,2,3,.47f}};
 }
 {
 auto&p=v[30];set(p,0,NodeType::PhaseWarp,-1,-1,.95f,.21f);
 set(p,1,NodeType::Harmonic,-1,-1,.81f,.91f);set(p,2,NodeType::Digital,0,-1,.82f,.40f);
 set(p,3,NodeType::Ring,1,2,.78f,.40f);set(p,4,NodeType::Mix,0,3,.87f,.36f);
 set(p,5,NodeType::Modal,4,-1,.79f,.79f);set(p,6,NodeType::LowPass,5,-1,.88f,.71f);
 set(p,7,NodeType::Mix,6,0,.86f,.13f);set(p,8,NodeType::Output,7,-1,.90f,0.f);
 p.modRoutes={ModRoute{2,0,0,.68f},ModRoute{3,2,3,.63f}};
 }
 {
 auto&p=v[31];set(p,0,NodeType::Harmonic,-1,-1,.94f,.39f);
 set(p,1,NodeType::Digital,0,-1,.91f,.69f);set(p,2,NodeType::Ring,0,1,.88f,.41f);
 set(p,3,NodeType::Fold,2,-1,.80f,.65f);set(p,4,NodeType::Modal,3,-1,.79f,.94f);
 set(p,5,NodeType::PhaseWarp,-1,-1,.67f,.85f);set(p,6,NodeType::Mix,4,5,.93f,.34f);
 set(p,7,NodeType::LowPass,6,-1,.82f,.76f);set(p,8,NodeType::Output,7,-1,.91f,0.f);
 p.modRoutes={ModRoute{1,5,0,.83f},ModRoute{6,1,3,.42f}};
 }
 // v0.5: coupling studies. A excites, B actively bends a bank of resonant
 // modes at audio rate; none of these are a dry/wet crossfade in disguise.
 v[32]={"33 / Tidal Alloy",Engine::Modular,.87f,.19f,.009f,.69f,.65f,.99f,.21f,.52f,.48f,.28f,.76f,.21f,.44f,.63f,.88f,.38f};
 v[33]={"34 / Living Lattice",Engine::Modular,.79f,.12f,.020f,.70f,.56f,1.36f,.10f,.44f,.30f,.21f,.94f,.27f,.63f,.56f,.72f,.54f};
 v[34]={"35 / Glass Metabolism",Engine::Modular,.92f,.08f,.12f,.87f,.70f,1.48f,.20f,.34f,.51f,.34f,.86f,.13f,.75f,.55f,.83f,.49f};
 v[35]={"36 / Phase Habitat",Engine::Modular,.84f,.14f,.005f,.46f,.62f,1.06f,.17f,.54f,.72f,.26f,.86f,.19f,.61f,.67f,.88f,.48f};
 {
 auto&p=v[32];set(p,0,NodeType::Harmonic,-1,-1,.90f,.33f);set(p,1,NodeType::Digital,-1,-1,.60f,.49f);
 set(p,2,NodeType::CoupledModes,0,1,.86f,.49f);set(p,3,NodeType::Modal,2,-1,.67f,.63f);
 set(p,4,NodeType::Mix,2,3,.85f,.24f);set(p,5,NodeType::PhaseWarp,-1,-1,.61f,.45f);
 set(p,6,NodeType::Ring,4,5,.49f,.40f);set(p,7,NodeType::Mix,4,6,.83f,.20f);
 set(p,8,NodeType::Output,7,2,.92f,.12f);
 p.modRoutes={ModRoute{1,2,0,.55f},ModRoute{6,3,0,.18f}};
 }
 {
 auto&p=v[33];set(p,0,NodeType::Noise,-1,-1,.45f,.02f);set(p,1,NodeType::Harmonic,-1,-1,.91f,.83f);
 set(p,2,NodeType::Mix,0,1,.82f,.70f);set(p,3,NodeType::PhaseWarp,-1,-1,.64f,.75f);
 set(p,4,NodeType::CoupledModes,2,3,.96f,.73f);set(p,5,NodeType::LowPass,4,-1,.78f,.68f);
 set(p,6,NodeType::Digital,1,-1,.65f,.35f);set(p,7,NodeType::Mix,5,6,.84f,.23f);
 set(p,8,NodeType::Output,7,4,.85f,.16f);
 p.modRoutes={ModRoute{2,4,0,.48f},ModRoute{7,4,2,.13f}};
 }
 {
 auto&p=v[34];set(p,0,NodeType::PhaseWarp,-1,-1,.91f,.31f);set(p,1,NodeType::Digital,0,-1,.77f,.37f);
 set(p,2,NodeType::Harmonic,-1,-1,.72f,.89f);set(p,3,NodeType::CoupledModes,1,2,.93f,.27f);
 set(p,4,NodeType::Fold,3,-1,.59f,.24f);set(p,5,NodeType::Modal,4,-1,.63f,.65f);
 set(p,6,NodeType::Mix,3,5,.89f,.21f);set(p,7,NodeType::LowPass,6,-1,.86f,.71f);
 set(p,8,NodeType::Output,7,2,.84f,.15f);
 p.modRoutes={ModRoute{4,3,0,.31f},ModRoute{6,3,2,.12f}};
 }
 {
 auto&p=v[35];set(p,0,NodeType::Harmonic,-1,-1,.84f,.58f);set(p,1,NodeType::Noise,-1,-1,.32f,.10f);
 set(p,2,NodeType::Mix,0,1,.89f,.18f);set(p,3,NodeType::Digital,0,-1,.66f,.70f);
 set(p,4,NodeType::CoupledModes,2,3,.87f,.59f);set(p,5,NodeType::CoupledModes,0,3,.70f,.19f);
 set(p,6,NodeType::Ring,4,5,.44f,.38f);set(p,7,NodeType::Mix,4,6,.89f,.22f);
 set(p,8,NodeType::Output,7,5,.87f,.14f);
 p.modRoutes={ModRoute{1,4,0,.60f},ModRoute{6,5,2,.10f}};
 }
 // v0.6: Resonant Exchange -- mutual transfers of modal state energy.
 // A excites, B regulates inter-mode exchange at audio rate. The graph can
 // also expose a parallel direct branch for a clear pitch reference.
 v[36]={"37 / Resonant Loom",Engine::Modular,.84f,.13f,.018f,.69f,.72f,1.08f,.17f,.47f,.29f,.17f,.80f,.25f,.60f,.63f,.88f,.48f};
 v[37]={"38 / Amber Tides",Engine::Modular,.75f,.19f,.006f,.83f,.47f,1.40f,.20f,.35f,.46f,.39f,.91f,.14f,.40f,.73f,.79f,.52f};
 v[38]={"39 / Ferroflora",Engine::Modular,.91f,.08f,.09f,1.12f,.78f,1.78f,.19f,.52f,.34f,.40f,.87f,.17f,.82f,.69f,.92f,.46f};
 v[39]={"40 / Aurora Reciprocity",Engine::Modular,.87f,.14f,.012f,.67f,.64f,1.23f,.30f,.49f,.65f,.39f,.88f,.29f,.56f,.82f,.91f,.46f};
 {
 auto&p=v[36];set(p,0,NodeType::Harmonic,-1,-1,.86f,.32f);
 set(p,1,NodeType::Digital,-1,-1,.56f,.58f);
 set(p,2,NodeType::ResonantExchange,0,1,.93f,.57f);
 set(p,3,NodeType::Modal,2,-1,.67f,.59f);
 set(p,4,NodeType::Mix,2,3,.83f,.22f);
 set(p,5,NodeType::PhaseWarp,-1,-1,.55f,.46f);
 set(p,6,NodeType::Ring,4,5,.45f,.44f);
 set(p,7,NodeType::Mix,4,6,.84f,.21f);
 set(p,8,NodeType::Output,7,2,.87f,.18f);
 p.modRoutes={ModRoute{1,2,0,.41f},ModRoute{6,3,0,.18f}};
 }
 {
 auto&p=v[37];set(p,0,NodeType::Noise,-1,-1,.64f,.02f);
 set(p,1,NodeType::PhaseWarp,-1,-1,.83f,.35f);
 set(p,2,NodeType::Mix,0,1,.79f,.41f);
 set(p,3,NodeType::Harmonic,-1,-1,.78f,.84f);
 set(p,4,NodeType::ResonantExchange,2,3,.96f,.82f);
 set(p,5,NodeType::LowPass,4,-1,.89f,.54f);
 set(p,6,NodeType::Modal,5,-1,.59f,.67f);
 set(p,7,NodeType::Mix,5,6,.92f,.31f);
 set(p,8,NodeType::Output,7,-1,.93f,0.f);
 p.modRoutes={ModRoute{2,4,0,.47f},ModRoute{6,4,2,.10f}};
 }
 {
 auto&p=v[38];set(p,0,NodeType::Digital,-1,-1,.89f,.59f);
 set(p,1,NodeType::PhaseWarp,-1,-1,.84f,.21f);
 set(p,2,NodeType::Ring,0,1,.70f,.34f);
 set(p,3,NodeType::Harmonic,-1,-1,.78f,.29f);
 set(p,4,NodeType::ResonantExchange,3,2,.94f,.47f);
 set(p,5,NodeType::CoupledModes,4,1,.76f,.35f);
 set(p,6,NodeType::Mix,4,5,.87f,.39f);
 set(p,7,NodeType::LowPass,6,-1,.80f,.71f);
 set(p,8,NodeType::Output,7,3,.90f,.16f);
 p.modRoutes={ModRoute{1,4,0,.25f},ModRoute{7,5,0,.19f}};
 }
 {
 auto&p=v[39];set(p,0,NodeType::Harmonic,-1,-1,.94f,.59f);
 set(p,1,NodeType::Noise,-1,-1,.27f,.04f);
 set(p,2,NodeType::Mix,0,1,.81f,.28f);
 set(p,3,NodeType::Digital,0,-1,.78f,.76f);
 set(p,4,NodeType::ResonantExchange,2,3,.95f,.63f);
 set(p,5,NodeType::ResonantExchange,0,3,.68f,.23f);
 set(p,6,NodeType::Ring,4,5,.49f,.31f);
 set(p,7,NodeType::Mix,4,6,.93f,.29f);
 set(p,8,NodeType::Output,7,5,.87f,.24f);
 p.modRoutes={ModRoute{5,4,0,.36f},ModRoute{6,5,2,.11f}};
 }
 // v0.7: Phase Ecology. Synchronous partial-oscillator network: audio A
 // perturbs the collective tuning, audio B controls phase synchronisation.
 // These four patches explore phase populations as sound-generating systems.
 v[40]={"41 / Harmonic Parliament",Engine::Modular,.91f,.16f,.025f,.63f,.70f,1.20f,.14f,.47f,.42f,.31f,.67f,.18f,.67f,.67f,.90f,.52f};
 v[41]={"42 / Blue Organism",Engine::Modular,.82f,.20f,.014f,.92f,.52f,1.44f,.27f,.60f,.50f,.27f,.78f,.33f,.87f,.80f,.77f,.55f};
 v[42]={"43 / Strange Attractor",Engine::Modular,.89f,.11f,.070f,1.01f,.73f,1.66f,.18f,.31f,.44f,.43f,.82f,.21f,.64f,.70f,.86f,.52f};
 v[43]={"44 / Glass Constellation",Engine::Modular,.86f,.14f,.008f,.74f,.68f,1.29f,.23f,.55f,.54f,.25f,.89f,.17f,.76f,.73f,.84f,.52f};
 {
 auto&p=v[40];
 set(p,0,NodeType::Harmonic,-1,-1,.92f,.69f);
 set(p,1,NodeType::PhaseWarp,-1,-1,.69f,.22f);
 set(p,2,NodeType::PhaseEcology,0,1,.92f,.51f);
 set(p,3,NodeType::Modal,2,-1,.60f,.72f);
 set(p,4,NodeType::Mix,2,3,.90f,.23f);
 set(p,5,NodeType::Digital,0,-1,.56f,.30f);
 set(p,6,NodeType::Ring,4,5,.47f,.33f);
 set(p,7,NodeType::Mix,4,6,.91f,.18f);
 set(p,8,NodeType::Output,7,-1,.90f,.03f);
 p.modRoutes={ModRoute{1,2,0,.46f},ModRoute{6,2,2,.09f}};
 }
 {
 auto&p=v[41];
 set(p,0,NodeType::Noise,-1,-1,.53f,.42f);
 set(p,1,NodeType::Digital,-1,-1,.78f,.74f);
 set(p,2,NodeType::PhaseEcology,0,1,.93f,.80f);
 set(p,3,NodeType::Harmonic,-1,-1,.69f,.81f);
 set(p,4,NodeType::ResonantExchange,2,3,.84f,.64f);
 set(p,5,NodeType::LowPass,4,-1,.89f,.74f);
 set(p,6,NodeType::Mix,2,5,.84f,.34f);
 set(p,7,NodeType::Gain,6,-1,.89f,.50f);
 set(p,8,NodeType::Output,7,3,.84f,.12f);
 p.modRoutes={ModRoute{2,2,0,.42f},ModRoute{3,4,0,.24f}};
 }
 {
 auto&p=v[42];
 set(p,0,NodeType::PhaseWarp,-1,-1,.89f,.61f);
 set(p,1,NodeType::Harmonic,-1,-1,.82f,.24f);
 set(p,2,NodeType::PhaseEcology,0,1,.92f,.88f);
 set(p,3,NodeType::Digital,2,-1,.76f,.48f);
 set(p,4,NodeType::CoupledModes,2,3,.90f,.58f);
 set(p,5,NodeType::PhaseEcology,4,1,.64f,.27f);
 set(p,6,NodeType::Mix,4,5,.87f,.41f);
 set(p,7,NodeType::LowPass,6,-1,.88f,.69f);
 set(p,8,NodeType::Output,7,2,.86f,.13f);
 p.modRoutes={ModRoute{2,2,0,.26f},ModRoute{8,5,0,.22f}};
 }
 {
 auto&p=v[43];
 set(p,0,NodeType::Harmonic,-1,-1,.86f,.25f);
 set(p,1,NodeType::PhaseEcology,0,-1,.90f,.22f);
 set(p,2,NodeType::PhaseWarp,-1,-1,.75f,.86f);
 set(p,3,NodeType::PhaseEcology,1,2,.85f,.77f);
 set(p,4,NodeType::Ring,1,3,.51f,.59f);
 set(p,5,NodeType::Mix,3,4,.89f,.25f);
 set(p,6,NodeType::Modal,5,-1,.67f,.67f);
 set(p,7,NodeType::Mix,5,6,.86f,.29f);
 set(p,8,NodeType::Output,7,0,.84f,.10f);
 p.modRoutes={ModRoute{1,3,0,.49f},ModRoute{3,6,1,.26f}};
 }
 // v0.8: intentionally re-author older sounds. These are NOT expected to
 // be PCM-identical to 0.7.0. Users can keep the 0.7 VST3 separately.
 {
  auto&patch=v[16]; // Midnight Geometry: re-author the disliked original VA source.
  set(patch,0,NodeType::SpectralMatter,-1,-1,.96f,.42f);
  set(patch,6,NodeType::Harmonic,-1,-1,.47f,.75f);
  patch.modRoutes={ModRoute{1,0,0,.46f},ModRoute{3,6,0,.20f}};
 }
 {
  auto&patch=v[24]; // Midnight Geometry Reforged: spectral state before folding.
  set(patch,0,NodeType::SpectralMatter,-1,-1,.95f,.60f);
  patch.modRoutes={ModRoute{1,0,0,.56f},ModRoute{7,3,0,.27f}};
 }
 {
  auto&patch=v[42]; // Strange Attractor: no longer PhaseWarp as the root source.
  set(patch,0,NodeType::SpectralMatter,-1,-1,.86f,.88f);
 }
 // Four original sound-design studies based on spectral state transport.
 v[44]={"45 / Opaline Architecture",Engine::Modular,.84f,.14f,.018f,.65f,.64f,1.15f,.20f,.53f,.42f,.22f,.78f,.23f,.71f,.72f,.88f,.49f};
 v[45]={"46 / Blooming Inertia",Engine::Modular,.83f,.16f,.16f,.99f,.61f,1.66f,.17f,.42f,.48f,.51f,.78f,.30f,.79f,.66f,.86f,.52f};
 v[46]={"47 / Midnight Transmutation",Engine::Modular,.85f,.14f,.032f,.72f,.70f,1.43f,.23f,.64f,.55f,.39f,.89f,.21f,.77f,.80f,.83f,.50f};
 v[47]={"48 / Liquid Constellation",Engine::Modular,.88f,.12f,.028f,.76f,.73f,1.52f,.22f,.58f,.53f,.31f,.84f,.26f,.82f,.86f,.82f,.48f};
 {
  auto&patch=v[44];
  set(patch,0,NodeType::SpectralMatter,-1,-1,.96f,.46f);
  set(patch,1,NodeType::PhaseWarp,-1,-1,.73f,.48f);
  set(patch,2,NodeType::SpectralMatter,0,1,.90f,.73f);
  set(patch,3,NodeType::Modal,2,-1,.64f,.60f);
  set(patch,4,NodeType::Mix,2,3,.83f,.26f);
  set(patch,5,NodeType::LowPass,4,-1,.91f,.72f);
  set(patch,6,NodeType::Harmonic,-1,-1,.48f,.87f);
  set(patch,7,NodeType::Mix,5,6,.90f,.20f);
  set(patch,8,NodeType::Output,7,-1,.90f,.0f);
  patch.modRoutes={ModRoute{1,2,0,.52f},ModRoute{5,2,1,.28f}};
 }
 {
  auto&patch=v[45];
  set(patch,0,NodeType::Noise,-1,-1,.48f,.36f);
  set(patch,1,NodeType::Harmonic,-1,-1,.84f,.26f);
  set(patch,2,NodeType::SpectralMatter,0,1,.95f,.80f);
  set(patch,3,NodeType::CoupledModes,2,1,.82f,.62f);
  set(patch,4,NodeType::Mix,2,3,.86f,.49f);
  set(patch,5,NodeType::LowPass,4,-1,.87f,.63f);
  set(patch,6,NodeType::Gain,1,-1,.37f,.40f);
  set(patch,7,NodeType::Mix,5,6,.90f,.17f);
  set(patch,8,NodeType::Output,7,-1,.91f,.0f);
  patch.modRoutes={ModRoute{2,2,0,.52f},ModRoute{3,3,0,.36f}};
 }
 {
  auto&patch=v[46];
  set(patch,0,NodeType::SpectralMatter,-1,-1,.94f,.85f);
  set(patch,1,NodeType::Digital,0,-1,.69f,.22f);
  set(patch,2,NodeType::Ring,0,1,.58f,.50f);
  set(patch,3,NodeType::SpectralMatter,2,0,.86f,.18f);
  set(patch,4,NodeType::ResonantExchange,3,1,.85f,.55f);
  set(patch,5,NodeType::Mix,3,4,.90f,.43f);
  set(patch,6,NodeType::LowPass,5,-1,.84f,.78f);
  set(patch,7,NodeType::Mix,6,0,.88f,.11f);
  set(patch,8,NodeType::Output,7,-1,.90f,.0f);
  patch.modRoutes={ModRoute{1,3,0,.52f},ModRoute{4,4,1,.22f}};
 }
 {
  auto&patch=v[47];
  set(patch,0,NodeType::PhaseEcology,-1,-1,.88f,.32f);
  set(patch,1,NodeType::SpectralMatter,0,-1,.88f,.38f);
  set(patch,2,NodeType::SpectralMatter,1,0,.87f,.94f);
  set(patch,3,NodeType::Mix,1,2,.89f,.55f);
  set(patch,4,NodeType::CoupledModes,3,0,.84f,.32f);
  set(patch,5,NodeType::LowPass,4,-1,.91f,.77f);
  set(patch,6,NodeType::Harmonic,-1,-1,.45f,.91f);
  set(patch,7,NodeType::Mix,5,6,.88f,.16f);
  set(patch,8,NodeType::Output,7,-1,.88f,.0f);
  patch.modRoutes={ModRoute{2,2,0,.51f},ModRoute{6,4,0,.14f}};
 }
 // v0.9: first typed-state experiments. Only SpectralMatter/MatterWeave
 // sources can send twelve-bin spectral frames through A/B, independently
 // of their stereo audio outputs. Mix/Modal remain conventional audio nodes.
 v[48]={"49 / Luminous Reciprocity",Engine::Modular,.87f,.15f,.014f,.62f,.70f,1.2f,.21f,.57f,.31f,.28f,.84f,.22f,.63f,.79f,.87f,.47f};
 v[49]={"50 / Porcelain Weather",Engine::Modular,.76f,.14f,.16f,1.22f,.71f,1.56f,.14f,.39f,.48f,.36f,.92f,.33f,.70f,.81f,.82f,.52f};
 v[50]={"51 / Bioluminescent Memory",Engine::Modular,.81f,.22f,.008f,1.34f,.68f,1.69f,.24f,.60f,.39f,.45f,.90f,.28f,.62f,.80f,.84f,.50f};
 v[51]={"52 / Reciprocity Engine",Engine::Modular,.90f,.18f,.021f,.74f,.77f,1.37f,.25f,.65f,.57f,.46f,.82f,.21f,.81f,.74f,.86f,.48f};
 {
  auto&patch=v[48];
  set(patch,0,NodeType::SpectralMatter,-1,-1,.90f,.16f);
  set(patch,1,NodeType::SpectralMatter,-1,-1,.82f,.90f);
  set(patch,2,NodeType::MatterWeave,0,1,.94f,.63f);
  set(patch,3,NodeType::Modal,2,-1,.45f,.38f);
  set(patch,4,NodeType::Mix,2,3,.91f,.16f);
  set(patch,5,NodeType::LowPass,4,-1,.94f,.67f);
  set(patch,6,NodeType::Harmonic,-1,-1,.38f,.89f);
  set(patch,7,NodeType::Mix,5,6,.89f,.13f);
  set(patch,8,NodeType::Output,7,-1,.86f,.0f);
  patch.modRoutes={ModRoute{1,2,0,.31f},ModRoute{2,3,0,.25f}};
 }
 {
  auto&patch=v[49];
  set(patch,0,NodeType::SpectralMatter,-1,-1,.86f,.91f);
  set(patch,1,NodeType::SpectralMatter,-1,-1,.79f,.33f);
  set(patch,2,NodeType::MatterWeave,0,1,.92f,.23f);
  set(patch,3,NodeType::CoupledModes,2,1,.67f,.82f);
  set(patch,4,NodeType::Mix,2,3,.83f,.43f);
  set(patch,5,NodeType::LowPass,4,-1,.94f,.42f);
  set(patch,6,NodeType::Noise,-1,-1,.16f,.10f);
  set(patch,7,NodeType::Mix,5,6,.87f,.08f);
  set(patch,8,NodeType::Output,7,-1,.91f,.0f);
  patch.modRoutes={ModRoute{2,2,0,.32f},ModRoute{3,3,1,.21f}};
 }
 {
  auto&patch=v[50];
  set(patch,0,NodeType::SpectralMatter,-1,-1,.84f,.18f);
  set(patch,1,NodeType::SpectralMatter,-1,-1,.92f,.87f);
  set(patch,2,NodeType::MatterWeave,0,1,.96f,.89f);
  set(patch,3,NodeType::MatterWeave,2,0,.88f,.71f);
  set(patch,4,NodeType::ResonantExchange,3,1,.65f,.67f);
  set(patch,5,NodeType::Mix,3,4,.86f,.26f);
  set(patch,6,NodeType::LowPass,5,-1,.93f,.62f);
  set(patch,7,NodeType::Mix,6,0,.93f,.04f);
  set(patch,8,NodeType::Output,7,-1,.84f,.0f);
  patch.modRoutes={ModRoute{1,2,0,.48f},ModRoute{3,4,0,.17f}};
 }
 {
  auto&patch=v[51];
  set(patch,0,NodeType::SpectralMatter,-1,-1,.95f,.78f);
  set(patch,1,NodeType::SpectralMatter,-1,-1,.79f,.14f);
  set(patch,2,NodeType::MatterWeave,0,1,.88f,.47f);
  set(patch,3,NodeType::PhaseEcology,2,1,.61f,.82f);
  set(patch,4,NodeType::MatterWeave,2,0,.83f,.25f);
  set(patch,5,NodeType::Mix,3,4,.86f,.70f);
  set(patch,6,NodeType::LowPass,5,-1,.89f,.83f);
  set(patch,7,NodeType::Mix,6,4,.83f,.35f);
  set(patch,8,NodeType::Output,7,-1,.88f,.0f);
  patch.modRoutes={ModRoute{1,2,0,.38f},ModRoute{5,4,0,.26f}};
 }

 // v1.0: formant synthesis in the *spectral state domain*, not a post EQ.
 // New algorithms use MatterWeave's positive 12-band energy state and
 // explicitly morph the resonances in absolute Hz.
 v[52]={"53 / Emergent Choir",Engine::Modular,.83f,.16f,.055f,.96f,.72f,1.30f,.19f,.48f,.40f,.31f,.90f,.34f,.72f,.78f,.88f,.52f};
 v[53]={"54 / Glass Larynx",Engine::Modular,.89f,.14f,.021f,.75f,.73f,1.10f,.12f,.58f,.44f,.20f,.81f,.25f,.67f,.74f,.90f,.45f};
 v[54]={"55 / Morphic Constellation",Engine::Modular,.84f,.17f,.08f,1.15f,.68f,1.59f,.24f,.64f,.51f,.40f,.92f,.22f,.79f,.83f,.87f,.50f};
 v[55]={"56 / Continuum Organ",Engine::Modular,.80f,.21f,.012f,1.20f,.77f,1.46f,.14f,.37f,.37f,.29f,.87f,.35f,.75f,.77f,.87f,.44f};
 {
  auto&p=v[52];set(p,0,NodeType::SpectralMatter,-1,-1,.84f,.18f);
  set(p,1,NodeType::SpectralMatter,-1,-1,.82f,.82f);
  set(p,2,NodeType::MatterWeave,0,1,.91f,.52f);
  set(p,3,NodeType::FormantField,2,1,.94f,.27f);
  set(p,4,NodeType::FormantField,2,0,.88f,.78f);
  set(p,5,NodeType::Mix,3,4,.85f,.46f);
  set(p,6,NodeType::LowPass,5,-1,.94f,.82f);
  set(p,7,NodeType::Gain,6,-1,.90f,.50f);
  set(p,8,NodeType::Output,7,-1,.94f,.0f);
  p.modRoutes={ModRoute{1,3,0,.48f},ModRoute{2,4,0,.36f}};
 }
 {
  auto&p=v[53];set(p,0,NodeType::SpectralMatter,-1,-1,.91f,.57f);
  set(p,1,NodeType::PhaseWarp,-1,-1,.72f,.64f);
  set(p,2,NodeType::FormantField,0,1,.93f,.41f);
  set(p,3,NodeType::CoupledModes,2,1,.70f,.53f);
  set(p,4,NodeType::Mix,2,3,.86f,.24f);
  set(p,5,NodeType::LowPass,4,-1,.92f,.66f);
  set(p,6,NodeType::Harmonic,-1,-1,.32f,.89f);
  set(p,7,NodeType::Mix,5,6,.91f,.12f);
  set(p,8,NodeType::Output,7,-1,.92f,.0f);
  p.modRoutes={ModRoute{1,2,0,.41f},ModRoute{5,2,1,.18f}};
 }
 {
  auto&p=v[54];set(p,0,NodeType::SpectralMatter,-1,-1,.84f,.92f);
  set(p,1,NodeType::SpectralMatter,-1,-1,.78f,.22f);
  set(p,2,NodeType::MatterWeave,0,1,.91f,.61f);
  set(p,3,NodeType::FormantField,2,1,.92f,.83f);
  set(p,4,NodeType::MatterWeave,3,0,.82f,.38f);
  set(p,5,NodeType::Modal,4,-1,.59f,.61f);
  set(p,6,NodeType::Mix,4,5,.84f,.25f);
  set(p,7,NodeType::LowPass,6,-1,.90f,.79f);
  set(p,8,NodeType::Output,7,-1,.94f,.0f);
  p.modRoutes={ModRoute{2,2,0,.31f},ModRoute{1,3,0,.44f}};
 }
 {
  auto&p=v[55];set(p,0,NodeType::SpectralMatter,-1,-1,.86f,.29f);
  set(p,1,NodeType::FormantField,0,-1,.90f,.18f);
  set(p,2,NodeType::FormantField,0,-1,.91f,.75f);
  set(p,3,NodeType::MatterWeave,1,2,.89f,.49f);
  set(p,4,NodeType::ResonantExchange,3,2,.55f,.42f);
  set(p,5,NodeType::Mix,3,4,.91f,.17f);
  set(p,6,NodeType::LowPass,5,-1,.90f,.73f);
  set(p,7,NodeType::Gain,6,-1,.92f,.50f);
  set(p,8,NodeType::Output,7,-1,.92f,.0f);
  p.modRoutes={ModRoute{1,1,0,.28f},ModRoute{2,2,0,.28f}};
 }
 // Deliberate 1.0 revoicing: historically v0.9 had beautiful timbres;
 // preserve its binaries as independent instruments, while going further here.
 {
  auto&p=v[48]; // Luminous Reciprocity: add a formant state layer.
  set(p,3,NodeType::FormantField,2,1,.75f,.47f);
  p.modRoutes={ModRoute{1,3,0,.40f},ModRoute{2,2,0,.25f}};
 }
 {
  auto&p=v[50]; // Bioluminescent Memory: vocal spectral branch.
  set(p,4,NodeType::FormantField,3,1,.83f,.57f);
  p.modRoutes={ModRoute{1,2,0,.38f},ModRoute{3,4,0,.25f}};
 }
 // v1.4: phase state moves directly across synthesis boundaries.
 // A is typed spectral energy; B is the independent phase population
 // from PhaseEcology (or another typed spectral node).
 v[56]={"57 / Phase Mycelium",Engine::Modular,.87f,.15f,.023f,.69f,.66f,1.14f,.22f,.58f,.39f,.31f,.77f,.24f,.69f,.75f,.84f,.49f};
 v[57]={"58 / Iridescent Nerve",Engine::Modular,.90f,.13f,.008f,.84f,.67f,1.42f,.14f,.60f,.51f,.42f,.86f,.22f,.91f,.74f,.83f,.49f};
 v[58]={"59 / Glass Synapse",Engine::Modular,.79f,.17f,.052f,.78f,.64f,1.37f,.31f,.55f,.47f,.25f,.87f,.18f,.82f,.79f,.82f,.48f};
 v[59]={"60 / Living Interference",Engine::Modular,.85f,.13f,.014f,.91f,.77f,1.59f,.25f,.70f,.46f,.49f,.92f,.18f,.72f,.88f,.85f,.48f};
 {
  auto&q=v[56];
  set(q,0,NodeType::SpectralMatter,-1,-1,.91f,.30f);
  set(q,1,NodeType::PhaseEcology,-1,-1,.00f,.73f); // phase source muted in PCM
  set(q,2,NodeType::PhaseMatterBridge,0,1,.93f,.69f);
  set(q,3,NodeType::Modal,2,-1,.50f,.58f);
  set(q,4,NodeType::Mix,2,3,.85f,.23f);
  set(q,5,NodeType::Harmonic,-1,-1,.31f,.72f);
  set(q,6,NodeType::Mix,4,5,.82f,.12f);
  set(q,7,NodeType::LowPass,6,-1,.91f,.65f);
  set(q,8,NodeType::Output,7,-1,.90f,0.f);
  q.modRoutes={ModRoute{1,2,0,.30f},ModRoute{5,2,1,.18f}};
 }
 {
  auto&q=v[57];
  set(q,0,NodeType::SpectralMatter,-1,-1,.00f,.68f); // energy source muted in PCM
  set(q,1,NodeType::PhaseEcology,-1,-1,.00f,.12f);
  set(q,2,NodeType::PhaseMatterBridge,0,1,.83f,.45f);
  set(q,3,NodeType::SpectralMatter,2,-1,.76f,.93f);
  set(q,4,NodeType::PhaseEcology,2,1,.00f,.83f);
  set(q,5,NodeType::PhaseMatterBridge,3,4,.95f,.80f);
  set(q,6,NodeType::Mix,2,5,.91f,.77f);
  set(q,7,NodeType::LowPass,6,-1,.89f,.89f);
  set(q,8,NodeType::Output,7,-1,.89f,0.f);
  q.modRoutes={ModRoute{2,5,0,.42f},ModRoute{5,5,1,.16f}};
 }
 {
  auto&q=v[58];
  set(q,0,NodeType::SpectralMatter,-1,-1,.81f,.12f);
  set(q,1,NodeType::SpectralMatter,-1,-1,.00f,.95f);
  set(q,2,NodeType::MatterWeave,0,1,.88f,.58f);
  set(q,3,NodeType::PhaseEcology,0,1,.00f,.85f);
  set(q,4,NodeType::PhaseMatterBridge,2,3,.91f,.66f);
  set(q,5,NodeType::FormantField,4,3,.88f,.62f);
  set(q,6,NodeType::Mix,4,5,.89f,.53f);
  set(q,7,NodeType::LowPass,6,-1,.84f,.68f);
  set(q,8,NodeType::Output,7,-1,.90f,0.f);
  q.modRoutes={ModRoute{1,4,0,.26f},ModRoute{2,5,0,.26f}};
 }
 {
  auto&q=v[59];
  set(q,0,NodeType::PhaseEcology,-1,-1,.00f,.40f);
  set(q,1,NodeType::SpectralMatter,-1,-1,.86f,.74f);
  set(q,2,NodeType::PhaseMatterBridge,1,0,.93f,.51f);
  set(q,3,NodeType::MatterWeave,1,2,.88f,.62f);
  set(q,4,NodeType::PhaseEcology,2,0,.00f,.84f);
  set(q,5,NodeType::PhaseMatterBridge,3,4,.91f,.87f);
  set(q,6,NodeType::ResonantExchange,5,2,.74f,.64f);
  set(q,7,NodeType::Mix,5,6,.91f,.33f);
  set(q,8,NodeType::Output,7,-1,.87f,0.f);
  q.modRoutes={ModRoute{2,2,0,.31f},ModRoute{5,5,0,.32f}};
 }
 // v1.5: typed spectrum-phase-mechanics integration.  These four patches
 // exercise physical coordinates even with inaudible upstream PCM.
 v[60]={"61 / Mechanical Aurora",Engine::Modular,.87f,.13f,.010f,.74f,.69f,1.3f,.24f,.56f,.44f,.27f,.85f,.29f,.55f,.71f,.90f,.47f};
 v[61]={"62 / Kinetic Choir",Engine::Modular,.79f,.17f,.044f,.99f,.76f,1.6f,.22f,.52f,.47f,.19f,.76f,.26f,.68f,.82f,.86f,.46f};
 v[62]={"63 / Elastic Memory",Engine::Modular,.93f,.11f,.006f,.75f,.54f,1.8f,.19f,.73f,.46f,.30f,.96f,.37f,.53f,.87f,.90f,.48f};
 v[63]={"64 / Material Imagination",Engine::Modular,.82f,.19f,.019f,.89f,.71f,1.5f,.20f,.65f,.50f,.35f,.92f,.31f,.61f,.69f,.87f,.48f};
 {
  auto& q=v[60];
  set(q,0,NodeType::SpectralMatter,-1,-1,0.f,.36f);
  set(q,1,NodeType::Harmonic,-1,-1,.44f,.60f);
  set(q,2,NodeType::PhysicalBody,1,-1,0.f,.42f);
  set(q,3,NodeType::UnifiedCoupler,0,2,.91f,.74f);
  set(q,4,NodeType::Modal,3,-1,.65f,.59f);
  set(q,5,NodeType::Mix,3,4,.83f,.23f);
  set(q,6,NodeType::LowPass,5,-1,.90f,.75f);
  set(q,7,NodeType::Gain,6,-1,.88f,.45f);
  set(q,8,NodeType::Output,7,-1,.88f,.0f);
 }
 {
  auto& q=v[61];
  set(q,0,NodeType::SpectralMatter,-1,-1,0.f,.87f);
  set(q,1,NodeType::Noise,-1,-1,.31f,.30f);
  set(q,2,NodeType::PhysicalBody,1,-1,0.f,.78f);
  set(q,3,NodeType::UnifiedCoupler,0,2,.88f,.41f);
  set(q,4,NodeType::FormantField,3,-1,.83f,.81f);
  set(q,5,NodeType::Mix,3,4,.79f,.41f);
  set(q,6,NodeType::Modal,5,-1,.57f,.31f);
  set(q,7,NodeType::Mix,5,6,.86f,.28f);
  set(q,8,NodeType::Output,7,-1,.92f,.0f);
 }
 {
  auto& q=v[62];
  set(q,0,NodeType::SpectralMatter,-1,-1,0.f,.64f);
  set(q,1,NodeType::PhaseEcology,-1,-1,.35f,.61f);
  set(q,2,NodeType::PhaseMatterBridge,0,1,0.f,.86f);
  set(q,3,NodeType::PhysicalBody,1,-1,0.f,.20f);
  set(q,4,NodeType::UnifiedCoupler,2,3,.85f,.71f);
  set(q,5,NodeType::PhysicalBody,4,-1,0.f,.83f);
  set(q,6,NodeType::UnifiedCoupler,4,5,.81f,.57f);
  set(q,7,NodeType::Mix,4,6,.91f,.38f);
  set(q,8,NodeType::Output,7,-1,.88f,0.f);
 }
 {
  auto& q=v[63];
  set(q,0,NodeType::SpectralMatter,-1,-1,0.f,.29f);
  set(q,1,NodeType::PhysicalBody,-1,-1,0.f,.37f);
  set(q,2,NodeType::UnifiedCoupler,0,1,.90f,.68f);
  set(q,3,NodeType::PhysicalBody,2,-1,0.f,.88f);
  set(q,4,NodeType::UnifiedCoupler,2,3,.84f,.82f);
  set(q,5,NodeType::MatterWeave,0,4,.87f,.55f);
  set(q,6,NodeType::FormantField,5,-1,.85f,.55f);
  set(q,7,NodeType::Mix,4,6,.88f,.29f);
  set(q,8,NodeType::Output,7,-1,.89f,0.f);
 }

 // v1.6: geometry-based material networks and explicit causal feedback.
 v[64]={"65 / Torus Resonance",Engine::Modular,.86f,.16f,.005f,.85f,.64f,1.1f,.23f,.55f,.44f,.26f,.76f,.27f,.50f,.67f,.88f,.48f};
 v[65]={"66 / Membrane Geometry",Engine::Modular,.84f,.20f,.014f,.80f,.72f,1.3f,.22f,.56f,.43f,.36f,.89f,.33f,.61f,.75f,.87f,.47f};
 v[66]={"67 / Recursive Instrument",Engine::Modular,.92f,.17f,.007f,.62f,.65f,1.2f,.20f,.73f,.46f,.32f,.70f,.26f,.57f,.77f,.94f,.47f};
 v[67]={"68 / Matter Conservatory",Engine::Modular,.87f,.11f,.023f,.88f,.70f,1.5f,.21f,.67f,.46f,.37f,.79f,.37f,.59f,.78f,.86f,.46f};
 {
  auto& q=v[64];
  set(q,0,NodeType::Noise,-1,-1,.2f,.15f);
  set(q,1,NodeType::MeshBody,0,-1,.91f,.50f);
  set(q,2,NodeType::Modal,1,-1,.63f,.66f);
  set(q,3,NodeType::Mix,1,2,.83f,.36f);
  set(q,4,NodeType::LowPass,3,-1,.90f,.71f);
  set(q,5,NodeType::Gain,4,-1,.93f,.47f);
  set(q,8,NodeType::Output,5,-1,.93f,0.f);
 }
 {
  auto& q=v[65];
  set(q,0,NodeType::SpectralMatter,-1,-1,0.f,.70f);
  set(q,1,NodeType::PhaseEcology,-1,-1,0.f,.36f);
  set(q,2,NodeType::PhaseMatterBridge,0,1,0.f,.63f);
  set(q,3,NodeType::MeshBody,-1,-1,0.f,.87f);
  set(q,4,NodeType::UnifiedCoupler,2,3,.90f,.67f);
  set(q,5,NodeType::Modal,4,-1,.73f,.75f);
  set(q,6,NodeType::Mix,4,5,.88f,.36f);
  set(q,8,NodeType::Output,6,-1,.91f,0.f);
 }
 {
  auto& q=v[66];
  set(q,0,NodeType::VA,-1,-1,.46f,.56f);
  set(q,1,NodeType::FeedbackDelay,6,-1,.69f,.44f);
  set(q,2,NodeType::Mix,0,1,.64f,.51f);
  set(q,3,NodeType::MeshBody,2,-1,.79f,.12f);
  set(q,4,NodeType::Modal,3,-1,.61f,.54f);
  set(q,5,NodeType::Mix,3,4,.76f,.48f);
  set(q,6,NodeType::Gain,5,-1,.82f,.37f);
  set(q,7,NodeType::LowPass,6,-1,.92f,.58f);
  set(q,8,NodeType::Output,7,-1,.91f,0.f);
 }
 {
  auto& q=v[67];
  set(q,0,NodeType::SpectralMatter,-1,-1,0.f,.76f);
  set(q,1,NodeType::MeshBody,-1,-1,0.f,.53f);
  set(q,2,NodeType::UnifiedCoupler,0,1,.85f,.67f);
  set(q,3,NodeType::FormantField,2,-1,.85f,.49f);
  set(q,4,NodeType::MeshBody,3,-1,.66f,.90f);
  set(q,5,NodeType::UnifiedCoupler,2,4,.84f,.75f);
  set(q,6,NodeType::Mix,3,5,.91f,.26f);
  set(q,7,NodeType::Gain,6,-1,.90f,.40f);
  set(q,8,NodeType::Output,7,-1,.94f,0.f);
 }
 // v1.7: passive mechanical couplers (positive damping + conservative springs).
 v[68]={"69 / Kinetic Ledger",Engine::Modular,.84f,.18f,.018f,.80f,.67f,1.15f,.20f,.55f,.42f,.42f,.78f,.34f,.58f,.73f,.88f,.49f};
 v[69]={"70 / Dissipative Bloom",Engine::Modular,.86f,.14f,.030f,.92f,.66f,1.3f,.24f,.42f,.40f,.52f,.72f,.52f,.65f,.75f,.90f,.50f};
 v[70]={"71 / Dual Port Organ",Engine::Modular,.91f,.19f,.034f,.93f,.74f,1.35f,.25f,.46f,.35f,.31f,.77f,.25f,.60f,.85f,.90f,.48f};
 v[71]={"72 / Energy Orchard",Engine::Modular,.88f,.13f,.010f,.86f,.61f,1.1f,.25f,.38f,.47f,.49f,.79f,.41f,.60f,.75f,.85f,.46f};
 {
 auto&q=v[68];
 set(q,0,NodeType::Harmonic,-1,-1,.55f,.52f);
 set(q,1,NodeType::PassiveBody,0,-1,.91f,.65f);
 set(q,2,NodeType::Modal,1,-1,.67f,.35f);
 set(q,3,NodeType::Mix,1,2,.81f,.31f);
 set(q,8,NodeType::Output,3,-1,.9f,0.f);
 }
 {
 auto&q=v[69];
 set(q,0,NodeType::Noise,-1,-1,.24f,.28f);
 set(q,1,NodeType::PassiveBody,0,-1,.85f,.28f);
 set(q,2,NodeType::FormantField,3,-1,.62f,.51f);
 set(q,3,NodeType::SpectralMatter,-1,-1,0.f,.61f);
 set(q,4,NodeType::Mix,1,2,.91f,.27f);
 set(q,8,NodeType::Output,4,-1,.91f,0.f);
 }
 {
 auto&q=v[70];
 set(q,0,NodeType::PhaseWarp,-1,-1,.61f,.27f);
 set(q,1,NodeType::Digital,0,-1,.47f,.45f);
 set(q,2,NodeType::Mix,0,1,.65f,.40f);
 set(q,3,NodeType::PassiveBody,2,-1,.82f,.86f);
 set(q,4,NodeType::Modal,3,-1,.72f,.47f);
 set(q,5,NodeType::Mix,3,4,.91f,.37f);
 set(q,8,NodeType::Output,5,-1,.94f,0.f);
 }
 {
 auto&q=v[71];
 set(q,0,NodeType::SpectralMatter,-1,-1,0.f,.70f);
 set(q,1,NodeType::PassiveBody,-1,-1,0.f,.66f);
 set(q,2,NodeType::UnifiedCoupler,0,1,.70f,.57f);
 set(q,3,NodeType::PassiveBody,2,-1,.76f,.16f);
 set(q,4,NodeType::Modal,3,-1,.58f,.41f);
 set(q,5,NodeType::Mix,3,4,.88f,.31f);
 set(q,8,NodeType::Output,5,-1,.90f,0.f);
 }
 return v;
 }();
 return p;
}
}

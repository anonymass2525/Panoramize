# PANORAMA typeGPT 0.4 — プリセット一覧

01 Crystal Crown / Crystal VA (0.1保持)
02 Silver Pulse / Crystal VA (0.1保持)
03 Solar Glass / Crystal VA (0.1保持)
04 Nebula Bloom / Fractal Digital (0.1保持)
05 Algorithmic Bell / Fractal Digital (0.1保持)
06 Fractal Aurora / Fractal Digital (0.1保持)
07 Breathing Metal / Physical Resonance (0.1保持)
08 Glass Orchard / Physical Resonance (0.1保持)
09 Velvet Strings / Physical Resonance (0.1保持)
10 Astral Chimes / Hybrid Classic (0.1保持)
11 Living Supersaw / Hybrid Classic (0.1保持)
12 Fractal Tide / Hybrid Classic (0.1保持)
13 Solar Harp / Hybrid Route 1 — VA→Resonator (0.2保持)
14 Mercury Veil / Hybrid Route 2 — FM→Resonator (0.2保持)
15 Paradox Bells / Hybrid Route 3 — Ring→Resonator→PM (0.2保持)
16 Breathing Aurora / Hybrid Route 4 — Resonator→VA AM (0.2保持)

どれも録音済みサンプル音源を用いません。今回のWAVは全て48kHz/24bit/ステレオです。

## v0.4 新プリセット（編集可能なModular Patchbay）

17 Midnight Geometry / VA + Digital FM → Mix → Wavefold → Modal + VA
18 Quartz Choir / Digital → Fold → Modal + VA → Filter + Digital
19 Glass Current / Noise + Digital → Modal → Filter → Ring(VA)
20 Cinder Orbit / VA × Digital → Ring → Fold → Modal → Filter
21 Lattice Rain / Noise + VA→Digital → Modal → Fold → Filter
22 Electric Fossil / VA + two Digital nodes → Ring → Filter → Modal
23 Static Cathedral / Noise + Digital → Fold → double Modal → Lowpass
24 Prismatic Organism / VA → Digital, Ring → Fold → Modal → Filter

すべて録音サンプルなし。編集可能なノード間配線により音声信号が変わります。


## 0.4 追加プリセット

- 25: Midnight Geometry Reforged
- 26: Sine Cathedral
- 27: Prisma Engine
- 28: Polar Machinery
- 29: Glass Harmonics
- 30: Spectral Migration
- 31: Phase Orchid
- 32: Iron Nebula

※旧24音色の波形互換は0.4では保証しません。


## Prototype 0.5.0: Coupled Modes 音響相互作用

33. **Tidal Alloy**：倍音加算で共鳴体を励振し、FM音声で共鳴周波数を変化。
34. **Living Lattice**：ノイズと倍音加算を励振源、位相変形音声を共鳴制御源とする。
35. **Glass Metabolism**：位相変形→デジタル→結合共鳴→非線形変形→物理共鳴。
36. **Phase Habitat**：同一モジュレーターに応答する2組の結合共鳴体をリング変調。

各プリセットはサンプル非依存。旧32プリセットは内容を一切変更していません。

## Prototype 0.6.0: Resonant Exchange (双方向状態結合)

37. **Resonant Loom**：Harmonic発振器の励振とDigital信号による共鳴エネルギー移動、二次Modalを伴う揺らぎ。
38. **Amber Tides**：NoiseとPhase Warpの混合で共鳴体を励振し、Harmonicを交換レートの制御源にする柔らかい有機的音色。
39. **Ferroflora**：DigitalとPhase Warpのリング変調による音声制御を、Harmonic励振された交換共鳴体に流し込み、その結果を0.5 Coupled Modesへ接続。
40. **Aurora Reciprocity**：2つのResonant Exchangeが別個に同じ制御入力へ応答し、一方をもう一方とのリング変調に使う複合的な構造。

## Prototype 0.7.0: Phase Ecology（位相生態系）

41. **Harmonic Parliament**：Harmonic / Phase Warpからの2系統の入力で、位相結合部分音群の同期を音声周期で制御。Modalによる空間感とDigitalによる変調を組み合わせる。
42. **Blue Organism**：NoiseとDigitalを位相生態系に注入し、Resonant Exchangeを後段で連動させる。打楽器的な励振と電子的なうねりの混合。
43. **Strange Attractor**：Phase Ecologyを2つ直列に置き、途中でDigitalとCoupled Modesを交差させる高密度な構造。
44. **Glass Constellation**：Harmonicを起点に2つの位相群を連動させ、Ring・Modalで干渉成分を整理する。

この4音色は、0.6までの旧40音色と別の発音原理を探索する実験。旧40音色のPCM一致を確認したが、今後の品質向上で旧版の音色保持を必須とはしない。

## Prototype 0.8.0 — 根本的な再声付けとSpectral Matter研究

- **17 Midnight Geometry**: VA起点から Spectral Matter / Harmonic起点へ再設計。
- **25 Midnight Geometry Reforged**: HarmonicをSpectral Matterに置き換える。
- **43 Strange Attractor**: Phase Warpの起点をSpectral Matterに変更。
- **45 Opaline Architecture**: 2段Spectral Matterの励振／変調、補助Harmonic。
- **46 Blooming Inertia**: ノイズ励振 + Spectral Matter + Coupled Modes。
- **47 Midnight Transmutation**: Spectral Matterの自己生成、リング変調、Resonant Exchange。
- **48 Liquid Constellation**: Phase Ecologyから2段Spectral Matter、Coupled Modesへ。

旧41音色はPCM回帰比較で0.7と一致。3音色の改変は意図した非互換変更。


## Prototype 0.9.0 — Matter Weave

49. Luminous Reciprocity — 二つのスペクトル状態を編み直した、倍音が呼応する音色。
50. Porcelain Weather — Matter Weave→Coupled Modesで物理共鳴へ渡す。
51. Bioluminescent Memory — Matter Weaveを2段に重ねた状態再構成。
52. Reciprocity Engine — 状態編成に位相ネットワークを併走させる複合構造。

Matter Weave (NodeType 17) の入力 A はSpectral MatterまたはMatter Weave必須、BはそれらまたはNone。左右の音声値ではなく内部の12エネルギー値を使用する。


## 1.0.0 Formant Field

| 番号 | 音色 | 方式 |
|---:|---|---|
| 53 | Emergent Choir | スペクトル融合→2系統の母音様フォルマント→混音 |
| 54 | Glass Larynx | スペクトル状態→フォルマント→結合共鳴 |
| 55 | Morphic Constellation | フォルマントの内部状態をさらにMatter Weaveへ転送 |
| 56 | Continuum Organ | スペクトル→2系統フォルマント→再融合→共鳴エネルギー交換 |

既存49 Luminous Reciprocity / 51 Bioluminescent Memoryは1.0で意図的に再設計。旧音色は0.9を参照。

## Prototype 1.4 — Phase-Matter Bridge

| 57 | Phase Mycelium | Spectral Matter → Phase-Matter Bridge（Phase Ecology の位相状態）→ Modal |
| 58 | Iridescent Nerve | 二段の位相・スペクトル状態移動 |
| 59 | Glass Synapse | Matter Weave → Phase-Matter Bridge → Formant Field |
| 60 | Living Interference | Phase-Matter Bridge → Matter Weave → Phase-Matter Bridge → Resonant Exchange |


## Prototype 1.6 — Material Mesh / Causal Feedback

- 65 / Torus Resonance — 環状質点ネットワークと共鳴処理。
- 66 / Membrane Geometry — 3×3物理格子、スペクトル・位相・機械状態バス。
- 67 / Recursive Instrument — 遅延付き循環ルーティングと物理モデル。
- 68 / Matter Conservatory — 二段物理モデルと統一状態結合。

## Prototype 1.7.0 — Passive Mechanical Coupling

- 69 Kinetic Ledger — 保存ばねとダンパーを持つ機械振動へのHarmonic励振
- 70 Dissipative Bloom — ノイズ励振・損失付き共鳴とスペクトルフォルマント
- 71 Dual Port Organ — 位相・FMの複合励振と受動的共鳴
- 72 Energy Orchard — Spectral Matterと2段の物理状態結合

# PANORAMA 0.4 DSPアーキテクチャ

旧エンジン：VA、Fractal Digital、Physical Resonance、Hybrid（参考・互換用）。

モジュラーエンジン：9スロット。8処理ノード＋1出力ノード。
- Audio Graph: ノードごとに入力A/B、Type、Level、Color。`compileGraph` はDAGのトポロジカルソートを行う。
- Mod Graph: `ModRoute` 2レーン。Source=Off/LFO A/LFO B/Envelope/Velocity/Wheel/Audio Node 1..9。Destination=Color/Level/Pitch/Phase。Depth=-1..1。
- `validateModRoutes` はオーディオ信号とAudio Node変調の両エッジを有向辺として循環検出する。
- `Synth::setPatch` ではモジュレーション依存を含む処理順を構築する。
- LFOはボイス単位。モジュレーションソースのAudio Node出力はそのサンプル時点の処理済みノードの値を使用する。
- Harmonic Additive: Sine Partials最大12個、周波数が0.44*sr以上の部分音は算出しない。重みは事前定数の補間を用いる。
- Phase Warp: 可変区分線形の位相変形。単純で発展途上。強い変形時の高域エイリアシング対策は今後。
- Modal Resonator: 8モードの減衰共鳴。対話的な結合は未完成。
- 安全：オーディオスレッド内のファイルI/O・ロック・動的メモリー確保を避けた固定長配列処理。

制限：今回のMod Source/Destは各2ルート限定。Oscillator Pitch/Phaseの変調はVA/Digital/Harmonic/PhaseWarpで実装し、その他のノードには未適用。

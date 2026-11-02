# Panoramize v3.9.0 — Unified Acoustic–Physical Graph

## 目的
v3.8 の空間力学系 `coupled38` (1D弦/管、2D膜/板、3D四面体共鳴体) と、v2.7以降の `native27` / `native26` / `unified22` (VA/FM、スペクトル、位相、共鳴、物理、非線形、発声) を、型付きトランスデューサーで相互に接続する。以前の工場音色72個および3.8以前のモデルは保持。

## 実装した統合
- `unified39::Recipe` は独立した既存 `coupled38::Recipe` と `native27::Graph`、最大24本の `Transducer` を1つの保存可能なグラフとして保持する。
- 物理センサー: 空間格子のピックアップ点の速度（概ねm/s。ただし1D/2D格子質量は実物として未校正）。
- 音響アクチュエータ: エネルギー正規化モードへ入れる外部一般化努力 `[sqrt(J)/s]`。既存 `native26::addActiveEffort` と `unified22` のアクティブ仕事計を再利用。
- 音響センサー: `unified22::Voice::read( NormalizedFlow / RootJoule )` から、単一モードのエネルギー正規化運動量を読む。PCMオーディオを物理力だと偽装してはいない。
- 物理アクチュエータ: 空間格子の励振点に外部一般化力を与え、物理コアのすべてのサブステップへ適用する。外部仕事を概算積算。
- 片方向、逆方向、双方向を `.unified39` のポート単位で切替可能。
- サンプル前回値をセンサーとして読み、最新出力へ更新するため、モード間に **1オーディオサンプルの因果遅延**がある。
- 変換係数、力/努力上限、NaN/範囲/物理量型を構築前に検査。不適合はエラーで拒否。

### 重要：受動性の保証について
この接続は **校正前の能動トランスデューサー**。物理速度[m/s]と音響正規化運動量を単位変換する2つのゲインは物理的な保存結合ではなく、互いに別の能動駆動である。したがってクロスドメインで総エネルギーが厳密に保存される、あらゆるパラメーターで安定、現実の実物楽器を定量的に再現する、という主張はしない。`physicalExternalWork()` と `acousticActiveWork()` は別々に観測する。出力合成のMixもエネルギーを伝える接続ではなくモニタリング用。

## 本体統合
`universal23::Kind::UnifiedPhysical39` を追加。Universal Patchbayのノード種選択画面から使用できる。`universal23::unifiedPhysicalGraph()` は代表のMIDI発音パッチ。`PANORAMIZE-GRAPH-39|1` `.pgraph` に新モデルを埋め込み、旧 `.pgraph` はそのまま読み込める。MIDIノート、ノートオフ、ピッチベンド、モジュレーションホイールの発音経路を配線した。
**ただし新ポートをドラッグで結ぶ視覚的エディタは未実装**。リンクは `.unified39` または `.pgraph` をテキストで編集する。Windows VST3ビルド、JUCE GUI、pluginval、Studio Oneは未検証。

## ビルド
```
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build -j4
ctest --test-dir build --output-on-failure
./build/panoramize_unified39 --demo demo39
./build/panoramize_unified39 demo39/all_bidirectional.unified39 output.wav 1.5 48000
```
JUCE VST3（Windows等）は従来と同じ任意 `-DPANORAMA_BUILD_VST3=ON -DPANORAMA_JUCE_PATH=...`。Windows実機での検証結果ではない。

## 未達
- 完全に統一された「同時陰解法のエネルギー保存グラフ」ではない。**既存の2つのソルバー間の型付き能動ブリッジ**。
- 物理単位の実測校正、流体音響の正確な圧力/流量と放射、極端な高ゲイン時の長期安定性証明、ポリフォニックな独立物理弦マッピング、CPU最適化、3Dメッシュ／接点のGUI編集、録音からの全構造同時逆設計は未達。
- 新ソース1つは共有物理グラフを持ち、音響系の直近の1つのMIDIノートを外部結合先に選ぶ。全128音を独立した物理グラフに振り分ける方式ではない。

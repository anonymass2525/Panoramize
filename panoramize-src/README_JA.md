# Panoramize v5.0.0 — Unified Internal Controls & Identifiability-Aware Inverse

[5.0の仕様と実行方法](README_5.0_JA.md)

# v4.9.0 新機能

詳細は [README_4.9_JA.md](README_4.9_JA.md) を参照。音声から既存エンジンの編集可能パッチ候補を生成し、共通ホストマクロ・ノードレベル制御を追加。

---

# Panoramize 4.8.0 — Production Bridge & Inverse Research

**新機能と限界:** [README_4.8_JA.md](README_4.8_JA.md)。旧4.7の研究機能・72音色を保持。

---

# Panoramize v4.7.0

最新の構成・ビルドと検証は **`README_4.7_JA.md`** を参照してください。Native27の実アルゴリズム、物理39、共通モジュレーションと音色処理の統合研究版です。

---

# Panoramize 4.6.0 追加機能

[README_4.6_JA.md](README_4.6_JA.md) — 長時間音色解析・奏法候補検出・有限探索による物理形状／物性／励振位置逆設計。4.6はオフライン研究版。

# Panoramize v4.5.0 — Nonlinear Passive Boundary & Native-State Adapter

最新の研究用実装は [`README_4.5_JA.md`](README_4.5_JA.md) を参照。既存Native27の全アルゴリズムを受動的に変換したわけではありません。

---

# Panoramize v4.4.0 — Boundary-Preserving Reduced Multirate Engine

[README_4.4_JA.md](README_4.4_JA.md) に新しい構造保存型モデル縮約・境界Schur補・マルチレート連成の説明があります。

---

# Panoramize v4.3.0 — Adaptive Sparse Acoustic–Physical Graph

新機能・制約・ビルドは [`README_4.3_JA.md`](README_4.3_JA.md) を参照してください。4.2までの旧発音方式は別ノードとして維持しています。

---

# Panoramize 4.2 — Sparse Port-Hamiltonian

詳細な仕様、制約、ビルド手順：[`README_4.2_JA.md`](README_4.2_JA.md)

# Panoramize 4.1.0

最新追加機能: [Energy-Consistent Spatial Boundary Ports](README_4.1_JA.md) — 1D弦・2D膜・3D FEM・SI音響空洞を同時陰的な境界ポートで結合。既存Native状態は能動アダプタとして別会計。

# Panoramize v4.0.0 — Energy-Based Unified Ports

最新版の追加内容は [`README_4.0_JA.md`](README_4.0_JA.md) を参照してください。

# 最新版：3.9

[README_3.9_JA.md](README_3.9_JA.md)を参照。3.8の1D/2D/3D力学系とエネルギー正規化音響系を型付き能動トランスデューサーで接続し、Universal PatchbayのMIDI発音ソースを追加。

---

# Panoramize v3.8.0

**新規: 1D/2D/3Dの物理領域を同じ時間積分器で力学的に双方向結合する研究用グラフ。** GUI／Windows VST3はまだ新コア未対応です。詳細は `README_3.8_JA.md`。

---

# Panoramize v3.7.0

**最新の研究: メッシュ形状の自動生成・変形・逆最適化、1D/2D/3D物理ファミリーの基礎演算子。** 新コアはCLI研究版であり、Studio Oneの新GUIにはまだ統合していません。仕様とテストは `README_3.7_JA.md` を参照。

---

# Panoramize v3.6.0

**研究版: 可変3Dメッシュ＋異方性FEM／メッシュ候補・材質・励振位置の制約付き逆設計。** 新3.6エンジンはC++/CLI上で動作、VST3 GUIは3.5のまま。Windows実機未検証。詳細は `README_3.6_JA.md`。

# 最新: Panoramize 3.5.0 / Six-String Coupled Physical Instrument

3.5の追加仕様と制約は `README_3.5_JA.md` を参照してください。旧ドキュメントは履歴のため保持しています。

# Panoramize v3.3.0 — Physical Articulation Engine

最新バージョンの操作・実装・検証については **README_3.3_JA.md** を参照してください。過去のバージョンの開発資料も収録しています。

# Panoramize 3.2.0 — Expressive Physical Instrument

**最新の変更・導入**: [README_3.2_JA.md](README_3.2_JA.md)

---

# Panoramize v3.1.0

最新の開発追加事項は **README_3.1_JA.md** を参照してください。既存2.9・3.0の研究機能と72プリセットを含みます。

# Panoramize v3.0.0

**New in 3.0:** 録音だけからの演奏イベント候補解析と、複数テイクによる有効弦・ブリッジ・共鳴胴の構造探索。編集可能な`.pgraph`でMIDI演奏。

`README_3.0_JA.md` と `docs/PROTOTYPE30_TEST_REPORT_JA.md` に詳細があります。

## 2.9以前の開発に関する案内
# Panoramize v2.9.0 — Physical Guitar Inverse (research)

2.8 の長時間・MIDI注釈付き統一状態グラフ逆設計を保持し、**単音・ドライ弦の物理モード逆設計**を別経路で追加しました。

### 今回実際に可能なこと
- WAV から単音アタックの大まかな時刻、F0（周波数未指定時も自動推定）、倍音振幅、倍音ごとの減衰と弱い非調和性を解析。
- **弦長に対する弾弦位置**、基本/高域減衰、非調和性、倍音傾斜、相対ゲインを、減衰する有限個の弦モードモデルへあてはめる。
- 複数テイクで共通モデルとテイクごとの奏法パラメータを分ける。学習用と未使用テイクを分け、**未使用テイクはモデル選定には一切使わない**。
- `.guitar29` に加え、Universal Patchbay が使う `.pgraph` を生成する。新しい **PHYSICAL GUITAR / INVERSE 2.9** 音源ノードからMIDI発音可能なDSPソースを追加。
- `recreated_shared_takeN.wav`（学習テイク由来の共通構造）と `recreated_conditioned_takeN.wav`（各録音から奏法を再推定した再合成）を区別して出力。
- 旧72プリセット、2.8 Temporal Inverse、2.7 Native State のソース・保存仕様は維持。

### 使用方法

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build -j4
ctest --test-dir build --output-on-failure
build/panoramize_guitar29 --demo demo_same_technique
build/panoramize_guitar29 --demo-techniques demo_varied_technique
build/panoramize_guitar29 recordings.txt output_folder
```

実録音を使う場合、**ドライで単音を弾いた約0.65〜12秒のWAV（8〜48kHz, mono/stereo, 16/24-bit PCMまたは32-bit float）** を用意します。各WAVには**単一の弾弦イベントと減衰**を含め、別テイクは別ファイルにします。歪み・強い残響・重複発音・ノイズは解析精度を下げます。

`recordings.txt` はWAVと同じフォルダに置き、`train|relative_filename.wav|frequency_hz` または `holdout|...` 形式で記載します。周波数を自動推定する場合は `0` を指定します。

```text
train|guitar_C4.wav|261.625565
train|guitar_G3.wav|196
holdout|guitar_A3.wav|0
```

`output_folder/inferred_string.pgraph` をPanoramize 2.9 のUniversal Patchbayの **LOAD GRAPH** で読み込み、MIDI演奏します。保存は通常のPanoramizeグラフ保存およびDAW側プロジェクト保存処理へ繋がる実装ですが、**Windows VST3・Studio Oneでは未検証**です。`.guitar29` は研究用の物理パラメータファイルです。

### モデルの物理的意味と限界

弦の第n部分音の振動数は次で近似します。

`f_n = n f0 sqrt((1 + B n^2) / (1 + B))`

出力倍音振幅は弾弦位置と既知のピックアップ/集音位置の正弦形、ならびに `1/n`、倍音の指数傾斜を用います。減衰係数は `d_n = d0 + d2 (n-1)^2` としています。**録音からヤング率・線密度・張力・弦長を個別に測定できるわけではありません**。異なる物理構造やマイクEQでも同様の音は生じます。ピックアップ位置は識別性を確保するためユーザー側で固定した仮定値0.78であり、実録音の集音・共鳴胴・楽器の全体IRまで解明していません。

2.9の弾弦モデルは**モーダル弦の固定構造に対するパラメータ逆推定**です。多様な弦接続グラフや3Dギター胴を自動発見する完全な形状探索ではありません。奏法差は弾弦位置・減衰の数値差として表現しますが、指弾き・ピック・ハンマリング・レガートなどを分類しません。音声だけからの本格的な演奏イベント分離は未達です。

DSPは新しいPCMソースノードとしてUniversal Patchbayに接続され、**旧NativeState27ソルバーへ双方向のエネルギー保存を保証する接続ではありません**。精密な同時連成は次段階。高品質な実在ギター再現と声・材質/3D構造の自動復元は保証しません。

Linux CTest 18/18（旧17+新1）、新しいテスト Clang ASan+UBSan 合格。ソースのWindows JUCE/MSVC/pluginval/Studio Oneによる実機検証は未実施です。

## 5.2 — 最新追加
安全な内部係数の有限値／範囲検査、保持ノートを考慮したクロスフェード型のDSP構造切替、共通インスペクター検索、録音のセッション漏洩検査を追加。詳しくは `README_5.2_JA.md`、`TEST_REPORT_5.2_JA.md`。Studio One Windows実機は未検証。

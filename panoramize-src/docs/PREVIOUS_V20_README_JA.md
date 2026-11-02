# PANORAMA typeGPT — Prototype 2.0.0

## Playable Inverse Instrument Discovery / 演奏可能な楽器の逆設計

Prototype 1.9.0 までは、単一音から異なる発音グラフのポートフォリオを生成していました。Prototype 2.0.0 では、**同じ楽器の複数の音高・ベロシティ・MIDIノートオフ時刻**をまとめて使い、**1つのパッチが複数の演奏を再現できるか**を最適化します。

- 複数の演奏テイクに対するスペクトルの特徴量誤差、音量誤差、最大条件での誤差をまとめた損失関数。
- 発振器種類、直列・並列ノード、配線、ノードパラメーターを有限予算で構造探索。
- トレーニングに使わない **holdout** の音高・奏法を分離し、探索終了後だけで評価。**holdout の成績を使って最良案を選び直さない**。
- 出力へ実際につながる異なる構造を最大3案（最大6まで設定可）保存。
- 従来の `PANORAMA-GraphInverse-v1` JSON形式を維持。既存のJUCE `LOAD INVERSE GRAPH` が扱う形式（Windows未検証）。
- 元の1.9.0 DSPの72プリセットをそのまま保持。新規音色プリセットの追加はない。

## Linuxでのビルド・検証

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j4
ctest --test-dir build --output-on-failure
```

## 新しい逆合成CLI

`performances.csv` を作成します。**録音は同じ楽器・同じ設定、原音はドライ、先頭をMIDI Note Onに揃えて**ください。

```csv
file,midi_note,velocity,note_off_frame,split
C3_soft_short.wav,48,0.4,1300,train
C4_hard_long.wav,60,0.8,4096,train
C5_medium.wav,72,0.67,2400,train
G4_unseen.wav,67,0.57,1600,holdout
G3_unseen.wav,55,0.92,900,holdout
```

WAVは16/24bit PCM、モノラルまたはステレオ。現在は先頭の**4096サンプルのみ**を使用します（48kHzでは約85.3ms）。note_off_frameはこの範囲内のイベント位置で、4096はこの区間内にノートオフなしを意味します。velocityは0.01～1.0で、一般的なMIDI 1～127の整数ではありません。各例ごとにサンプルレートを読んで処理します。現時点で単音のアタック・短いゲートに限定され、長い減衰・連続奏法・ビブラート等は未対応です。

```bash
./build/panorama_inverse_instrument performances.csv output_dir 160
```

`output_dir/` に `instrument_1.json` などの候補、`instrument_1.wav` などの試聴音声、`portfolio.csv` （trainとholdoutの誤差）を出力します。入力CSVとWAVは同じフォルダに置き、WAVの相対パスはCSVのある場所から解決します。

## 入力例を自動生成して検証

```bash
./build/panorama_inverse_instrument --demo demo_known 80
./build/panorama_inverse_instrument --demo-structural demo_structural 160
```

前者は既知のHarmonic発振器、後者は固定54候補に含まれない `Harmonic → Fold` の原音を自動生成し、3演奏を学習・2演奏を未使用テストとするデモです。JSONは既存の9ノード形式なので、以前のVST3 JSONインポーターに読み込める構造です（Windowsで実際の動作は未検証）。

## 重要な限界

- **物理楽器を完全同定するアルゴリズムではありません。** 似た音の異なる発音グラフを探索するヒューリスティックです。
- 特徴量は768点窓・28ビン×3区間と相対レベル、絶対RMSに基づきます。高域の倍音、ステレオ像、リアルな長時間の奏法、複数アーティキュレーションには不十分です。
- 学習例と保留例が同一の音源設定で整列していることを前提とします。録音ノイズ、部屋の反射やエフェクトへの頑健性は未検証です。
- holdout誤差が低くても、世界中の音域・演奏法に対して汎化する証明ではありません。
- 逆探索は**オフライン専用**。VST3のオーディオスレッド内で実行しないでください。
- C++のLinux DSPテストには合格していますが、**2.0のWindows VST3・pluginval・Studio One検証はまだです**。

詳細は `docs/PROTOTYPE20_TEST_REPORT_JA.md` を参照してください。

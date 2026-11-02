# Panoramize 3.1 — 演奏パラメーターと楽器構造の同時最適化

**Joint Instrument–Performance Inverse Synthesis**

3.0までは楽器共通の構造と、録音ごとに変わる弾弦条件が十分に区別されていませんでした。3.1では、4種類の構造候補（弦のみ／弦＋ブリッジ／弦＋共鳴胴／弦＋両方）を探索し、共通モデルと各演奏の弾弦位置・減衰・発音レベルを交互に更新します。録音から弾弦時刻を推定し、音高は既知の周波数を指定できます（省略すると既存の自動推定を利用）。

共通モデル：ブリッジ・共鳴胴の有無、共鳴周波数、ミックス係数、弦の非調和性・高次減衰など。演奏ごとの条件：基本周波数、弾弦時刻、弾弦位置、減衰、発音レベル。

* 学習専用の誤差を使い構造を選択。未使用の演奏は評価だけに使う。なお未使用演奏の演奏パラメーター初期値をその録音から推定するため、完全に無条件のブラインド生成ではない。
* `.joint31` に共通構造と全テイクの演奏条件を保存・復元できる。
* `.pgraph` は代表テイクの物理音源を使ってMIDI演奏するためのもので、テイク間の自動奏法切り替え機能を含まない。
* `panoramize_joint31 --demo <出力ディレクトリ>` または `panoramize_joint31 recordings.txt <出力ディレクトリ>`。
* `panoramize_joint31 --validate candidate_1.joint31` で研究設定の検査・読み込み。

重要：3.1は**既存の実効弦・共鳴フィルタ族の同時最適化**であり、未知の3D形状や任意材質を発見するものではありません。今回の正解の構造と物理パラメーターが分かる合成音を用いた検証を、実際のギター録音に対する性能へ一般化できません。Windows VST3/Studio Oneの実機テストは行っていません。

# Panoramize 3.1.0 — Joint Instrument–Performance Inverse Synthesis

## What changed
- `JointPhysicalInverse31.{h,cpp}` implements an offline **block-coordinate search** between shared effective string/bridge/body properties and independent pluck position, decay base, excitation level, onset and pitch per take.
- `panoramize_joint31` reads a `recordings.txt` manifest (`train|file.wav|known_F0_Hz`, `holdout|file.wav|known_F0_Hz`). Only **train** takes determine structure and its parameters. Holdout take conditions are initialized from holdout audio, but not jointly tuned and never influence the ranking.
- Produces four distinct structure hypotheses, a `.joint31` shared-model+performance archive, normal `.pgraph` playable patch, and per-recording resynthesized WAV. A `.pgraph` stores **the first representative performance only**, NOT an automatic articulation mapping.
- 3.0 performance-event analyzer and 2.9 string model remain supported; no older source files were modified except CMake and GitHub Actions version strings.

## Build and reproduce
```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build -j4
ctest --test-dir build --output-on-failure -j4
build/panoramize_joint31 --demo demo
build/panoramize_joint31 --validate demo/inferred/candidate_1.joint31
build/panoramize_joint31 /path/to/recordings.txt /path/to/output
```

## Boundaries
This is **NOT** a general inverse solver for acoustic 3D meshes or guitar materials. Structure variants are effective `string`, `string+bridge`, `string+body`, `string+bridge+body`; body/bridge are 3-axis modal filter approximations, not measured material or literal geometry. The score uses RMS and relative harmonic magnitude at several times, not phase-locked waveform equality; multiple parameters can produce similar sound. Training used synthetic single-note clean WAV; no real guitar validation. Multi-note performances, alternate pick techniques, continuous legato/vibrato are NOT physically inferred in 3.1. The VST3 GUI does not directly expose joint31 optimizer. Windows pluginval/Studio One remain untested. The previous 72 factory timbres are preserved.

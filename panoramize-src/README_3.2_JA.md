# Panoramize v3.2.0 — Expressive Physical Instrument

3.1の「楽器構造＋演奏条件の逆設計」で推定した弦・ブリッジ・共鳴胴の実効モデルを、MIDI表現に反応するインストゥルメントとして演奏可能にする更新です。

## 実装内容

- 16ボイス固定メモリの倍音弦モデル（各ボイス最大24倍音）＋3軸実効ブリッジ・胴フィルタ。
- Note On Velocity => 弦励振力（非線形べき特性）と弾弦位置の限定的な演奏マッピング。
- Pitch Wheel => 音高（張力の代替としてモデル化）の平滑化。位相を保持して連続変化。
- Channel Pressure / Polyphonic Aftertouch => 接触を模したモードごとの散逸（増加減衰）。
- Mono legato => 重複Note On時に弦の振動を再励振せず、周波数のみ連続移動。元の音高へ復帰も可能。
- Poly mode => 最大16同時ボイス。Sustain CC64に応答。
- Universal Patchbayで `NEW PHYSICAL 3.2`、`EDIT EXPRESSION`、`.pgraph`保存。
- `panoramize_upgrade32`: 既存3.0/3.1 `.physical30` / `.pgraph` / `.joint31` を3.2の演奏可能パッチへ移行（旧ファイルは保持）。
- 新版プラグイン製品コード`Pz32`、旧版との同居を想定。

## ビルド・テスト

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -G Ninja
cmake --build build -j4
ctest --test-dir build --output-on-failure
./build/panoramize_expressive32_demo /tmp/output_directory
./build/panoramize_upgrade32 candidate_1.joint31 /tmp/converted.pgraph
```

Windows用の手順は `docs/STUDIO_ONE_V32_ACCEPTANCE_JA.md` に記載。

## 機能の限界

弦は動的なモード近似であり、3D形状、弦と指板の非線形接触、完全な物理モデリング奏法、実録音からのMIDI/奏法の自動認識は実装していません。ブリッジ・胴は3軸の実効フィルタです。音高変更は実際の弦張力の物理解ではなく振動周波数の制御です。モノレガートのノートオーバーラップを操作する仕様で、あらゆる奏法を自動判定しません。

Windows VST3・JUCE GUI・pluginval・Studio One実機は未検証。旧72ファクトリープリセットと以前のGraphPatchは維持しています。

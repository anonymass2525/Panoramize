# PANORAMA typeGPT 0.2 — Cross-Synthesisの構造

## DSP信号経路

1 MIDI NoteOn→Voiceの独立した発振器へ周波数とベロシティを供給。
2 VA部は帯域制限Saw/Pulse/Sineの合成を計算。
3 Digital部は位相変調Carrierと多段Waveshapingを計算。
4 経路選択で共鳴体に入力する励振信号を切り替える。
5 8基のモード共鳴器が励振信号を再帰的に処理。
6 一部の経路では共鳴出力を次サンプルのPMまたはVAの振幅変調へ再利用。
7 既存のフィルター、エンベロープ、ボイス合計、DC除去を通してステレオ出力。

### 経路表

| 値 | 名前 | 物理的な処理 | フィードバック |
|---:|---|---|---|
| 0 | Classic | 0.1と同等のVA+Digital励振、および既存Hybrid処理 | 既存の単一サンプル履歴 |
| 1 | VA→Modes | VAをModal Resonator入力に使用 | なし |
| 2 | FM→Modes | Digital/FMをModal Resonator入力に使用 | なし |
| 3 | Ring→Modes→PM | VA×DigitalをModalに供給し、Modalの前サンプル出力でDigitalのPMを変調 | 1サンプル遅延 |
| 4 | Modes→VA AM | VAで励振した共鳴出力がVAの振幅を変調 | 音声入力方向とは逆向きのAM経路 |

すべてAudio-rateの演算。`routing` は `Hybrid` エンジンでのみ有効です。`depth` は非Classic時のみ有効。`stereoResonance` は8共鳴モードに左右の小さな振幅差を付与し、演算したモードの和からステレオを作ります。

## 重要な技術上の注意

- この段階は汎用的なGraphコンパイラではありません。**検証可能な5つの固定ルート**です。
- ルート3のフィードバックは `crossDelay` を使用した1サンプル遅延であり、ゼロ遅延の代数ループはありません。
- 既存12音色は全てRoute 0で、実際のレンダリングWAVが旧版と完全一致することを検証しました。
- Resonatorのモード周波数や減衰係数はNoteOnで初期化されます。演奏中のそれらのパラメーター変更を完全に反映するモーフィングは今後の改善事項です。
- 全体の制御可能パラメーターは、固定構造の範囲です。将来の0.3でtyped DSP graph（Audio/Control/Event）、安全なFeedbackノード、任意接続UIへ進みます。

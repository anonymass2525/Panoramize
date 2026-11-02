# PANORAMA typeGPT Prototype 0.9.0 — 実装・検証レポート

## リリース要約

- 新DSP: Matter Weave（NodeType 17）。Spectral Matter/Matter Weaveが生成した**12成分のスペクトル内部状態**を、他のノードの内部処理に直接渡す。元の音声をMixするだけではない。
- 新4音色: 49 Luminous Reciprocity、50 Porcelain Weather、51 Bioluminescent Memory、52 Reciprocity Engine。合計 **52** プリセット。
- プラグイン識別子はPnG9 / com.independent.panorama.typegpt.v09として0.8から変更。旧セッションを保護するための措置。Studio Oneでの併存は未検証。
- 0.8で継承されていた重大なJUCE choice値上限ミスを修正。従来の`juce::jlimit(0,14,...)`では選択値15のSpectral Matterを正しく復元できない（Phase Ecologyへ丸められる）。0.9では新規Matter Weaveまで選択できるよう`jlimit(0,16,...)`に修正。JUCE保存・復元の検証コードも追加、**Windowsでの実行は未実施**。

## 実際に検証した結果（Linux）

| 試験 | 判定 |
|---|---|
| C++17 / CMake Release | PASS |
| CTest DSP（52プリセット発音・構造検証・音声確認など） | PASS 1/1 |
| ASan / UBSan Debug CTest | PASS 1/1 |
| 0.8の48プリセット × 3サンプルレート（44.1/48/96kHz） | **144/144 SHAではなくFNV64ハッシュ一致** |
| 0.8 PCMハッシュのテスト長 | 1プリセット/レートにつきMIDIノート60、float32ステレオ 4096フレーム |
| 0.9の4プリセット（3レート）| すべて有限、検証したサンプル範囲内で振幅制限内 |
| Matter Weave内部状態テスト | upstreamの音声出力を0にしても音が出て、Bのスペクトル変更で出力波形が変化 |
| 非スペクトル型ノードからの状態接続、A不在、グラフの循環 | コンパイル時に拒否 |
| 4種類の48kHz / 24bitステレオWAV | 作成・ピーク/RMS測定済み |
| 試聴用MP3（48kHzステレオ） | 作成・FFmpegデコード成功 |
| Windows VST3・JUCE Smoke・pluginval・Studio One | **未実施** |

### 新規音色の実測

| No. | Name | Peak | RMS |
|---|---|---:|---:|
| 49 | Luminous Reciprocity | 0.3909 | 0.0658 |
| 50 | Porcelain Weather | 0.3718 | 0.0681 |
| 51 | Bioluminescent Memory | 0.4081 | 0.0736 |
| 52 | Reciprocity Engine | 0.3590 | 0.0658 |

### CPU負荷：依然として重要な制約

同じLinux環境のReleaseビルドで、48kHz・**0.2秒（9,600フレーム）** の音声を合成した処理時間の実測。1回測定であり、Windows/Studio Oneでの性能値ではない。

| No. | 4音同時 | 16音同時 |
|---|---:|---:|
| 45（参考0.8音色）| 200 ms | 1,011 ms |
| 49 | 139 ms | 582 ms |
| 50 | 107 ms | 472 ms |
| 51 | 136 ms | 622 ms |
| 52 | 146 ms | 637 ms |

16音同時では**0.2秒の実時間予算200msを全新規音色で超過**。4音同時でもOS/DAWの揺らぎを踏まえると十分なリアルタイム余裕があるとは保証できない。初回は48kHzで少数音から検証する。

## 未解決事項

- 0.9は最初の「typed spectral-state bus」であり、位相・材料・共鳴パラメータまで網羅する完全な統一音響アーキテクチャではない。
- 周波数領域のスムーズフェードにより部分音を減衰するが、全エンジンの厳密なエイリアシング除去は未達。
- JUCEのグラフ編集・保存・再読込はWindows CIで検証する必要がある。Windows用のJUCE実機ビルド済みVST3は同梱していない。
- 主観的な音色品質はユーザーの試聴判断を待つ。内部モデルの新規性は「好みの改善」や「究極の音質」を保証しない。

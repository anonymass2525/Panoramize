# PANORAMA typeGPT Prototype 1.5.0 — 検証レポート

## ベースライン

- 旧1.4.0の全ソースZIPを展開してLinux上でC++17 Releaseビルド：成功。CTest 2/2合格。
- 1.5.0は同じコードを基礎に新規ノード・共通状態バス・JUCE選択肢を追加。

## 実施済みの検証

1. Linux ReleaseビルドとCTest: DSP本体、FastMath、UnifiedStateの3/3成功。
2. 専用UnifiedStateテスト: 61〜64番で正常発音（44.1/48/96kHz）、上流PCM無音状態でも型付きデータで発音、物理ノードのColor変更による変化、不正型・不正ノード番号・循環グラフの拒否、既存共鳴状態の物理バス接続、8音同時ピッチベンド中の有限値と振幅上限を検査。
3. AddressSanitizer / UndefinedBehaviorSanitizer: 上記専用UnifiedStateテストに対して異常検出なし（ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 / UBSAN_OPTIONS=halt_on_error=1）。**サニタイザーで全60旧音色やJUCEを検査したわけではない。**
4. 旧60音色×3サンプルレート×4096ステレオフレーム：旧1.4のライブラリと新1.5のライブラリをそれぞれの正しいヘッダーでビルドした比較実行体でPCM出力を生成。**すべてバイト一致**。双方のraw PCM SHA-256: `01670be3a1ecc037444c189f6bd5ca647e8e1a2421c7e19ab657d758c998d30a`。
5. 新規61〜64番: 48kHz 24bitステレオ・各2.8秒WAVを実レンダリング。MP3プレビューは4曲連続・約11.232秒。

## 実測音量（48kHz、2.8秒）

| プリセット | Peak | RMS |
|---|---:|---:|
| 61 Mechanical Aurora | 0.2361 | 0.0693 |
| 62 Kinetic Choir | 0.2226 | 0.0492 |
| 63 Elastic Memory | 0.2043 | 0.0390 |
| 64 Material Imagination | 0.2357 | 0.0485 |

## Linux簡易CPUベンチ（4,096フレーム @ 48kHz、実時間予算85.33ms）

| プリセット | 4音 | 16音 |
|---|---:|---:|
| Mechanical Aurora | 36.8ms | 153.5ms |
| Kinetic Choir | 38.7ms | 183.9ms |
| Elastic Memory | 72.8ms | 297.7ms |
| Material Imagination | 71.2ms | 264.1ms |

共有Linux実行環境での単回測定なので参考値。**16音同時のリアルタイム性能は未達**。4音同時でもホストや他の処理に使う余裕が少ないプリセットがある。

## Windows/JUCE/Studio Oneの状態

- JUCEパラメータタイプへのマッピングを拡張、JUCE統合テストに2新ノードの保存・復元・音声処理のチェックを追加。**Windows/JUCEの実際のコンパイル・テストは未実施。**
- GitHub ActionsのWindows VST3ビルドワークフローは用意。pluginval strictness 5は未実施。
- Studio One上のv1.5動作試験は未実施。

## 「統一音響エンジン完成形」との差

スペクトル、位相、物理（変位・速度・力・エネルギー）を一つの状態フレームで交換する実働基盤は実装。しかし、一般物理現象の万能なモデル、厳密なエネルギー保存、任意の状態間変換、自由な遅延フィードバック、完全なリアルタイム安定性、商用製品レベルのVST3検証は未達。**v1.5.0は完成した統一状態交換コアを持つ研究版であり、理想の完全統一音響エンジンの完成を宣言するものではない。**

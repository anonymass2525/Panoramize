# PANORAMA typeGPT 0.4.3 — Modular Patchbay Hotfix 2

## 原因（既存0.4.3のソースで再現）

プリセット1〜16は独立した4つの従来方式を使用します。これらのFactory Patchは
Modularグラフの8ノードがOff、OUTに入力がない状態でした。GUIでSynthesis Engineを
Modular Patchbayに切り替えると、音源アルゴリズムだけが切り替わって接続は未設定のため
無音になります。DSPで実際に再現すると、48000Hz/8192フレームのRMSは0でした。

## Hotfix 2内容

- 明示的にModularに切り替えられ、かつ完全に未接続のグラフに限定して、
  Harmonic Additive → Phase Warp → Mix / Digital / Ring / Modal → Outputという
  有効なスターター構造を生成します。
- DSP側に安全なフォールバックを実装。GUIを開かないDAWの自動化、旧セッションの再読み込みでも音が出ます。
- GUIタイマーのメッセージスレッドで、必要な場合のみAPVTSにグラフを永続化。画面の
  ノード設定と実際に鳴っている音の経路を一致させます。
- 既存の有効なモジュラー配線（プリセット17〜32や独自編集）と、意図的に無音にした
  接続済みグラフを上書きしません。閉路グラフは引き続きエラーで無音です。
- 修正の影響範囲と旧セッションの復旧をDSP/ JUCE統合スモークテストに追加しました。
- 旧0.4.3の配布ZIPに残っていたpluginval成功文字列の誤判定も修正し、
  Start-Processの$proc.ExitCodeで判定するよう修正しました。

## 実測・検証（この環境ではLinuxのみ）

| チェック | 結果 |
|---|---|
| 旧プリセット0→Modular、48000Hz / 8192f / MIDI60 | 修正前RMS 0 → 修正後 RMS 0.0998409 |
| 従来の32 Factory presets × 44.1/48/96kHz × 4096ステレオf | 旧0.4.3と出力バイト単位で一致 |
| 上記PCM SHA256（旧／新） | ac5653fb66e7deaff0afc33e3244e6ea41b0ed7bec0695235df462f6b7989735 |
| Linux GCC C++17 Release DSP CTest | 1/1 PASS |
| Linux GCC Debug AddressSanitizer / UBSan CTest | 1/1 PASS |
| Windows MSVCコンパイル / JUCE統合 CTest | **未実行（GitHub Actions再ビルドが必要）** |
| pluginval Strictness 5 / GUI | 以前の0.4.3でユーザーがSUCCESSを確認。**本Hotfix 2は未検証** |
| Studio One でこの修正版の演奏 | **未検証** |

## 互換性

CMakeの製品名、バージョン表記、manufacturer code、plugin code、BUNDLE_IDを変更していません。
旧0.4.3のプラグイン置換が目的です。ただし、実プロジェクトの保存・復元・
自動化互換性はStudio Oneで最終確認してください。
既存のVST3インストールを上書きする前にバックアップを推奨します。

## GitHub Actionsへ反映

`PANORAMA_typeGPT_v0.4.3_ModularPatchbay_Hotfix2_ReplaceFiles.zip`内の
`include/`, `src/`, `juce/`, `tests/`, `ci/`をGitHubリポジトリのルートへ上書きします。
`PANORAMA_typeGPT_v0.4.3_ModularPatchbay_Hotfix2_Source.zip`なら全ソースです。
リポジトリをGitHub Desktopでcloneし、エクスプローラーで該当ファイルを
上書きし、Commit → Pushする方法が確実です。

Gitパッチを利用する場合には `git apply --check` でプレビューしてから適用。
すでにCIスクリプトを個別に直してあるリポジトリでは、`ci/`のパッチが衝突する可能性があるので、
その場合はDSP/JUCE/テストコードのファイルだけ更新します。

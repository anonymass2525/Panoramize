# PANORAMA typeGPT Prototype 0.4.3 — 検証レポート（2026-10-27）

## 実装内容（0.4.2からの差分）

1. プラグインの表示名・バージョンを0.4.3へ更新。既存DAWセッションでの互換性を考慮し、VST3製造者コード・プラグインコード・bundle IDは維持。
2. `PANORAMA_BUILD_VST3` 有効時にWindowsでDSP＋JUCE Processor/Editorの独立スモーク検証ターゲットを構築。`getNumPrograms()`、MIDI Note On/Offによる非無音発音、工場プリセット、Midnight Geometry ReforgedのHarmonic発振器、APVTS状態の保存復元、GUIの生成・リサイズ・破棄を検査する試験コードを追加。
3. GUIにおけるホストからのプログラム変更・セッション復元後のプリセットコンボボックス同期を追加（受動的更新、音声処理部は不変更）。
4. Windows x64 Releaseビルド、CTest、VST3構造チェック、pluginval strictness 5（GUI有効）の自動ワークフローとローカルPowerShellスクリプトを追加。JUCE 8.0.16-security、pluginval v1.0.4を固定参照。
5. Studio One実機の手動受け入れテスト手順を文書化。

## この環境で実際に実行した検証

| 項目 | 結果 |
|---|---|
| Linux x64 / GCC 14.2 / C++17 DSPビルド | PASS |
| Linux CTest（DSP） | 1/1 PASS |
| Linux Debug `-fsanitize=address,undefined` CTest | 1/1 PASS |
| v0.4.2 vs v0.4.3 PCM回帰比較：32プリセット×3レート（44100/48000/96000Hz）×4096ステレオフレーム | バイト完全一致 |
| 結合PCM SHA-256（両版） | `4c38e53ef30dfc1b9eae0b0d10cc55fd512e7a432702f1aaca234542526b29ff` |
| CMake Windows設定/Windows MSVCビルド | **未実施** |
| JUCE Processor/Editorスモークテストの実行 | **未実施** |
| Windows x64 VST3バイナリ生成 | **未実施** |
| pluginval strictness 5・GUI検査 | **未実施** |
| Studio One実機 | **未実施** |

**重要:** 新しいJUCEテストのコードとWindows CIの存在はコンパイル成功・検証成功を示しません。現在のLinux環境にはWindows/MSVC、JUCEソース、Windowsのプラグイン実行環境がなく、外部のJUCE取得もできなかったため、実際のVST3の完成・検証を宣言できません。

## 次に実行すべきWindows検証

`docs/BUILD_WINDOWS_JA.md` に従ってWindowsの `ci/Build-WindowsVST3.ps1` またはGitHub Actionsを実行してください。もしWindowsビルド・JUCEスモーク・pluginvalが通った場合に限り、Windows x64 VST3を生成物として扱えます。Studio Oneのスキャン・MIDI演奏・GUI・プリセット保存復元が通るまではStudio One実機確認済みとはしません。

0.4.3はDSP音色改良版ではなく、Windows VST3の構築と検証のための品質ゲートを整備した準備版です。

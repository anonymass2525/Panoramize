# PANORAMA typeGPT 0.4.3 — Windows C2660修正

## 原因
Windows GitHub Actions の MSVC が `juce/PluginProcessor.cpp` の
`addParameter(v, ...)` で C2660 を報告しました。
補助関数 `addParameter` と基底クラス `juce::AudioProcessor::addParameter`
の名前が衝突しており、C++ の名前探索によって補助関数が隠れていました。

## 修正内容
`addParameter(` を `addFloatLayoutParameter(` に一括変更。
補助関数の定義1箇所＋呼出21箇所＝計22箇所です。
DSP本体、32プリセット、パラメータID、VST3識別子は変更していません。

## GitHubでの最小修正手順
リポジトリのルートから `juce/PluginProcessor.cpp` を開き、鉛筆アイコンで編集。
`addParameter(` を `addFloatLayoutParameter(` に全箇所置換して main に
コミットしてください。`push` によってActionsは自動再実行されます。
Windowsで次のエラーが出た場合はログを再提出してください。

## 検証済みと未検証
ファイル上の変更箇所数と元DSPソースの不変性は確認済み。
LinuxのDSPテスト結果はWindows JUCE/VST3成功を保証しません。
Windows版VST3本体、pluginval、Studio Oneは未検証です。

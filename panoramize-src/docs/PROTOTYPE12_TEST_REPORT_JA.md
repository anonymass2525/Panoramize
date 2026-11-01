# PANORAMA typeGPT 1.2.0 — 技術検証レポート

## 実装

1. `juce/PluginProcessor.cpp`, `juce/PluginProcessor.h`: `samePatchConfiguration`で全音響パラメータとグラフ、2本のモジュレーションレーンを比較し、変化時のみ `Synth::setPatch` を実行。Studio Oneでの毎ブロック係数再構築を回避する狙い。浮動小数点の生値比較を用いるためホストが微小値を変えた場合は更新する。ノートオン／オフ、MIDIイベント処理はそのまま。
2. `src/PanoramaDSP.cpp`, `include/PanoramaDSP.h`: Spectral Matterの固定Color時に使う分散比の12成分平方根を、グラフ設定時に一度だけ共有キャッシュ。動的Color（オーディオレート変調）は従来どおりの式で処理。浮動小数点演算順序 `base*bin*factor` を維持。
3. `juce/PluginProcessor.h`, `juce/PluginEditor.cpp`, `CMakeLists.txt`, `.github/workflows/windows-vst3.yml`: バージョン1.2.0およびVST3識別子 `Pn12` / `com.independent.panorama.typegpt.v12`。
4. `tests/test_panorama.cpp`: パラメータ比較の新規回帰テスト。

## Linux実測結果

| 項目 | 結果 |
|---|---|
| C++17 Releaseビルド | 成功（GCC 14.2） |
| CTest DSP／FastMath | 2/2 PASS |
| 旧1.1とのPCM比較 | 56プリセット×44.1/48/96kHz×2音同時×4096ステレオフレーム。計1,376,256 float32サンプル**全一致** |
| 1.2試聴4音色と1.1試聴WAV全長比較 | 48kHz、24bit、ステレオ、6.7秒×4音色のPCMが**全バイト一致** |
| ASan/UBSanスモーク | 56プリセット×3レート×4音同時×2048フレーム=344,064フレーム PASS |
| MP3変換とデコード | 成功 |
| Windows VST3、JUCE統合テスト、pluginval | **未実施** |
| Studio One 1.2実機 | **未実施** |

## CPUについて（重要）

同一Linux環境・48kHz・4音・0.05秒処理の8回交互起動による非固定CPU負荷測定では、プリセット45/49/53/56の1.1→1.2中央値はそれぞれ25.763→21.809ms、33.331→32.761ms、34.486→30.665ms、32.122→30.625ms。改善の傾向は見られるが、実行ごとの変動が大きく、**安定した一般的高速化率としては扱えない**。

変更のないパッチを5万回評価するCPU制御経路の単体ベンチでは、従来の `setPatch` 毎回実行がおよそ47〜67ms、変更なし判定によるスキップがおよそ1.5〜2.3ms（3回の計測）。**これはDSPパッチ再構築の処理時間比較であり、DAW全体のCPU削減率ではない。**

1.2の16音・48kHz・0.05秒区間の単回計測（プリセット45/49/53/56）は85.8/146.6/130.5/113.9msと、50msの実時間予算を超過。音色と品質の検証ができても、16音リアルタイム安定性能は**未達**。同一Windows機材での再検証が必要。

## Studio Oneで確認すべき項目

- GitHub Actions: Windows x64 VST3、JUCE統合スモーク、pluginval Strictness 5。
- 旧1.1、1.0と1.2が別のVST3として列挙されること。
- 設定を変更しないブロックでは発音が安定して継続すること。Color等のオートメーション・プログラム変更・保存と再読込に確実に追従すること。
- 48kHz / 96kHz、128/256/512/1024サンプルブロック、4/8/16音のDAW CPUとドロップアウト。
- リアルタイム／オフラインWAVの音色とタイミングが一致すること。

**研究版の評価：Linux DSP・音声互換性は確認。Windows VST3の実機動作は未検証。**

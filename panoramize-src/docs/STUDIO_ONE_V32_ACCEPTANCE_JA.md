# Panoramize 3.2.0 - Studio One導入・演奏チェック

> 2026-10-31: このソースのWindows VST3、pluginval、Studio One動作はまだ未検証。Linux C++17 DSPテストのみ成功。

## ソースからのWindowsビルド

1. GitHubリポジトリに ZIP のディレクトリ内容を追加し、`.github/workflows/windows-vst3.yml` のWindows CIを実行する。
2. JUCE 8.0.16-security を用いて x64 Release VST3をビルドし、21件のDSP CTestとJUCE Smokeを実行する。CIはpluginval strictness 5 GUIテストも試み、成功後にVST3 artifactを保存する。
3. Windows VST3 bundleが生成・検証できたときのみ、既存旧版を消さずに新しいPanoramizeをStudio Oneへ導入する。VST3 ID `Pz32`。
4. Studio Oneでインストゥルメントとして読み込み、**NEW PHYSICAL 3.2** をクリック。
5. **EDIT EXPRESSION** に進み、各物理操作を設定。元のパッチ一覧へ戻って **SAVE GRAPH** で `.pgraph` 保存可能。
6. MIDI C3~C5で鳴らし、Note On velocityの強弱を変えて、変化の有無を確認。
7. Pitch Bend（既定 ±2 半音のJUCEマッピング）を動かし、位相のクリックを確認。
8. Channel Pressure（MIDI D0）、Poly Aftertouch（MIDI A0）、重複ノートによるmono legato、Sustain CC64を確認。
9. プロジェクトを保存して閉じ、再読み込み後パッチとGUIの設定が復元されるかを確認する。
10. 48kHz／128・256・512サンプル、Realtime/Offlineレンダリング、複数インスタンス、長時間、CPU/ドロップアウト、voice stealing（16音）を検証する。

## 旧3.1推定モデルの移行

```powershell
panoramize_upgrade32.exe candidate_1.joint31 converted_32.pgraph
```

または3.1の`candidate_1.pgraph`を第一引数に指定する。保存先は異なる名前にすること（旧パッチを壊さない）。`converted_32.pgraph`をPanoramizeのPatchbayで読み込む。

変換は**推定した弦・胴・ブリッジの共通モデルを新しい演奏発音器に引き継ぐ**機能であり、3.1が推定したすべての個別演奏条件をMIDIへ自動転写する機能ではない。

## Windows未検証の留意点

この環境にはWindows、JUCE SDK、Studio Oneがないため、GUI表示やpluginvalを実行していない。CI成功報告が出る前に実用状態と判断しないこと。音質・音量・オーディオバッファ・CPU負荷をStudio One側で別途確認すること。

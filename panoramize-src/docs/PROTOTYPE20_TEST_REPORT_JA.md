# PANORAMA typeGPT 2.0.0 — 技術検証結果

## 実装した機能
- 複数録音のMIDIノート・ベロシティ・ノートオフ時刻を持つ `InstrumentExample` を追加。
- 同じ GraphPatch を訓練用の複数演奏に適用する `inferPlayableInstruments` を追加。平均誤差だけでなく、最悪の演奏条件へのペナルティを設置。
- 探索用データから独立した holdout を探索終了後のみ評価。構造の異なる上位3パッチを出力。
- `panorama_inverse_instrument` CLIを追加。CSV＋WAV→3つのJSON、WAV、評価CSV。既存 `GraphInverse-v1` のJSON形式と旧単音CLIは維持。

## 検証済み
- Linux C++17 Release ビルド成功。
- CTest **7/7成功**（旧6件＋新しい複数演奏Inverseテスト）。
- ASan + UBSanの新規探索テストは合格。
- 旧1.9.0の72プリセット×44.1/48/96kHz＝**216条件**のPCM FNV-1aハッシュ完全一致。各条件4096ステレオフレーム、MIDI 60、velocity 0.75。
- holdoutのWAVを別の波形に差し替えても、訓練で選択されたグラフ・訓練誤差は不変。holdout誤差だけが変化するテストを通過。

## 実際の入力WAVを使う演奏横断テスト
3訓練例：C3 / C4 / C5、velocity 0.40 / 0.80 / 0.67、noteoff 1300 / 4096 / 2400。
2未使用検証例：G4 / G3、velocity 0.57 / 0.92、noteoff 1600 / 900。

### 既知Harmonic発音器（コントロール）
- 探索予算80、評価80、出力に到達する異なる構造27種類。
- 3つのGraphPatchを出力。
- 最良案train損失 **4.00345e-10**、holdout損失 **3.47931e-10**（16bit WAV量子化後）。
- 最良案は参照音のHarmonic単独グラフを正しく復元。

### 固定54候補に含まれない Harmonic → Fold 発音器
- 探索予算160、評価160、異なる構造61種類。
- 3つのGraphPatchを出力。
- 第1案train **3.4093e-05**、holdout **3.51862e-05**。
- 第2案train **1.51639e-04**、holdout **1.58883e-04**。
- 第3案train **5.04173e-04**、holdout **4.87197e-04**。
- 第1案は合成器の `Harmonic → Fold → Output` 型の構造を再発見（パラメータの完全一致ではない）。

以上は同一エンジンで合成した合成音を対象とする研究試験であり、実在楽器や未知音源での高い再現率を保証しません。

## 未検証
- Windows JUCE/VST3ビルド、pluginval strictness5、Studio OneでのJSONロード・試奏。
- 長い奏法（解析窓は4096サンプル）、レガート、連続ピッチ変化、ノイズを含む録音、実録音の汎化。
- 多音同時、リアルタイムCPU負荷の測定。Inverse探索自体はオフライン専用。

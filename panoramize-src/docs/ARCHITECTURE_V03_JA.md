# PANORAMA typeGPT 0.3 — Editable Modular Graph

- `GraphPatch`：固定長9ノード。各Nodeはtype、入力source A/B、Level、Colorを保持。
- `compileGraph()`：先行ノード制限なしで依存グラフを作成し、トポロジカルソートする。循環、自己参照、無効参照は拒否。
- `Voice::GraphState`：各MIDIボイス・各ノード固有の位相、FM位相、フィルター履歴、モード共鳴履歴、乱数状態。
- `Voice::processGraph()`：毎サンプル、コンパイル済みトポロジカル順でDSPを実行。音声サンプルの参照をA/Bへ供給。
- `Synth::setPatch()`：パラメーターのクランプ、グラフ検証、フィルター係数準備。
- `PluginProcessor.cpp`：JUCE APVTSの各ノードパラメーターを構造体へ変換、状態保存・復元。
- `PluginEditor.cpp`：種類・A/B・Level・Colorの編集GUIを表示。非循環/循環の状態表示。

## リアルタイム安全

`processGraph()`では固定長配列とボイス内の事前確保状態のみを使用する。ファイルI/Oやmutexを使用しない。ノード間の信号はステレオ2ch。初期化時と再設定時の共鳴モード係数の更新はNoteOn時。モーダル共鳴のピークと総和にはソフトクリッピングを使用。

グラフ編集に伴う信号跳躍や音色クリックの完全抑制、複雑なフィードバック、Audio/CV/Eventの型分離、音質評価はまだ未完成。

## 旧版互換

Engine `0...3` の音声演算は旧版のままとして、Engine 4をModular専用に追加。旧版の16音色の固定WAVをハッシュ照合し完全一致を確認。ただしVSTの永続状態互換性は別途検証が必要。

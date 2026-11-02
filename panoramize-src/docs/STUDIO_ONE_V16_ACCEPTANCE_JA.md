# Studio One / Windows 1.6 受入テスト（未実施）

1. GitHub ActionsでWindows x64 VST3のReleaseビルドが通る。
2. JUCE smoke：全68音色、MeshBody/FeedbackDelayのAPVTS保存・復元、Editor起動・破棄。
3. pluginval strictness 5でGUIを含む検証合格。
4. Studio Oneでv1.5とv1.6が別IDで共存する。
5. 65〜68番のMIDI試奏、ピッチベンド、ノートオフ・保存復元、リアルタイムとオフラインレンダーの差異を検証。
6. 44.1/48/96kHz・64/128/256/512/1024フレーム、1/4/16音でCPUとドロップアウトを確認。
7. 自作の任意物質形状のUI編集は未対応。InverseはVST3 GUIから実行しない（オフラインCLI）。

合格報告にはWindows実機ログが必要。

# PANORAMA 1.2 — Studio One受入試験（未実施）

1. Studio Oneを閉じ、1.1など既存バージョンとプロジェクトをバックアップ。
2. GitHub ActionsのWindows x64 Release、CTest、JUCEスモーク、pluginval Strictness 5の完走を確認。
3. 1.2 VST3バンドルを配置し、Studio Oneで独立した `Pn12` として認識されることを確認。
4. 49/51/53/54/55/56の音声出力、モジュラーグラフ変更・保存・再読込を確認。
5. ホストから音色Color、Filter、Mod Laneをオートメーションし、変更のないブロック、変化するブロックとも連続的に演奏できるか確認。
6. 44.1/48/96kHz、128/256/512/1024サンプルブロック、4/8/16音のCPUとドロップアウトを記録。
7. 1.1と同一MIDIからWAVを作り、リアルタイム／オフラインの差も含め比較。

Linuxでのテスト合格はWindows実機成功の証明ではない。

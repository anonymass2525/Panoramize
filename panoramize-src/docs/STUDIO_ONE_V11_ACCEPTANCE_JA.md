# PANORAMA 1.1 — Studio One 実機受入手順（未実行）

旧0.9、1.0のVST3とセッションをバックアップしてから試験する。1.1は独立したID `Pn11`、Bundle ID `com.independent.panorama.typegpt.v11` を用いる。これは共存を狙った設計であり、ホストでの正常共存を証明するものではない。

1. GitHubに1.1ソースの**フォルダーの中身**をコミット／Pushする。
2. GitHub Actionsの `PANORAMA typeGPT 1.1.0 Windows VST3 and JUCE Validation` を確認。Windowsビルド・DSP/JUCE統合テスト・pluginval strictness 5がすべて通ってからArtifactを取得。
3. Studio Oneを終了してから1.1の`.vst3`をWindows VST3フォルダーへ配置。0.9や1.0のバンドルは上書きしない。
4. Studio One上でファクトリープリセット全56件、特に49/51/53/54/55/56の発音確認。
5. 48kHzで1/4/8/16音のリアルタイム再生・ドロップアウト・CPUを観測。96kHzも試す。音切れがあればボイス数を下げる。
6. 保存・再読込、ノートオン/オフ、サスティン、ピッチベンド、モジュレーション、Modular Patchbayのスターターグラフ切替を確認。
7. 音質比較は0.9、1.0の同MIDIプリセットと録音をAB比較。1.1でCPUが減ったと判定するには同一Windows環境・ブロックサイズ・プリセット・ボイス数で複数回測定する。

LinuxでDSP CTestが通ったことはWindowsでVST3が動くことを意味しない。

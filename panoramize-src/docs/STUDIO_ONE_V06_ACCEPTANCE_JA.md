# PANORAMA typeGPT v0.6.0 — Studio One実機検証チェックリスト（未実施）

1. GitHub ActionsでWindows x64 ReleaseのVST3とJUCE Integration Test、pluginval strictness 5 GUIを成功させる。
2. 0.5.0のVST3とStudio Oneソングをバックアップ。0.6版の同一IDプラグインを重複インストールしない。
3. 48kHz Balancedで0.6をロードし、全40プログラムを切り替えて発音／停止／保存／再読込をチェック。
4. 37〜40番でModular Patchbayを開き、Resonant Exchangeノードの選択、Input A/B、Color、Mod Matrix 2レーンの操作を確認。
5. 0.5の33〜36番および0.4の17／25番に戻し、以前の音色・MIDI発音が損なわれていないか確認。
6. リアルタイムとオフラインのWAVを書き出し、先頭の無音や遅延がないか確認。
7. 44.1 / 48 / 96kHz、ポリフォニーとCPUパフォーマンスを測定。96kHz 16音は高負荷の可能性。
8. 既存ソングのAPVTSとパラメータ自動化保存復元をテスト。ノード型Choice拡張に伴う互換性を重視。

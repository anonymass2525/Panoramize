# Studio One: Prototype 1.4 受け入れ試験

**Windows実機試験未実施。** GitHub ActionsのWindows x64 Release / JUCE / pluginval strictness 5が全合格した後、旧VST3をバックアップし、独立したVST3「PANORAMA typeGPT v1.4.0」としてインストールしてください。

1. Studio One 48kHz/256〜512 samplesで読み込み、57〜60番を単音・4音・16音で確認。特に4音でも処理時間が重い58番に注意。
2. 57番を選択し、Phase Ecology（Node 2）の音声Levelが0でもPhase-Matter Bridgeが鳴ることを確認。
3. Node 2のColorを変更してBridge音色が変わることを確認。
4. 新ノードを選択し、パッチを保存・再読込。新しいVST3識別子が旧版のセッションを上書きしないことを確認。
5. 48kHz→96kHz、Realtime/Offlineバウンス、複数インスタンス、MIDIのノートオフ、ピッチベンド、オートメーションを試験。
6. CPU過負荷、音切れ、NaN/クリップ、クラッシュ、状態保存の問題があればログとWAVを保存。

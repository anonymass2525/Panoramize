# Windows / Studio One 1.7 実機受け入れ手順（未実行）

1. 1.6などの既存プラグインとStudio Oneソングをバックアップする。
2. Windows GitHub ActionsのVST3ビルド、JUCE smoke、pluginval strictness5が全件成功したか確認する。
3. 新ID `Pn17` のVST3をインストールし、旧版との併存を確認。
4. 69〜72番を少数発音→16音、44.1/48/96kHz、オフライン/リアルタイム、ノートオン/オフ/ピッチベンド/保存復元で検証する。
5. `panorama_inverse_graph input.wav recipe.json 60 preview.wav` で生成したJSONを、新VST3の `LOAD INVERSE GRAPH` ボタンから読み込み、出力・ノード配線・ホスト保存復元を確認する。読み込んだパッチは変更前にコピーして保存する。
6. GUIを閉じた状態での安定性、CPU負荷、無音切替、長時間使用、複数インスタンスを確認する。

**このチェックリストは実行結果ではありません。**

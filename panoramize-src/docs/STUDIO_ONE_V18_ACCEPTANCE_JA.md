# PANORAMA 1.8 — Studio One受入テスト（未実施）

1. GitHub ActionsでVST3コンパイルとJUCEスモークを確認し、pluginval strictness5 GUIを実行。
2. 旧版のVST3をバックアップしてから、Plugin ID Pn18が独立製品として認識されるか確認。
3. 旧72音色の演奏、MIDIノート・ピッチベンド、GUI表示、プロジェクト保存・再読み込みを確認。
4. オフライン `panorama_inverse_graph` に任意の単音WAV（16/24bit PCM）を渡し、JSONとプレビューを生成。
5. 1.8 GUIの `LOAD INVERSE GRAPH` にそのJSONを指定し、ノード配線・音色設定・ホスト状態保存を確認。
6. 生成したモジュラーグラフを48kHzで少数同時発音から試し、CPU負荷・無音・クリック・エイリアシングを記録。

現段階で、このWindows受入テストは実施していない。

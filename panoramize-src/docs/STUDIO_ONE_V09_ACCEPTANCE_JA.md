# Prototype 0.9 Studio One確認手順（ユーザー環境）

1. 0.8 VST3のバックアップを保持。0.9は新規ID（PnG9）であり、別のプラグインとして認識される想定。
2. GitHub ActionsでWindows VST3ビルド + JUCE Smoke + pluginvalが実際に全成功したか確認。
3. Studio One 48kHzに0.9を追加し、プリセット49–52の発音・CPU・ノートオフ・長音・同時4音を試す。
4. Patchbayでノード2個をSpectral Matter、ノード3をMatter Weaveにし、A=Node1/B=Node2。音の変化を確認。
5. グラフ編集の保存→プロジェクト終了→再読み込みでノード種別を保持するか確認。
6. 0.8旧セッションを開き旧バージョンと0.9が混同されていないか確認。
7. Buffer 64/256/1024、48/96kHzの負荷・無音・クラッシュの有無を検証。

ここまでの項目はWindowsホストでは**未検証**。テスト成功と混同しない。

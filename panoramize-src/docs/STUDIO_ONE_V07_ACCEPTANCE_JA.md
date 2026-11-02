# Studio OneでのPANORAMA 0.7 実機検証（未実施）

1. PANORAMA 0.6や0.4.3のVST3をバックアップし、Studio Oneを終了する。
2. GitHub Actionsで**0.7用にビルドされた** `.vst3` バンドルが生成され、CTestとpluginval Strictness 5が成功したことを確認。
3. VST3バンドルを `C:\Program Files\Common Files\VST3\` に導入。旧版と混在させない。
4. Studio OneでPANORAMA 0.7をスキャン。プリセットが44種類表示され、41〜44をMIDIで演奏できるか確認。
5. Modular Patchbayのノード型コンボボックスで「Resonant Exchange」「Phase Ecology」の両方が選択できるか確認。
6. プリセット41のNode 3のColor、入力AとB、LFO/Audio-rate mod、DAWオートメーションの作用を確認。
7. 48kHz / 96kHz、ブロックサイズ64/256/1024、1〜16音でピーク負荷・ドロップアウト・非数値・異常音を記録。
8. 保存→Studio One終了→再読み込みで音とGUI/プリセット状態を確認。0.6プロジェクトの互換性はバックアップを用意して検証。

**ここに記載したWindows/Studio Oneの検証は実施済みとみなしてはならない。**

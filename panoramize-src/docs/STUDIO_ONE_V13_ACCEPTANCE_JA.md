# Studio One — PANORAMA 1.3実機受け入れチェック

1. GitHub ActionsのWindows x64 Release、JUCE smoke、DSP CTest、pluginval Strictness 5が全て合格すること。
2. 1.2と1.3が別プラグインとして認識され、旧プロジェクトが更新されないこと。
3. プリセット37〜40および56を中心に、48kHz/96kHz、バッファ128/256/512/1024、4/8/16音同時発音を試し、ドロップアウトやCPU変化を測定する。
4. 長いノートの途中でピッチベンド・Colorオートメーション・ModWheel・グラフ接続編集を行い、クリック、無音、NaN、音色の不連続を確認する。
5. 未使用SpectralMatterを追加し、Outputへ接続しない場合のCPUを1.2と比較する。接続して音が鳴ること、閉路は拒否されること。
6. 保存・再読込・複数インスタンス・リアルタイム/オフライン比較を試す。

Linuxの成功だけでWindows実機成功としないこと。

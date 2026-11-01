# PANORAMA typeGPT 0.5.0 — Studio One実機受け入れテスト

**本ファイルは手順書であり、現時点の合格レポートではありません。** Windows x64、Studio One本体で人が操作して判定してください。

## 推奨検証（各項目に PASS / FAIL / 未実施 を記録）

- [ ] VST3のスキャンとプラグインインスタンスの生成に成功し、Studio Oneが落ちない。
- [ ] GUI表示、ウィンドウのリサイズ、スクロール、閉じる／開き直す動作が正常。
- [ ] MIDI鍵盤またはピアノロールでノートオン／オフ、ベロシティ、サステイン、ピッチベンド、Mod Wheelが機能する。
- [ ] 36プリセットから 17 Midnight Geometry / 25 Midnight Geometry Reforgedを含む異なる音色が実際に切り替わる。
- [ ] 25 Midnight Geometry Reforgedの最初のノードが Harmonic Additive である。
- [ ] 33 Tidal Alloy / 34 Living Lattice / 35 Glass Metabolism / 36 Phase Habitat を選び、Coupled Modesの入力Bを変更すると音質が変わる。
- [ ] 旧プリセットからModular Patchbayに切り替えても無音にならない（Hotfix 2回帰）。
- [ ] GUIの9ノードと2変調レーンを変更すると実音が変化する。無効な循環配線は拒否され、ホストをクラッシュさせない。
- [ ] ホストの保存→終了→再起動→プロジェクト再読込でプリセット番号、手動編集、Modulation Matrixが元に戻る。
- [ ] GUIを開いた状態でホストオートメーションを実行してもフリーズしない。
- [ ] サンプルレート44.1/48/96kHz、ブロックサイズ64/128/256/512/1024で持続発音と停止を検証する。
- [ ] モノラルへの折り畳みで異常な消失がない（聴感確認）。
- [ ] 10分以上の演奏とプリセット切替、停止／再開で異常音・NaN/Inf・クラッシュがない。

## 結果記録テンプレート

- Studio Oneバージョン：
- Windowsバージョン：
- VST3ファイルのSHA-256：
- pluginval strictness 5 / GUI：
- 全体判定：PASS / FAIL / 未実施
- 発生した問題と再現手順：
- 検証者・検証日時：

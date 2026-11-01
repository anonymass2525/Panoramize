# Panoramize 2.7 — Studio One 実機受け入れ検証

未実行の試験手順です。チェック完了前にStudio Oneで動作確認済みとは報告しないこと。

1. Windows GitHub ActionsあるいはMSVCを使いビルド。VST3の実体とpluginval strictness 5を確認。
2. `Panoramize v2.7.0.vst3` バンドルを標準VST3ディレクトリに配置。旧Pz26/Pz25は別IDなので共存可能。
3. Studio Oneで新規インストゥルメントとして読み込む。起動時は `DirectState27` → `Output` という新規ネイティブ楽器。
4. MIDI C3〜C5で単音を鳴らす。ゼロ音、ノイズ爆発、NaN、再生停止、音程不一致がないか。
5. GUIの `UNIFIED PATCHBAY` → `EDIT ACOUSTIC STATE` に入り、VA / FM / Spectral / Phase / Physical / Vocal / Resonance / Nonlinearを編集。
6. `MODE` の比率・減衰・非線形・集音ゲイン・励振、`COUPLING` の端点・保存／散逸・強度を編集。編集後すぐ音が変わるか。
7. `BACK TO PATCHBAY` → `SAVE GRAPH` で .pgraph を保存。DAWプロジェクトを保存、Studio Oneを終了して再起動、音響グラフと音が復元するか。
8. エディターを100回開閉・リサイズ。編集と再生を繰り返し、クラッシュやクリックノイズがないか。
9. 44.1 / 48 / 96 kHz、64/128/256/512/1024フレームを試す。Reatime/Offlineレンダリングのサンプル整合性を確認。
10. 最大16音ポリフォニーで負荷・グリッチ検証。32音はLinuxベンチでリアルタイム時間予算超過、使用注意。
11. 古い2.5/2.6の .pgraph を LOAD GRAPHで読み込み、互換モードの音色が再生できるかを確認。
12. 問題があればGitHub Actionsのビルドログ、pluginvalログ、Studio Oneの再現手順・WAVを保存。

注意：現行のGUIは機能的な状態編集画面で、先に作成した高級デザインモックアップの完全な実装ではありません。

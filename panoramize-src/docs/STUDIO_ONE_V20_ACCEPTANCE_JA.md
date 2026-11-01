# PANORAMA 2.0 — Windows / Studio One 受入テスト（未実施）

1. GitHub ActionsでJUCEコンパイル、DSP全CTest、GUI smokeを合格させる。
2. Windows x64 VST3バンドルの存在とpluginval strictness5 GUI検証を確認。
3. 以前のPn19とは独立したID `Pn20` でStudio Oneに認識されることを確認。
4. 旧72プリセットのMIDI発音、音色変更、MIDI noteOff、プロジェクト保存・再読み込みを検証。
5. Windows上でオフラインCLIの `performances.csv` から複数JSONを生成し、それぞれを「LOAD INVERSE GRAPH」で読み込む。
6. MIDI 48、60、72、67、55など訓練・未訓練のピッチ、ベロシティ、ゲート時刻を比較。
7. 発音停止、無音、クリック、再読込、シングル/ポリ負荷、CPU負荷を記録。

まだWindows実機は実行していない。Linux DSPの結果と混同しないこと。

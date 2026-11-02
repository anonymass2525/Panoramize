# PANORAMA typeGPT 1.0.0 — Studio One 受入検証（未実施）

このドキュメントは**作業手順**であり、成功したと主張するものではありません。

1. 0.9 VST3とは別のProduct ID `Pn10` とBUNDLE IDでWindows x64 VST3をGitHub Actionsビルド。
2. CTestでJUCE smoke検証（新Formant Field含む）とpluginval strictness 5を通す。
3. Studio One 48kHzで0.9の49/51音色と、1.0の49/51を聞き比べる。0.9の既存セッションが変化しないことを確認。
4. プリセット53〜56を発音。エディタを開閉、パラメータ自動化、保存・再読込、96kHzの短時間出力も検証。
5. 4音/8音/16音時のCPU負荷とドロップアウトを実機測定。Linuxで16音はリアルタイム予算未達。
6. サンプルレート変更、複数インスタンス、Realtime/Offlineバウンスを確認。

受入完了は全手順の証跡が必要。

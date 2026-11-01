# PANORAMA typeGPT Prototype 0.6.0 — 検証レポート

## 検証実施範囲

| 項目 | 状態 |
|---|---|
| GCC C++17 Releaseビルド（Linux） | PASS |
| Linux DSP CTest | 1/1 PASS（内部に複数assert） |
| Linux ASan / UBSan Debug CTest | 1/1 PASS |
| 旧36プリセット × 44.1 / 48 / 96kHz × 4096 ステレオサンプル | **旧0.5とバイト完全一致** |
| SHA256（旧36プリセット出力） | `3e6afee2b0106f2195d7a0fe60ed01bf74082cad1c2c1bfe8612cac1d47a9473b` |
| 新規プリセットの音声出力 | 4/4 可聴データ／有限値 |
| Node B（エネルギー交換レート）の出力変化 | DSPテスト PASS |
| Input Aを無音にした際のゼロ出力 | DSPテスト PASS |
| 44.1/48/96kHzと複数ボイス処理 | DSPテスト PASS |
| 変更後のサンプルレート再初期化 | DSPテスト PASS |
| 音響グラフ循環拒否 | DSPテスト PASS |
| JUCE GUIコンパイル（0.6） | **未実施** |
| Windows VST3（0.6） | **未実施** |
| pluginval strictness 5（0.6） | **未実施** |
| Studio One実機（0.6） | **未実施** |

## 新規4プリセットのWAV検証

48kHz・24bit PCM・ステレオ・6.7秒、同じメロディー＋和音で比較。

| # | プリセット | Peak | RMS |
|---|---|---:|---:|
| 37 | Resonant Loom | 0.3466 | 0.0697 |
| 38 | Amber Tides | 0.4358 | 0.0856 |
| 39 | Ferroflora | 0.3934 | 0.0707 |
| 40 | Aurora Reciprocity | 0.2599 | 0.0532 |

## CPU参考値（Linux、単一スレッド、計算量検討用）

16音同時、1秒分の音声を連続処理した壁時計時間。測定値はWindows/DAWのリアルタイム性能を保証しない。

| 音色 | 48kHz | 96kHz |
|---|---:|---:|
| Tidal Alloy（0.5参考） | 838 ms | 1667 ms |
| Resonant Loom | 796 ms | 1530 ms |
| Amber Tides | 843 ms | 1685 ms |
| Ferroflora | 861 ms | 1900 ms |
| Aurora Reciprocity | 872 ms | 1675 ms |

**96kHz・16音では処理予算1秒を超えているため、リアルタイム演奏での音切れリスクがある。** 48kHz・少ない発音数から試奏し、オフラインレンダリングも比較してほしい。CPU負荷の改善は次の開発課題。

## 互換性と注意

- VST3の `BUNDLE_ID` / `PLUGIN_MANUFACTURER_CODE` / `PLUGIN_CODE` は0.4/0.5と同一。Windowsに旧版と同時導入しない。
- 新規ノードはGUI Choiceの末尾に追加。旧プリセットのPCMは維持したが、Studio One既存セッションの完全な保存復元・自動化互換性は**未確認**。
- 今回の0.6は音響研究プロトタイプ。実測音響材料モデルではなく、エイリアスの定量評価、CPUの最適化、サンプル精度のパラメータ変更などは今後の研究課題。
- GitHub Actionsのビルド手順は含むが、CI結果が成功するまで0.6のWindows版は完成とは呼ばない。

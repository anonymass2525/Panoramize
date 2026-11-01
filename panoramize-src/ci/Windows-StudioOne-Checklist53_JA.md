# Panoramize 5.3 — Windows/DAW 実測受け入れ基準

**未実施。** この文書とCIコードは、実機合格を意味しません。

## 自動検証: GitHub Actions Windows Server 2022 / Visual Studio 2022

1. `cmake -DPANORAMA_BUILD_VST3=ON -DPANORAMA_BUILD_JUCE_SMOKE=ON` を実行する。
2. 全DSP + JUCE processor/editor smoke + VST3をMSVC x64 Releaseでビルドする。
3. 旧長時間テストを除くCTestを実行。v5.3のJUCE host smokeは**g48 / g49 / g51のautomation値更新、MIDI発音、ValueTree state保存・復元**を検査する。
4. x64 `.vst3` DLLの存在・最低サイズを確認。 `ci/Validate-WindowsVST3-53.ps1` でpluginval strictness 5 **GUI含む**を実行し、終了コード・ログ・`windows_validation53.json`を保存する。
5. 検証に失敗した場合はvst3成功アーティファクトを作成しない。

## Studio One (Windows PC所有者が手動で実施、未実施)

| 項目 | 証跡 | 現状 |
|---|---|---|
| Fender Studio/Studio OneのVST3スキャン、GUIを開く | ホストのプラグイン一覧とスクリーンショット | 未実施 |
| 44.1k / 48k / 96k, 64 / 128 / 256 / 512 frames | 16以上のレンダーログ | 未実施 |
| MIDI noteOn/off, legato, sustain, pitch, pressure | MIDIと録音WAV | 未実施 |
| g48/g49/g51のホストオートメーションを描き・保存・復元 | プロジェクト再読み込み後のパラメーター値とWAV | 未実施 |
| グラフを保持ノート中に変更、PCM遷移・ノートオフ・CPU | WAVおよび音切れ・XRUNログ | 未実施 |
| 10回のGUI開閉 / 2インスタンス / 20分連続実行 | 再現可能な手順とログ | 未実施 |
| オフラインレンダリングとリアルタイムレンダリング比較 | 48kHz WAVとサンプルレベル比較 | 未実施 |
| Studio Oneの終了・再起動、プロジェクト保存復元 | DAWプロジェクト・スクリーンショット | 未実施 |

## 重要な境界

- GitHub Actionsでpluginval合格してもStudio One実機検証済みとは見なさない。
- C++17の`shared_ptr` atomic操作は必ずしもlock-freeではない。5.2のgraph swapはhard-realtime安全性未証明。
- 5.3の`observability53`は追加測定を設計する診断器であり、未知の全物理構造を録音から完全復元するツールではない。
- 既存VST3識別子`PnGP/Pz52`とバンドルIDを維持して旧5.2セッション互換性を優先。JUCE製品表示名のみ5.3へ更新。

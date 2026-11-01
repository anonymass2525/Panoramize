# Panoramize 5.2 — Windows / Studio One validation checklist

**このファイルは実行済みの合格証明ではありません。** 実機での検査記録欄です。LinuxからStudio One自体の操作はできません。

## Automated GitHub Actions (Windows Server 2022 x64)

1. Pinned JUCE 8.0.16-securityを取得。
2. Visual Studio 2022 MSVC Release x64ビルド。DSP、JUCE smoke、VST3を全ビルド。
3. CTest対象の完走、JUCE smokeで音声出力・状態保存・グラフ切替・保持ノートの継続を確認。
4. VST3 x64 DLL構造とサイズを検査。
5. pluginval v1.0.4, strictness 5, GUI tests enabled; exit code nonzeroで失敗。
6. ログ・VST3をActions artifactへ保存。

## Studio One Windows x64 (manual, NOT DONE)

| 項目 | 実施結果 |
|---|---|
| VST3 scan / instantiate, GUI present | 未実施 |
| 44.1/48/96 kHz × 64/128/256/512 sample buffer | 未実施 |
| Mono/stereo, multi-instance | 未実施 |
| Held note during structural patch edit; transition click/pop observation | 未実施 |
| Notes-off, sustain CC64, pitch wheel/aftertouch during transition | 未実施 |
| Automation g48/g49/g51, recording and session re-open | 未実施 |
| Fast automation stress, invalid/NaN parameter rejection | 未実施 |
| Offline bounce matches expected render / tail and silence | 未実施 |
| CPU overload, repeated GUI opening/closing, long-run leak checks | 未実施 |
| pluginval strictness 5 with GUI | 未実施 |

## Limits

- v5.2の「crossfade」はPCM波形を約12ms混合。内部の全FEM/スペクトル位相やエネルギーの移送ではない。
- 構造グラフはUI側で準備する。現在はJUCE message threadで同期prepareするため、重いFEMはGUI応答を止め得る。
- Windows実機測定が済むまで「Studio One stable」とは評価しない。
- `std::atomic_load/store(shared_ptr)` はロックフリーを保証しない。callbackからの解放を制限してもhard realtimeは未証明。
- レガシーDSP testの一部は数分実行になるため除外する場合は、テスト名と理由をログに残す。

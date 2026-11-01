# Panoramize 5.2 — Safe Coefficients, Note-Aware Graph Replacement, Provenance-Aware Evaluation

検証日: 2026-11-01。Linux C++17 Releaseビルド。**Windows/JUCE GUI/Studio One実機未検証**。

## 実装

- C++ `safe52::CoefficientBank`: 入力を有限値・範囲検証後にatomic publish、callbackで指数平滑化。検証済みbatchから段階的に各atomic fieldを更新するため、**全フィールド同時の不可分コミットではない**。
- C++ `swap52::SmoothGraphSwap`: 制御スレッド側で準備した既存`universal23::Instrument`を音声スレッドへ公開。保持中MIDIノート・ノートオフ、ホイール、ピッチベンド等を2つのエンジンへ転送し、約12msのPCMクロスフェード。古いグラフの参照は一時保持して次のUI公開時に回収。内部状態の完全転送ではない。
- `juce/PluginProcessor.cpp`: 従来の構造変更時即時swapを、上記切替処理へ変更。既存`tryUpdateEnergyGraph`をUIスレッドから実DSPに呼んでいた経路を停止。`PLUGIN_CODE=Pz52`, 独立バンドルID。
- `juce/UnifiedParameterEditor50.h`: 既存の幅広い内部パラメータGUIに検索とライブ変更可項目のフィルターを追加。これは全FEM/声道の専門GUI完成を意味しない。
- `panoramize_joint50 --infer-provenance`: ライセンス、出典URL、録音セッションID、音高、ホールドアウト区分を保存。学習/ホールドアウトで同一session IDを使う漏洩を拒否する。
- Windows CI: JUCE 8.0.16-security、VST3 MSVC x64 Release、JUCE processor/editor smoke、pluginval strictness 5の設定と手動Studio One確認表。**CI実行ログは未取得**。

## 実測・試験結果

| 検査 | 結果 |
|---|---|
| 全Linux Releaseターゲットをビルド | 成功 (198 build actions) |
| 5.2専用C++ swap+coefficient test | 合格 |
| Linux主要回帰10テスト（23/40/43/44/47/49/50/51/52） | 10/10合格 |
| `safe52::CoefficientBank`単独ASan/UBSan | 合格 |
| 5.2新ソースを計測したASan/UBSan専用試験 | 合格（リンクした旧DSPライブラリーは非サニタイズ） |
| provenance TSV（人工音3テイク） | 6候補を再生・ランキング、CSV出力成功 |
| 同一録音sessionがtrain/testに重複 | EXIT 2、拒否成功 |
| 旧DSPを含む全CTest | 全件完了確認に至らず。全件合格は主張しない |
| Windows VST3コンパイル、pluginval | 未実施 |
| Studio One実機のノート継続、GUI/セッション復元 | 未実施 |
| 実録音の未知楽器逆設計精度 | 実WAV未取得、未評価 |

試聴デモ: 24kHz、同じNative27の音高57を保持中にNative47へ切替、2秒の出力。切替1サンプル間の振幅差（このデモだけの値）は、PCMクロスフェード **0.00202988**、即時切替 **0.00396233**。これは主観的音質や任意の構造・位相でのポップ低減を保証しない。`Panoramize_5.2_Swap_AB.mp3`はクロスフェード→即時切替の順。

## 既存の逆設計器の実行データ（人工音、実録音ではない）

- 3テイク、既知MIDI 57/60/63。6候補、18評価。学習では`hybrid`が最上位、未使用音高では`physical_string`が相対的に有利。
- 正しい独立session IDなら推定・エクスポートが動作、同じsession IDを学習と未使用評価の両方に指定すると拒否。
- 外部録音を取り込み、メタデータを監査する経路はあるが、**実在の録音を取得して精度を測ったことはない**。人工デモの `https://example.org/` はテスト用ダミー出典であり、実録音の根拠ではない。

## 現状の限界

1. 旧DSPを含めてすべての係数更新を原子的・連続的に扱う統一機構は未完成。Native27の既存5msスムージングは維持されるが、全FEM・音響係数の変更には再prepareが必要。
2. クロスフェードで保持ノートは引き継がれるが、位相、構造エネルギー、FEM節点速度、リバーブ履歴は移植しない。サステインで既にキーオフになっている音や複数グラフの特殊MIDIステートも完全移植されない。
3. 音声コールバックにおけるC++17 `atomic_store(shared_ptr)` はプラットフォームにより内部ロックを取る可能性がある。サンプルごとにpending取得を行う実装は今後のさらなるリアルタイム監査が必要。UI message thread上で重いprepareを同期実行するため、UIフリーズの可能性あり。
4. スタンドアロンJUCEコードのWindows実コンパイル、MSVCの全CIテスト、pluginval、Studio Oneホストセッションの保存・復元、音色を聴いた評価は未確認。
5. 不明な物理構造・未知材質・声道について単一音声から完全な一意復元はしていない。

## 追加のWindows検証

- `.github/workflows/windows-vst3.yml`
- `ci/Windows-StudioOne-Checklist52_JA.md`

実録音資料候補（実データ未取得）:
- University of Iowa Musical Instrument Samples: https://theremin.music.uiowa.edu/MIS.html
- TinySOL 2020 (CC BY 4.0): https://zenodo.org/records/3685367
- Meta Open Sound (CC0 instruments): https://github.com/facebookincubator/Meta-open-sound

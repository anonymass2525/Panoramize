# Panoramize 3.5 / Studio One 受入チェックリスト（未実施）

- GitHub Actions（Windows x64, JUCE pinned）Release VST3生成、CTests全件合格、pluginval strictness 5およびGUI検証。実行結果とログを保管。
- Studio OneのVST3スキャンでPanoramize 3.5を旧版とは別プラグインとして認識するかを確認。
- SIX STRING FEM / 3.5を選び、E2 A2 D3 G3 B3 E4の開放弦和音を演奏。
- ブリッジ結合0/0.08で音響変化、パームミュートCC22とピッチベンド、サステイン、ハンマリング／スライドCC20を聴く。
- 編集→.pgraph保存→再読み込み→DAWプロジェクト保存→終了→再起動→復元後の同音・同設定を確認。
- 48kHz/44.1kHz、バッファ64/128/256/512、1インスタンス／複数インスタンスでXRUN・CPU・メモリ・ノイズを観測。
- Realtime/Offline WAV差分を分析。特に旧版で発生した初回2048サンプル遅延の再発を確認。
- GUI自動化（操作中のグラフ再構築）の音切れを検出。運用上、現在の再構築は非Realtime領域に限定することを推奨。

以上は今後実機で確認する項目であり、このリリースで成功を宣言するものではない。

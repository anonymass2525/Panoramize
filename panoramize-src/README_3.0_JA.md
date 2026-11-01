# Panoramize 3.0.0 — Performance-Aware Physical Inverse Synthesis

新しい `PerformancePhysical30` コアによって、WAVのみから弾弦オンセット・音高遷移・急減衰・ビブラート候補を追跡し、複数の単音録音から有効弦/ブリッジ/共鳴胴のモデル構造を探索します。

- Linux C++17 `ctest`: 19/19 PASS。新機能のASan/UBSan PASS。
- 既存PANORAMA/PanoramizeのDSPと72ファクトリープリセットを維持。
- 新 `.physical30` モデル形式、`.pgraph`の `PhysicalCoupled30` 発音ソース（Kind=12）。JUCEパッチベイのソース一覧にも追加。
- 出力例: `/demo` の `recordings.txt`, `inferred/candidates.csv`, `inferred/candidate_*.physical30`, `inferred/candidate_*.pgraph`, `inferred/candidate_*.wav`, `performance_analysis/events.csv`, `performance_analysis/tracking.csv`。
- Windows VST3は**ソース／ワークフローのみ**、未ビルド・未検証。Studio Oneでの実際の読み込み・演奏は未検証。
- **研究上の重要な注意**: 3軸共鳴体は有効モードのフィルタ群であり完全な3D FEMではありません。未知の材質/形状や実演奏の奏法を一意に同定する機能ではありません。

詳細は `docs/PROTOTYPE30_TEST_REPORT_JA.md` を参照。

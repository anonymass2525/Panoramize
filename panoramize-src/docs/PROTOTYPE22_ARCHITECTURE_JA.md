# PANORAMA 2.2 — Unified Energy-Normalized Acoustic State Graph

## 設計原則

1. **音響ドメインに依存しない保存状態**: StateModeは domain, frequencyHz, dampingPerSecond, quartic, sensor calibration を持つ。内部は q,p の1自由度2状態であり、例えば12成分のスペクトル分布自体を直接保存する場合は12個のモードを用いる。将来は可変長ベクトル状態へ一般化する。
2. **Power-normalized connection**: 個別ドメインの観測物理量をそのまま接続しない。正規化座標のみが接続可能。必要な単位換算はノードごとに明示する。
3. **Passive vs Active**: K=K^T>0、D=D^T>=0。各接続は保存的なポテンシャル、速度依存の散逸、または外部能動ポートのいずれかに分類し、エネルギー収支へ載せる。
4. **Cyclic solution**: Conservative/Dissipativeの任意の循環は一括で対称正定値行列に入れ、サンプル遅延なしに計算する。一般の非線形フィードバック系すべてを解くものではない。
5. **Discrete gradient**: 正の4次ポテンシャルは平均勾配 `(q1^3+q1^2q0+q1q0^2+q0^3)*lambda/4` を用いる。これはポテンシャル差を離散的に正確に表すため、数値解が収束した範囲では保存・散逸・仕事を整合させられる。
6. **Rendering limits**: 128モード上限、密行列利用。非線形モデルは各サンプルでニュートン法と行列分解が必要。大規模リアルタイム処理では疎行列、モード縮約、SIMD化が必要。

## 今後の拡張

- 真のベクトル化SpectralMatter/Phase Ecology、声帯・声道の音響管/調音モデルを、型付き入出力と適切なエネルギー伝達関数を持つブロックとして接続。
- 既存3D物理FEM `AcousticGraph21` の変位/速度/力を、明示的に校正されたpower-portへ接続（今回の2.2では独立コアのまま）。
- コンパイル時に保守結合の安全性を検証し、必要に応じてゼロ遅延非線形ループの収束/安定性を検証する一般ソルバー。
- JUCE VST3のポリフォニック再生、CSV/JSONグラフエディター、保存・復元、自動化、リアルタイム負荷制御。
- Inverse Synthesisの探索対象を、型付き状態グラフと物理パラメーターへ拡張。

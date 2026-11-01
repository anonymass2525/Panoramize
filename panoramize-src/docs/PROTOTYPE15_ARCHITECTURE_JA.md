# Prototype 1.5.0 — 統一音響状態バスの設計

## 共通フレームと意味

`AcousticStateFrame` はフレーム単位で以下を保持する。

| 要素 | 値 | 単位／範囲 |
|---|---|---|
| `spectralEnergy[12]` | 部分音ごとの音響エネルギー重み | 正規化・非負（厳密な物理Jではない） |
| `phaseCycle[12]` | 各部分音の位相 | 周期 [0,1) |
| `displacement` | 物体モデル位置 | m（1kg参照モデル内） |
| `velocity` | 物体モデル速度 | m/s |
| `force` | 外力 | N |
| `mechanicalEnergy` | E=½mv²+½mω²x²、m=1kg | J |
| `domains` | Spectral / Phase / Mechanical ビットマスク | 静的型チェックに対応 |

更新は各声部・各ノードの毎サンプル。ノードはトポロジカル順序で走る。型付き読み取りは依存元の生存・先行実行を保証した後のみ。`Level=0` はPCMレベルだけをミュートし状態を消さない。

## PhysicalBody

物理座標 `x` および正規化速度 `u=v/ω` を持ち、減衰 `ρ<1` の回転更新で、自由振動の正しい位相を保つ。

`[x';u'] = ρ [cosθ sinθ; -sinθ cosθ] [x;u] + [0; F/(ω Fs)]`

θ=ω/Fs、ω=2πf、質量m=1kg。外力は上流PCMから取り出して明示的な力[N]として導入。状態を減衰させ、発音体から変位・速度・外力・機械エネルギーを公開する。

## UnifiedCoupler

Aは型付きSpectral/Phase、Bは型付きMechanicalを必須とする。Bの力をω²で割って比較可能な長さ次元へ変換。スペクトル位相差とBの変位・外力から内部の共鳴自由度を駆動し、内部の物理座標が部分音エネルギーと位相速度を修飾する。統合ノード自身がSpectral/Phase/Mechanicalの全領域を次のノードへ公開する。

重要：これは**ノード内で1サンプル前の状態に依存する結合**。任意ノード同士のグラフにゼロ遅延のフィードバック・サイクルを作れるわけではない。

## コンパイラ側の検証

- UnifiedCoupler A: SpectralMatter / MatterWeave / FormantField / PhaseMatterBridge / UnifiedCoupler
- UnifiedCoupler B: PhysicalBody / UnifiedCoupler / ResonantExchange / CoupledModes
- 不正値、自己参照、循環依存は不許可
- 有効な変調エッジもDAGとして検証
- すべて既存の9ノード制限の下で処理する

旧版の全音色は今回変更していない。ノードの追加は列挙の末尾の番号20/21のみで、古い番号を変更しない。JUCEのUI選択はOutput=10を除外して連番マッピングする。

## 品質上の未解決問題

「実験的な型付き状態グラフ」はできたが、「あらゆる音響・物理現象の数学的な統一」はまだできていない。具体的には数値安定性の形式的証明、機械エネルギーに対応するスペクトル全帯域の変換、FEM・非線形接触などが必要。リアルタイム性能、音質評価とWindows試奏も残る。

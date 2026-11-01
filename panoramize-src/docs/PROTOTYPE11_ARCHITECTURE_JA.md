# Prototype 1.1 — DSP 設計変更

1.0の9ノード・56音色を維持。FormantField、MatterWeave、SpectralMatterの位相和を`detail::sineCycles`に置換し、数学的誤差に上限がある縮約多項式で位相発振を処理する。標準`std::sin`と最大4.4e-14程度の絶対差を示す。ただし他のノードのVA/FM/物理共鳴計算方式は変更しない。

MatterWeaveの分散比 `bin*sqrt((1+dispersion*bin*bin)/(1+dispersion))` はパッチ固定Colorのときだけ定数。これを`Synth::graphWeaveRatios_`で保持し、すべてのボイスがconst参照する。ColorのMod Laneが変化した場合は従来式をそのまま実行する。ピッチは`baseHz`の乗算に残るためベンドに追従する。パッチ更新で再計算、NoteOnごとのsqrt再計算はしない。

型付きスペクトル状態を参照する全ノードは`compileGraph`で親の型がチェックされ、DAG順に評価される。この保証のもと、毎サンプル全9ノード×12成分のゼロクリアを省略。上流ノードはそのフレームを下流が読むより先に全12個書き込む。

これは**完成した高速エンジンではなく、微小差品質維持を確認した処理経路の見直し**。数値検証と速度評価の限界は`PROTOTYPE11_TEST_REPORT_JA.md`参照。

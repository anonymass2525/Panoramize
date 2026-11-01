# Scite research capture — portable after trial expiration

Research date: 2026-11-02. Metadata and selected abstract/excerpt evidence retrieved with Scite; these are research leads, not verified implementations.

## Digital waveguides and physical modeling
1. Julius O. Smith (2008), *Digital Waveguide Architectures for Virtual Musical Instruments*, DOI: https://doi.org/10.1007/978-0-387-30441-0_25. Surveys efficient physical-model structures for string and wind instruments. Publisher access; full text not confirmed available. Design implication: evaluate waveguides as a low-CPU baseline before using spatial discretization everywhere.
2. Välimäki, Pakarinen & Erkut (2006), *Discrete-time modelling of musical instruments*, DOI: https://doi.org/10.1088/0034-4885/69/1/r01. Survey of discrete-time physical instrument modeling. Publisher access; do not treat metadata as proof of any specific solver's superiority.

## Passivity and energy ports
3. Beattie, Mehrmann & Van Dooren (2019), *Robust port-Hamiltonian representations of passive systems*, DOI: https://doi.org/10.1016/j.automatica.2018.11.013 ; open version https://arxiv.org/pdf/1801.05018. Focuses on stable/passive linear-system representations. Relevance: formalize power-conserving/dissipative port contracts. Limitation: does not directly validate nonlinear audio-rate graph implementations.
4. Camlibel & van der Schaft (2022), *Port-Hamiltonian systems and monotonicity*, DOI: https://doi.org/10.48550/arxiv.2206.09139 ; open version https://arxiv.org/pdf/2206.09139. Research lead for nonlinear/monotone coupling and passivity concepts.

## Priority experiments
- Compare explicit vs implicit coupling for two oscillators with energy exchange; plot total stored energy and solver cost over sample rates.
- Distinguish passive links, dissipative elements and active excitation in graph port metadata.
- Benchmark waveguide vs finite-difference models for equivalent resonator targets; compare pitch stability, spectral error, CPU and callback jitter.

## Research discipline
Revisit original papers before committing formulas; preserve citations and access URLs in GitHub. Scite is optional after the trial; use original publications, open repositories and independent numerical tests.

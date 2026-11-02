# Bell sound-design research references for Panoramize 5.5+

These papers inform future directions but **do not certify the current sound quality or physical accuracy** of Panoramize's stylized 8-mode Bell implementation. Research notes recorded 2026-11-02.

## 1. Karjalainen, Välimäki & Esquef — DAFx 2002
**Efficient Modeling and Synthesis of Bell-like Sounds**.
Original conference paper index: https://dafx.de/paper-archive/details/g31AEhtQ6ww5tzEZDddvkg

Why it matters: bell-like sounds can be synthesized from inharmonic partials by frequency-zooming ARMA analysis or a digital waveguide allpass loop, potentially supplemented by parallel resonators. A future CPU-vs-GPU comparison should test a waveguide approach alongside our new 8-mode filter bank.

## 2. Carvalho et al. — Heritage Science (2021)
**Physical modelling techniques for the dynamical characterization and sound synthesis of historical bells**.
Open article: https://www.nature.com/articles/s40494-021-00620-2
DOI: https://doi.org/10.1186/s40494-021-00620-2

Why it matters: realistic bells require geometry, measured or FEM-estimated modal frequencies, modal masses/shapes, excitation/clapper interaction, and mode-specific damping. The study discusses conventional tuning intervals of hum/prime/tierce/fifth/nominal and close modal pairs / warble. Our current synthetic ratio set should not be labeled a real measured bell: the purpose is creative sound design rather than historical replication.

Future R&D: modeshape-aware strike/pickup positions, paired resonances for warble, clapper contact stiffness and high-frequency damping.

## 3. Poirot, Bilbao & Kronland-Martinet — EURASIP Journal (2024)
**A simplified and controllable model of mode coupling for addressing nonlinear phenomena in sound synthesis processes**.
Open article: https://link.springer.com/article/10.1186/s13636-024-00358-2

Why it matters: coupling interactions between modes are a potential next stage for physical sonic depth beyond independent resonators. Evaluate positive energy balance, boundedness and CPU cost before adding a physically inspired coupling matrix.

## Engineering interpretation
The user especially values the **weight/realism** and metallic partial structure of factory preset #05. Our immediate 5.5 experiment preserves the unique Fractal Digital exciter while adding an opt-in resonator. Follow-up experiments should compare current sound with a physically constrained FEM/modal family, high-order waveguide and coupled inharmonic modes with controlled excitation and true time-varying damping.

Do not confuse a struck metallic plate model with a plate-type artificial reverberation effect.

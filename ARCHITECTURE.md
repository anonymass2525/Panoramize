# Panoramize — Architecture and engineering constraints

Status: design direction; verify implementation against source ZIP before asserting that a subsystem exists.

## Unified Acoustic State Graph
Target: typed connections between synthesis operators and acoustic states, including physical displacement/velocity, force/pressure, spectral state, phase, resonances, nonlinear response and voice structures. Graph connections must declare units, direction, rates, latency and valid parameter ranges.

## Energy and physical ports
Investigate effort/flow pairs, energy storage, dissipation and explicitly active excitation. For physical coupling, evaluate power balance, passivity, numerical stability and boundary conditions. Do not assume that connecting two DSP nodes produces a physically valid coupling.

## Numerical methods
Compare digital waveguides, finite-difference/FEM models and implicit coupling methods where relevant. Test stiff systems, sample-rate changes, graph topology changes, parameter automation and zero/near-zero denominators.

## Real-time audio rules
Avoid blocking locks, heap allocation, disk/network I/O and unbounded work on the audio thread. Prepare expensive graph updates off the audio thread; publish bounded, validated state transitions. Benchmark CPU, peak callback time, latency, NaN/Inf and denormals.

## Implementation source of truth
The ZIP and GitHub workflow are the current implementation artifacts. Extract and inspect actual sources before documenting specific class interfaces. The next structural improvement is to version source files directly in Git rather than treating the ZIP as the only source, after confirming no accidental changes.

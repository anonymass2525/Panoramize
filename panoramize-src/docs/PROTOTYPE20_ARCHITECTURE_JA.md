# PANORAMA 2.0 — Playable Instrument Discovery Architecture

### Input
A set of isolated-note, dry mono/converted-to-mono PCM examples. Every example is tagged with MIDI pitch, normalized velocity and sample-accurate note-off time. Separate examples into **train** and **holdout** before search.

### Objective
For each candidate GraphPatch, render all training gestures at their own conditions using PANORAMA's actual C++ DSP. Score the distance of the 3-region low-band Goertzel descriptor and log-RMS using:

`gesture_loss = spectral_feature_squared_error + 0.022 * log((predicted_rms+1e-6)/(target_rms+1e-6))^2`

`training_loss = mean(gesture_loss) + 0.20 * max(gesture_loss)`

This penalizes sacrificing the hardest gesture in order to fit only a single note. The objective is heuristic and not a perceptual metric.

### Graph search
Seed 54 legacy graph recipes, then mutate node type, node addition/removal, topological rewiring, parallel branches, levels, color and patch timbre settings. Candidate graph validation rejects invalid zero-delay algebraic cycles. Visit at most the specified evaluation budget. The top 10 quality/complexity candidates guide evolutionary steps. Archive the best fit for every output-reachable graph signature.

### Instrument proposals
Sort the archive by training error and take the top fitting graph, then further connected topologies with normalized structural distance >= 0.15 from earlier selections. No proof of historical novelty. Each is a full editable PANORAMA GraphPatch.

### Holdout
Only after the entire search is complete, synthesize the unseen gestures **for the already selected candidates**. The held-out examples do not influence mutations, early stopping, ranking or chosen proposals. Report train and test errors separately. Changed heldout audio in the unit test provably leaves the training winner unchanged while changing heldout error.

### Known gaps
The first 4096 samples (85.3ms at 48kHz) are analyzed. Long-term spectral development, legato, slides, pitch bends, vibrato, multi-velocity crossfade laws, noise/reverb robustness, stereo and advanced physical material inversion require future versions. A similarity of a few synthetic clips is not equivalent to discovery of a real-world instrument.

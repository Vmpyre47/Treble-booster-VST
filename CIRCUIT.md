# Circuit and modelling notes

This is an original software implementation of the published Dallas Rangemaster circuit. It is not a measured emulation of a particular surviving unit and is not affiliated with Dallas Musical Ltd.

## Sources checked

- [ElectroSmash circuit image](https://electrosmash.mas-effects.com/images/dallas-rangemaster/dallas-rangemaster-schematic-parts.png): checked the actual connections, especially the collector potentiometer wiper.
- [ElectroSmash original article, archived](https://electrosmash.mas-effects.com/dallas-rangemaster): original component identification and circuit analysis. Some numerical bias calculations in the article contain inconsistent current units; the software solves Kirchhoff equations instead of using those example calculations.
- [Amplified Parts build and circuit](https://www.amplifiedparts.com/pedal-guides/build-project-rangemaster) and [original terminal-strip layout](https://www.amplifiedparts.com/sites/default/files/associated_files/p-0601h02_rangemaster_layout.pdf): independent cross-check of topology and nominal component values. Their build lists 50 uF electrolytics; the implementation uses the 47 uF version.
- [Original enclosure photograph](https://electrosmash.mas-effects.com/images/dallas-rangemaster/dallas_rangemaster_treble_booster_analysis.jpg): reference for grey enamel, light faceplate, ribbed black knob, slider switch, socket and rising staff motif. Interface is drawn from code, without embedding the photograph or a Dallas logo.

## Topology

The original uses positive ground. With the audio ground at 0 V, the other battery terminal is -9 V. Q1 is PNP. Original components:

| Part | Connection | Value |
|---|---|---|
| C1 | Input to base | 5 nF |
| R1 | Base to -9 V | 470 kohm |
| R2 | Base to ground | 68 kohm |
| R3 | Emitter to ground | 3.9 kohm |
| C3 | Emitter bypass, parallel to R3 | 47 uF |
| RV | Collector to -9 V, output from wiper | 10 kohm audio taper |
| C2 | Wiper to output | 10 nF |
| C4 | Battery decoupling | 47 uF |
| Q1 | Common-emitter transistor | OC44 germanium PNP |

The virtual amplifier input after C2 is 1 Mohm. Master follows that output, so it does not load the original circuit. The total resistance of RV remains 10 kohm as Boost moves; the two pot segments and the load through C2 are included in the nodal solution. The ideal logarithmic taper gives approximately 9% at half rotation. A real vintage pot can differ.

## Numerical model

`Source/Circuit.h` solves base, emitter and collector voltages simultaneously using a pivoted Newton solve and an analytic Jacobian. Pot-wiper and output nodes are eliminated algebraically, retaining their load on the collector. C1, C2 and C3 use trapezoidal capacitor companions (voltage and current history). The PNP circuit is represented by a sign-normalised NPN equation system; both input and output signs are reversed. This preserves PNP asymmetric distortion and common-emitter inversion.

The transistor uses forward and reverse Ebers–Moll currents, so cutoff and saturation arise from junction currents. There is no waveshaper substituted for the stage. Nominal forward transport saturation current, forward beta, reverse beta, and collector-emitter leakage at approximately room temperature:

| Mode | Is | betaF | betaR | leakage |
|---|---:|---:|---:|---:|
| Germanium, OC44-like PNP | 80 nA | 90 | 2 | 15 uA |
| Silicon, generic PNP replacement | 6.7 fA | 160 | 3 | 0 |

These are explicitly chosen engineering approximations. They are not extracted from a measured OC44 or silicon specimen. Silicon retains every original passive value, so its bias changes naturally. It is a modification of the germanium original, not a second historical Dallas circuit.

Both models remain live and material changes crossfade over 20 ms. Boost, Master and On/Off are smoothed over 20 ms. Caps are initialised at the DC solution and current Boost position to avoid a startup charging thump. Changing Boost still changes the capacitor's DC operating voltage, as in the real collector pot.

The processor runs at 4x host sample rate using JUCE polyphase-IIR up/downsampling with integer latency reported to the host. On/Off crossfades at the oversampled rate and shares the filter delay; it is not a bit-exact wire bypass. This version has no user quality switch. Mono and stereo instances use independent circuit states.

## Calibration and limits

Digital +/-1.0 corresponds to +/-1 V at the circuit input. This is a deliberate fixed reference; the real voltage represented by an audio interface's DI depends on its gain setting. Reduce the DI clip gain before the plugin for a cleaner response. Master ranges from -60 to +6 dB, defaults to -18 dB, and does not feed back into the circuit. The peak indicator turns red at digital full scale. There is no output limiter.

The battery is an ideal 9 V source. C4 is electrically present in the reference circuit but has no effect across an ideal source, so no dynamic supply state is needed. No battery sag, temperature drift, resistor noise, microphonics, transistor junction capacitance, Early effect, or physical pickup/cable network is included. DI playback cannot reconstruct the original interaction of a passive pickup with the pedal input. Fourfold oversampling reduces aliasing; this is not a guarantee of alias-free operation. The trapezoidal method in 1.0.1 replaces backward Euler from 1.0.0; the independent SPICE audit is documented in CONTROL_AUDIT.md.

These limits prevent a claim of exact hardware matching. Circuit topology and nominal passives follow the source; device parameters and numerical method are documented so they can be replaced with measurements later.

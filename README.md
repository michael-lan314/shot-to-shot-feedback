# Shot to shot feedback box
Programmable electronic boxes for discrete time PID feedback between experimental shots for ultracold atomic experiment. Use cases include optical tweezer position feedback and sideprobe frequency feedback. 

Feedback is performed as follows: an error signal from the experiment is fed into a Red Pitaya board, which calculates feedback and outputs an analog voltage. Then, using a PCB summing amplifier, this voltage offsets the external experimental setpoint. Thus, this repo includes feedback code for the Red Pitaya, the PCB design, and the CAD design for a case.

# Parts list
- Red Pitaya (model __ or __ ..)
- Dual noninverting summing amplifier board (design below, needs parts)
- 3D printed enclosure: ()

### PCB construction

The PCB schematic is pictured below. The choice of resistors will depend on the desired input/output voltage ratios, using the equation: (adder equation)
The circuit is built for the following parts:
- AD706 op amp
- the BNC and SMA connectors..

# Redpitaya instructions
### setup, packages

### installation

# Usage
### modular code
While all types of feedback will have the same general structure, the error signal input, feedback logic, and output will be different for each use case. Thus the C code includes multiple input, feedback, and output modules that are called by (main).

### examples
#### Tweezer position feedback
#### Sideprobe frequency feedback

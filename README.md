# Shot to shot feedback box
Programmable electronic boxes for discrete time PID feedback between experimental shots for ultracold atomic experiment. Use cases include optical tweezer position feedback and sideprobe frequency feedback. 

Feedback is performed as follows: an error signal from the experiment is fed into a Red Pitaya board, which calculates feedback and outputs an analog voltage. Then, using a PCB summing amplifier, this voltage offsets the external experimental setpoint. Thus, this repo includes feedback code for the Red Pitaya, the PCB design, and the CAD design for a case.

# Parts list
# Redpitaya instructions
# Usage

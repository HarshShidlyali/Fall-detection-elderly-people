## This prototype Represents the Fall detection for the elderly people. In this, we used machine learning called TinyML which is used for low end end devices.Firstly, I took the predefined data set disitiguishing between the real fall dataset and the normal activity dataset.then I compared the data set i.e Real fall dataset and the normal activity dataset.and I computed the true tilt + acceleration differences, and now we can define the real FALL THRESHOLD.
--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

## RESULTS (Based on Actual Data)
### Tilt Angle Comparison
<img width="334" height="94" alt="Screenshot from 2025-12-12 23-16-47" src="https://github.com/user-attachments/assets/14e72e84-2856-48b7-aaf5-742c410d2742" />




Note :- Tilt values overlap heavily
Meaning: Tilt alone cannot separate fall from normal activity in dataset. so I computed acceleration magnitude.

ACC Impact Values
Fall Data	: ACC Max	13.99 g
Normal Data : ACC Max 4.99 g
---- Falls have huge spikes (≈ 14g) 
---- Normal activity stays under 5g

Final Fall-Detection Threshold (Accurate for the Data)
* A fall occurs when BOTH:
----- ACC (impact) > 6.0 g
AND
----- Tilt > 60° after the impact
(staying horizontal)

Why This Works for Your Dataset
*** Normal Activity:
Tilt ≈ 45°–90° (lying, sitting, bending)
ACC < 5g (no impact)

*** Fall Activity:
ACC spikes up to 14g
Tilt goes to ~90° afterward

FINAL FALL THRESHOLD (Derived from data set)
**** ACC > 6g + Tilt > 60° = TRUE FALL

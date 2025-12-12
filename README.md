## This prototype Represents the Fall detection for the elderly people. In this, we used machine learning called TinyML which is used for low end end devices.Firstly, I took the predefined data set disitiguishing between the real fall dataset and the normal activity dataset.then I compared the data set i.e Real fall dataset and the normal activity dataset.and I computed the true tilt + acceleration differences, and now we can define the real FALL THRESHOLD.
--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
## Working : _The ESP32 continuously reads acceleration and body tilt values from the MPU6050 sensor. When both the impact (ACC) and tilt angle rise above their defined thresholds, the system suspects a fall and immediately enters a 5-second verification window. At that moment, a message is sent indicating a potential fall and giving time for cancellation. During these 5 seconds, the user has the option to press the cancel button—if the button is pressed, the system recognizes the event as a false alarm and no fall alert is sent. However, if the button is not pressed within the 5-second window, the system concludes that a real fall has occurred and automatically sends a final alert message stating “FALL DETECTED! Need urgent attention.”_
--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
## * RESULTS (Based on Actual Data)
### Tilt Angle Comparison
<img width="334" height="94" alt="Screenshot from 2025-12-12 23-16-47" src="https://github.com/user-attachments/assets/14e72e84-2856-48b7-aaf5-742c410d2742" />

_Note :- Tilt values overlap heavily
Meaning, Tilt alone cannot separate fall from normal activity in dataset. so I computed acceleration magnitude._

--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

## * ACC Impact Values
### > Fall Data	: ACC Max	13.99 g 
### > Normal Data : ACC Max 4.99 g
>_Falls have huge spikes (≈ 14g) \
>Normal activity stays under 5g_
--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

## Final Fall-Detection Threshold (Accurate for the Data)
### * A fall occurs when BOTH:
----- > ACC (impact) > 6.0 g & Tilt > 60° after the impact
(staying horizontal)

--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

## Why This Works for Your Dataset
### *** Normal Activity:
> Tilt ≈ 45°–90° (lying, sitting, bending)
> ACC < 5g (no impact)

### *** Fall Activity:
> ACC spikes up to 14g
> Tilt goes to ~90° afterward
--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
## FINAL FALL THRESHOLD (Derived from data set)
### **** ACC > 6g + Tilt > 60° = TRUE FALL
--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
# Results
### ![IMG_20251212_223905](https://github.com/user-attachments/assets/a0f799f9-cd4d-45a8-8fcc-0ab46b1575ef)
![IMG_20251212_223958](https://github.com/user-attachments/assets/8773cf29-8738-447e-b3a2-bdca95d482f0)
![IMG_20251212_224036](https://github.com/user-attachments/assets/ae9d4030-7f76-4fda-9017-a582dd5dd526)
![IMG_20251212_224109](https://github.com/user-attachments/assets/d46159ef-629c-4663-99a7-92ff30b504bc)
![IMG_20251212_224132](https://github.com/user-attachments/assets/b020a01b-634f-42e9-b2d1-fd4d221d797e)

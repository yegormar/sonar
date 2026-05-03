Reasons:
- My last two projects were related to websites. 
- I enjoy working with hardware (one of the reasons why I selected Automation and Robotics for my college major)
- I decided to refresh my arduino knowledge
- Coolness factor. Adding accessibility features to the web site is a too easy and not cool.

Though process:
- Initial though was a phone based app that captures image with camera and translates it into sound. Rejected as too complex for the short project, would require App creation and no hardware
- Lidar with single sensor - poor direction
- Lidar with two sensors - that is the project that I selected

Challenges (basic):
- You need to understand the physics and fundamentals. Examples:
  - not reading knowing specs of the LED - burned my Red LED by applying 5V and had to spend whole day trying to troubleshoot why it is not turning on
  - not knowing angle of the ultrasonic sensor was giving a lot of trouble in initial prototype - was getting very strange results
- Experiment is the king! It is hard to predict real life factors, for example shape of the target, echo signals and signals from the multiple objects
- Had to recall last year physics unit on electricity just to make blue/red lights

 
UX challenge (fundamental) - how to notify user about closeness and direction.
It was difficult to invent a signalling mechanism and caused me to do several iterations.
Closeness was easier as it is more or less standard - closer to the target more frequent beeping - same as car parking. 
But selecting a tone of the beep was challenge especially when I was trying to combine tone frequency to indicate direction.

Direction. Tested approaches:
- stereo and volume (didn't work as arduino doesn't support it but most likely would be still the best design - requires more expensive setup)
- pitch frequency coding where frequency changes with the angle. Didn't work as angle measurements are jumpy and it creates a lot of beeps of different frequency that are very confusing
- different math based approaches to smooth signal - 25 experiments to capture data showed that cheap ultrasonic sensors with low angle and application of such methodologies as:
  1. State Machine with Hysteresis
  2. Moving Average Filter
  3. Entry/Exit Detection Algorithm
  4. Time-Based Smoothing
  5. Feedback Smoothing Techniques
still didn't give good results. I even considered neural network approach but rejected it as it would take me too much time with not guaranteed good result

Final selection that gives reasonably good results:
- two independent buzzers 
  - Buzzer 1 to serve as proximity beeper
  - Buzzer 2 (different pitch) to indicate when target is in the center of the view - in front of the person

Why it works. 
- Distance beeper is easily recognisable and gives urgency feeling
- center detection tells you when target is in front of you so you can either stop or turn. 

That brought final challenge to make it work it has to be very fast as it is expected that person will be moving left/right to find center.
I needed to do a lot of code optimization to make it so it will give 10-20 measurements a second.

How it works:
- ultrasonic sensor emit sound and measure echo delay (dt)
- Distance = (dt * V_sound) / 2
- there are two front sensors with narrow overlapping field
  - both sensors see target - it is in the middle
  - at least one sensor gives proximity measurement - this is how close the target is (min of reported distances)

Target Audience: Who is this application for? How can it benefit them? What is your application designed to do?
- fully or partially blind people
- for not blind but in case need to navigate in complete darkness
- energy efficient (no CPU is needed)
- doesn't emit any light (even infra-red)
- very cost-efficient (estimate is about $50 per device)
# Accessibility Project Presentation (A+ Version)

Use this as your slide-by-slide script. Keep each slide visual and speak the points in the notes.

---

## Slide 1 - Title
**Dual Ultrasonic Spatial Feedback System**  
**Accessibility Project - [Your Name] - [Class/Date]**

### On-slide points
- Low-cost assistive navigation prototype
- Converts spatial information into sound and light feedback
- Built with Arduino + ultrasonic sensors

### Speaker notes
- "This project helps users detect obstacles and align direction without using a screen."
- "I focused on practical accessibility, fast response time, and low cost."

---

## Slide 2 - Problem + Target Audience (Rubric #1)
### On-slide points
- People who are fully or partially blind
- Anyone navigating in complete darkness
- Need: detect obstacle distance and heading in real time

### Speaker notes
- "My goal was to provide usable spatial awareness with simple signals."
- "This system can support safer movement and better confidence in unknown spaces."

---

## Slide 3 - Value Proposition
### On-slide points
- Affordable (about $50 per prototype)
- Energy-efficient microcontroller solution
- No camera and no internet required
- Fast local feedback (10-20 measurements per second)

### Speaker notes
- "I designed this to be realistic, not just a concept."
- "It runs locally, so it is private, lightweight, and usable anywhere."

---

## Slide 4 - Why This Project (Rubric #5)
### On-slide points
- Wanted to build hardware, not another website
- Accessibility challenge felt meaningful
- Motivated by solving a real UX problem

### Speaker notes
- "I chose this because I enjoy robotics and wanted real-world impact."
- "I intentionally picked a harder path than a standard web accessibility assignment."

---

## Slide 5 - Design Journey and Iterations
### On-slide points
- Rejected mobile camera + audio idea (too large for timeline)
- Tested single-sensor design (poor direction)
- Selected dual-sensor design (better directional signal)
- Ran repeated experiments to validate reliability and UX clarity

### Speaker notes
- "I explored multiple approaches before committing."
- "Most of the project time was spent on experiments, failures, and redesign."
- "Experimentation was critical because theory alone was not enough."

---

## Slide 6 - How the System Works (Concepts + Application)
### On-slide points
- 2 ultrasonic sensors read left and right distances
- Distance from echo timing: `distance = (dt * speed_of_sound) / 2`
- Closest object from `min(L, R)`
- Center detection when both sensors overlap on same target

### Speaker notes
- "This applies physics, signal timing, and algorithmic processing."
- "The two sensors create spatial comparison, not just raw distance."

(picture for the slide - Left/Center diagrams)

---

## Slide 7 - Sensing Physics: Beams, Reflections, and Obstacles
### On-slide points
- Ultrasonic beams spread and reflect differently by surface shape/material
- Flat walls produce stronger echoes than angled/soft targets
- Multiple obstacles can create misleading nearest-return measurements
- Real environment physics explains sensor noise and edge cases

### Speaker notes
- "Understanding beam physics was critical to interpreting unstable readings."
- "This explains why experimentation was necessary, not optional."

(picture for the slide - Beams diagram)

---

## Slide 8 - REDUNDANT (Slide 6 is enough)
### On-slide points
- Inputs: `L` (left distance), `R` (right distance), baseline `D`
- Core logic:
  - proximity = `min(L, R)`
  - center detected when both sensors overlap on same target
  - side bias from `delta = L - R` and thresholded classification
- Use thresholds/hysteresis to prevent flicker near center

### Speaker notes
- "This slide shows the exact decision model behind center/left/right behavior."
- "The math is simple on purpose so it stays fast and reliable in real time."

---

## Slide 9 - Classes, Objects, and Interaction (Rubric: Concepts)
### On-slide points
- Sensor objects: collect and normalize data
- Processing object: calculates proximity + direction
- Feedback object: controls active/passive buzzers + LEDs
- Main loop: orchestrates read -> compute -> output

### Speaker notes
- "Even on Arduino, modular architecture helped me iterate faster."
- "This structure made testing and optimization much easier."


(picture for the slide - Electrical diagram)
---

## Slide 10 - UX Challenge + Final Audio Design
### On-slide points
- Hardest issue: communicating closeness + direction clearly
- Failed/weak approaches:
  - stereo/volume concept on Arduino (not practical with current hardware)
  - single-buzzer pitch-only coding (too confusing when readings jitter)
- Final approach:
  - Active buzzer (D3) = proximity urgency (faster beep when closer)
  - Passive buzzer (D6) = center alignment cue (object straight ahead)

### Speaker notes
- "The key was reducing cognitive load for the user."
- "The final design gave clear, non-conflicting audio information."

---

## Slide 11 - Design Decisions and Trade-offs (Core Evidence)
### On-slide points
- What worked:
  - dual-sensor overlap for center detection
  - separate buzzers for urgency vs alignment
- What failed or underperformed:
  - unstable angle/pitch mapping in noisy motion
  - complex smoothing methods without reliable UX gain
- Trade-off decision:
  - selected simpler, faster, more interpretable feedback over mathematically complex logic

(picture for the slide - one of the analysis output graphs)

### Speaker notes
- "This is the most important part of my project: design decisions based on evidence."
- "I tested many methods and kept only what improved user reliability."

---

## Slide 12 - Electrical Architecture and Wiring Diagram (Redundant Slide 9???)
### On-slide points
- Two HC-SR04 sensors provide left/right input channels
- Active buzzer (D3) handles proximity urgency output
- Passive buzzer (D6) handles center-alignment output
- LEDs provide visual state confirmation during testing/demo
- Common ground and correct resistors are required for stable behavior

### Speaker notes
- "This diagram shows the full signal path from sensing to user feedback."
- "Electrical correctness was essential to avoid false debugging conclusions."

---

## Slide 13 - Live Demo Plan (Rubric #4)
### On-slide points
- Demo 1: object far -> slow beep
- Demo 2: object near -> rapid beep
- Demo 3: move object left/right -> directional feedback
- Demo 4: center object -> center indicator trigger
- Validation message: expected behavior matches observed behavior in repeated tests

### Speaker notes
- "I will show success criteria before each demo step."
- "This demo reflects stable behavior from frequent testing, not a one-off run."

---

## Slide 14 - Learning Evidence (Rubric #2 and #3)
### On-slide points
- Applied physics (wave travel + timing)
- Applied electronics (safe wiring, resistors, power/ground integrity)
- Applied software design (modular logic, control loop optimization)
- Applied accessibility principles (clear, interpretable feedback)

### Speaker notes
- "This project combines software dev thinking with embedded systems practice."
- "Combination of software development skills with electrical engineering and physical experimentation"
- "I can explain not just what works, but why it works."

---

## Slide 15 - Reflection (Rubric #6)
### On-slide points
- What I learned:
  - Accessibility must be user-centered, not feature-centered
  - Hardware behavior in real environments is unpredictable
  - Iteration and testing are essential
- If done again with bigger time/budget:
  - Add stereo directional audio (headphones/earbuds) for richer left/right cues
  - Add left/right haptic vibration modules on arms for non-audio direction cues
  - Improve enclosure and ergonomics
  - Add calibration mode for different spaces

### Speaker notes
- "My biggest learning was designing for clarity under real constraints."
- "Next version would improve comfort, robustness, and multisensory output."

---

## Slide 16 - Optional Group Task Split (If Team Project)
### On-slide points
- Hardware lead: wiring + sensor calibration
- Firmware lead: distance/direction logic + timing
- UX/testing lead: user trials + feedback interpretation
- Presentation lead: demo flow + speaking transitions

### Speaker notes
- "If presenting as a group, this is how responsibilities can be divided clearly."

---

## Slide 17 - Closing
### On-slide points
- Objective achieved: practical accessibility prototype
- Demonstrated learning across hardware, software, and UX
- Clear path for next iteration and real-world value

### Speaker notes
- "This project demonstrates both technical depth and user-focused design."
- "Thank you - I am ready for questions."

---

## Q&A Prep (Put in presenter notes, not on slides)
- Why not camera-based AI?  
  "Too complex for timeline; this project prioritized reliability, affordability, and real-time response on-device."
- Why two buzzers?  
  "It separates urgency and alignment signals, reducing confusion."
- Main limitation?  
  "Ultrasonic sensors are sensitive to surface angle/material and have noisy close-range behavior."
- Next improvement?  
  "Add stereo directional audio and left/right haptic feedback, then add calibration for different environments."

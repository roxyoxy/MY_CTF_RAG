# htb university 2025 SleighComms

**Challenge Name:** SnowSignal OS - SleighComms Edition
**Creator:** Joaquin Iglesias (Joey)
**Prepared by:** Joaquin Iglesias (Joey)
**Date:** 17 December, 2025
**Difficulty Level:** Easy
**Classification:** Official

---
## Synopsis

A civilian sleigh has gone off-course during a snow squall on Christmas Eve, 1985. The pilot managed to send an emergency transmission using encoded bell tones before their communication equipment failed. As a Signal Operator at Tinselwick Signal Intercept Station, you must analyze the intercepted tone sequence, decode the pattern using the station's reference manual, cross-reference with the landmark database, and identify the distress location to dispatch the rescue team before time runs out.

---
## Case Briefing

╔════════════════════════════════════════════════════════════════╗
║           TINSELWICK SIGNAL INTERCEPT STATION                  ║
║                  CLASSIFIED: YULETIDE EYES ONLY                ║
╚════════════════════════════════════════════════════════════════╝

MISSION BRIEF - OPERATION: LOST SLEIGH
DATE: December 24th, 1985
TIME: 23:47 GMT
STATUS: URGENT

SITUATION:
At approximately 22:15 hours, our listening post intercepted an
emergency transmission from a civilian sleigh that went off-course
during tonight's snow squall. The pilot managed to send a distress
beacon before their communication equipment failed.

YOUR MISSION:
1. Analyze the intercepted tone sequence in SIGNAL PLAYER
2. Decode using the TONE DECODER REFERENCE MANUAL
3. Cross-reference decoded patterns with LANDMARK DATABASE
4. Identify the distress location to coordinate rescue

TIME CRITICAL:
Temperature dropping. Rescue team standing by for your decoded
coordinates. Lives depend on accurate signal interpretation.

---
## Step-by-Step Walkthrough & Explanations

### Step 1: Read the Mission Brief

Open the Mission Brief application from the desktop.

- Review the situation details
- Note the signal format: Encoded bell tone sequence
- Understand the objective: Decode the transmission to identify distress location
- Recognize the urgency: Temperature dropping, lives at stake

**Why?**
Understanding the mission context is critical. The briefing explains that Tinselwick uses a landmark coding system where bell tones represent letter pairs from location names. This establishes the decoding methodology you'll need to apply.

---
### Step 2: Analyze the Signal Transmission

Open the Signal Player application to examine the intercepted transmission.

- Observe the visual waveform with 5 distinct colored segments
- Note each segment's characteristics:
  - Segment 1: Blue block (1 DING)
  - Segment 2: Green block (2 DINGS)
  - Segment 3: Blue block (1 DING)
  - Segment 4: Yellow block (3 DINGS)
  - Segment 5: Green block (2 DINGS)

**Visual Sequence:** `[1] [2] [1] [3] [2]`

**Why?**
The color-coded visual representation makes pattern recognition easier. Each segment can be clicked individually to verify the count. The consistent color coding (blue=1, green=2, yellow=3) helps prevent counting errors during analysis.

**Pro Tip:** Click individual segments to isolate and replay them for verification. Use the playback speed controls if needed.

---
### Step 3: Consult the Tone Decoder Manual

Open the Tone Decoder Manual application for the mapping reference.

Review the tone-to-letter conversion table:

| Tone Count | Letter Pair | Description |
|------------|-------------|-------------|
| 1 DING     | ST          | Star/Stone prefix |
| 2 DINGS    | FR          | Frost/Frame prefix |
| 3 DINGS    | EV          | Ever/Evening prefix |
| 4 DINGS    | HO          | Holly/Hope prefix |
| 5 DINGS    | PE          | Pepper/Peace prefix |

Apply the decoder to your observed sequence:
```
1 DING  → ST
2 DINGS → FR
1 DING  → ST
3 DINGS → EV
2 DINGS → FR
```

**Combined Result:** `ST + FR + ST + EV + FR = STFRSTEVFR`

**Why?**
This is a simple substitution cipher where each tone count maps directly to a specific letter pair. The decoder manual provides all possible mappings (1-5 dings), making this a straightforward translation exercise once you've accurately counted the tones.

---
### Step 4: Search the Landmark Database

Open the Landmark Database application and search for the decoded prefix.

- Enter `STFRSTEVFR` in the search field
- Review the search results

**Match Found:**
- **Prefix:** STFRSTEVFR
- **Name:** Starfrost Chapel Way
- **Full Name:** Starfrost Chapel Waypoint
- **Description:** Ancient wayfinding monument where three major routes converge
- **Features:**
  - Navigation beacon combining all three district markers
  - Central waypoint for lost travelers
  - Triple-spired structure visible from all directions

**Why?**
The landmark database contains all possible distress locations. The prefix system allows pilots to encode their precise location using standardized letter pairs. Notice that "Starfrost Chapel Waypoint" combines elements from multiple districts (ST from Starshard, FR from Frostpine, EV from Everlight), making it a logical convergence point where a lost sleigh might end up.

**Critical Observation:** Other locations in the database have shorter prefixes (ST, FR, EV, HO, PE) or partial combinations (STFR, STFRSTEV), but only STFRSTEVFR matches your complete decoded sequence.

---
### Step 5: Verify Your Solution

Cross-reference your findings:

1. **Signal Pattern:** [1] [2] [1] [3] [2] ✓
2. **Decoded Sequence:** STFRSTEVFR ✓
3. **Database Match:** Starfrost Chapel Waypoint ✓
4. **Location Type:** Convergence point for lost travelers ✓

**Why?**
Verification prevents costly errors. The location's description as a "central waypoint for lost travelers" with a "navigation beacon" confirms this is exactly the type of landmark a distressed pilot would navigate toward during an emergency.

---
### Step 6: Submit the Solution

Open the Submit Location application and enter your answer.

**Acceptable answer formats:**
- `STFRSTEVFR`
- `Starfrost Chapel Way`
- `Starfrost Chapel Waypoint`
- `Starfrost`

Click Submit to dispatch the rescue team.

**Why?**
Multiple answer formats are accepted to account for different reporting conventions. Whether you submit the technical prefix code or the landmark's common name, the rescue team will know exactly where to go.

**Success Confirmation:**
Upon correct submission, you'll receive confirmation that the rescue team has been dispatched. Mission complete - you've successfully decoded the emergency transmission and saved lives through signal intelligence analysis.

---
## Skills Developed

- Signal Intelligence (SIGINT) analysis and interpretation
- Audio pattern recognition and counting
- Substitution cipher decoding techniques
- Database query and cross-referencing
- Multi-source information correlation
- Critical thinking under time pressure
- Attention to detail in data analysis
- Technical communication and reporting

---

## Answer Key

The investigation requires collecting and processing the following data points:

1. **Signal Segment 1:** 1 DING (Blue)
2. **Signal Segment 2:** 2 DINGS (Green)
3. **Signal Segment 3:** 1 DING (Blue)
4. **Signal Segment 4:** 3 DINGS (Yellow)
5. **Signal Segment 5:** 2 DINGS (Green)
6. **Tone Pattern:** [1] [2] [1] [3] [2]
7. **First Letter Pair:** ST (from 1 DING)
8. **Second Letter Pair:** FR (from 2 DINGS)
9. **Third Letter Pair:** ST (from 1 DING)
10. **Fourth Letter Pair:** EV (from 3 DINGS)
11. **Fifth Letter Pair:** FR (from 2 DINGS)
12. **Combined Prefix:** STFRSTEVFR
13. **Location Name:** Starfrost Chapel Way
14. **Full Location Name:** Starfrost Chapel Waypoint

**Final Answer:** STFRSTEVFR / Starfrost Chapel Waypoint

---

## Additional Notes

**Common Mistakes:**
- Miscounting the number of dings in yellow segment (it's 3, not 4)
- Stopping at partial prefix matches (STFR or STFRSTEV instead of complete STFRSTEVFR)
- Case sensitivity confusion (answer validation is case-insensitive)
- Missing the distinction between similar landmarks (Starfrost Evening Plaza vs Starfrost Chapel Waypoint)

**Hint System:**
If struggling, the challenge provides progressive hints after incorrect attempts:
1. Count methodology reminder
2. Decoder manual reference
3. Explicit tone sequence reveal
4. Letter pair combination guidance
5. Landmark type clue (waypoint/junction)

**Time Estimate:** 5-10 minutes for first-time solvers

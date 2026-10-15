# htb-university-2025_FrostFleet

**Challenge Name:** FrostFleet OS - RiverWatch Edition
**Creator:** Joaquin Iglesias (Joey)
**Prepared by:** Joaquin Iglesias (Joey)
**Date:** December 17, 2025
**Difficulty Level:** Medium
**Classification:** Official

---

## Case Briefing

FROM: navigation.authority@prna.gov
TO: maritime.investigator@security.team
SUBJECT: CRITICAL: Vessel Recovery - FROSTSTAR Missing
CLASSIFICATION: URGENT
DATE: 12/25/2025 00:15 EST

Navigator,

Patrol vessel FROSTSTAR went dark at 23:30 Christmas Eve during Starshard investigation patrol. AIS ghost ping received at 23:47 indicates vessel stationary 6+ hours at coordinates 42.3456°N, 71.0892°W. River ice forming rapidly. Identify exact dock location immediately for rescue dispatch.

---

## Step-by-Step Walkthrough & Explanations

### Step 1: Review Mission Dossier

Open the Mission Dossier application from the desktop.

Key information to document:
- Vessel: FROSTSTAR (18m patrol vessel)
- Captain: Hollis Wintergale
- Last Known Position: 42.3456°N, 71.0892°W
- Status: Grounded, stationary 6+ hours
- Mission: Investigating Starshard disappearances

**Why?**
The mission dossier provides the critical AIS coordinates and context. These coordinates are the foundation for all distance calculations and location verification. Understanding the vessel's draft (2.5m) is essential for depth analysis.

---

### Step 2: Examine AIS Plotter Data

Open the AIS Plotter application.

- Note the ghost ping location on the map
- Observe the 50-meter accuracy threshold circle
- Verify coordinates: 42.3456°N, 71.0892°W
- Status shows: "DISTRESS - No movement 6+ hours"

**Why?**
The AIS Plotter visualizes the search area. The 50-meter threshold means only docks within this distance are viable candidates. This immediately narrows the investigation scope from 12 possible docks to a handful.

---

### Step 3: Review Complete Dock Registry

Open the Dock Registry application and examine all entries.

Focus on docks with matching characteristics:
- D-001 "Starlight Pier": 42.3445, -71.0901 (4m depth, Ice-resistant)
- D-002 "Peppermint Wharf": 42.3461, -71.0885 (3m depth, Passenger terminal)
- D-003 "Candlewick Marina": 42.3412, -71.0923 (5m depth, Fuel depot)
- D-004 "Everlight Landing": 42.3458, -71.0890 (2m depth, Historic site, Near chapel, MAINTENANCE)

**Why?**
The dock registry provides coordinates, depth, features, and status for cross-referencing. Note that D-004 has unique characteristics: shallow water (2m), historic site, chapel proximity, and maintenance status - all potential clues.

---

### Step 4: Calculate Precise Distances

Open the Distance Calculator application.

Input the ghost ping coordinates and calculate distances to all docks:

| Dock | Coordinates | Distance | Within 50m? |
|------|-------------|----------|-------------|
| D-001 Starlight Pier | 42.3445, -71.0901 | ~141 meters | NO |
| D-002 Peppermint Wharf | 42.3461, -71.0885 | ~56 meters | NO |
| D-003 Candlewick Marina | 42.3412, -71.0923 | ~533 meters | NO |
| D-004 Everlight Landing | 42.3458, -71.0890 | ~24 meters | YES |

**Why?**
The Haversine formula provides precise great-circle distances. Only Everlight Landing falls within the 50-meter accuracy threshold, making it the strongest quantitative candidate for the grounding location.

---

### Step 5: Analyze Captain's Log Chronologically

Open Captain's Logs application and review all entries.

Critical entries revealing location clues:

**Dec 24, 14:00:**
"Approaching the old landing near the chapel. Water seems shallow here. Depth sounder reading 2.1 meters."

**Dec 24, 17:30:**
"Found evidence of recent activity near the historic dock. Footprints in snow leading to old structure. Chapel bells ringing in distance."

**Dec 24, 23:30 [FINAL ENTRY]:**
"Ice forming rapidly. Attempting to navigate away from shallow area but vessel responding sluggishly. Draft too deep for this location. Chapel still visible to northeast."

**Why?**
The captain's narrative provides qualitative evidence that must match the quantitative data. Key phrases: "old landing", "near the chapel", "historic dock", "shallow water", "depth 2.1 meters" - all describe a specific location.

---

### Step 6: Cross-Reference All Evidence

Correlate quantitative and qualitative data:

**Quantitative Evidence:**
- Distance: 24 meters (only dock within 50m threshold)
- Coordinates: 42.3458°N, 71.0890°W matches AIS ping location

**Qualitative Evidence from Captain's Log:**
- "Historic dock" → D-004 features list "Historic site"
- "Near chapel" → D-004 features list "Near chapel"
- "Shallow water, 2.1m depth" → D-004 depth listed as "2m"
- "Old landing" → D-004 status "MAINTENANCE" (not active)

**Physical Evidence:**
- Vessel draft: 2.5m
- Dock depth: 2m
- Result: Vessel would ground in shallow water

**Why?**
Multi-source verification is critical in OSINT investigations. When both mathematical calculations and narrative descriptions converge on the same location, confidence in the conclusion reaches near-certainty. The depth mismatch (2.5m draft vs 2m depth) explains the grounding incident.

---

### Step 7: Verify with River Map

Open the River Map application for visual confirmation.

- Locate Everlight Landing on the SVG chart
- Verify proximity to chapel landmark
- Note the shallow water designation in that area
- Confirm historic site marking

**Why?**
Visual cartographic verification provides final confirmation. The River Map shows geographical context that supports all previous evidence - the chapel landmark, shallow water zones, and historic site designation all align with the identified location.

---

### Step 8: Format and Submit Flag

Based on comprehensive analysis:

- Dock Name: Everlight Landing
- Dock ID: D-004

Accepted variations:
- Everlight Landing
- D-004

---

## Skills Developed

- Maritime coordinate system analysis (latitude/longitude)
- Haversine distance calculation methodology
- Multi-source OSINT correlation techniques
- Chronological timeline analysis
- Physical constraint verification (draft vs depth)
- Cross-platform data triangulation
- Qualitative and quantitative evidence synthesis
- Critical thinking and elimination methodology

---

## Answer Key

The investigation requires collecting and correlating these key data points:

**AIS Data:**
1. Ghost ping coordinates: 42.3456°N, 71.0892°W
2. Vessel status: Stationary 6+ hours
3. Timestamp: Dec 24, 23:47

**Vessel Specifications:**
4. Name: FROSTSTAR
5. Type: Patrol Vessel (18m)
6. Draft: 2.5m
7. Captain: Hollis Wintergale

**Dock Registry - D-004:**
8. Name: Everlight Landing
9. Coordinates: 42.3458°N, 71.0890°W
10. Depth: 2m (shallow water)
11. Features: Historic site, Near chapel
12. Status: MAINTENANCE

**Captain's Log Clues:**
13. "Old landing near the chapel"
14. "Historic dock"
15. "Shallow water, depth 2.1 meters"
16. "Chapel bells ringing in distance"
17. "Chapel still visible to northeast"

**Mathematical Verification:**
18. Distance calculation: ~24 meters (within 50m threshold)
19. Depth analysis: 2.5m draft > 2m depth = grounding confirmed

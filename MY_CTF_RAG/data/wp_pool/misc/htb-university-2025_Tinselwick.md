# htb university 2025 Tinselwick

**Challenge Name:** The Tinsel Conspiracy: Registry Corruption Investigation
**Creator:** Joaquin Iglesias (Joey)
**Prepared by:** Joaquin Iglesias (Joey)
**Date:** December 17, 2025
**Difficulty Level:** Easy
**Classification:** Official

---

## Case Briefing

FROM: Chief Elf Hollybranch
TO: Security Analyst
SUBJECT: URGENT: Registry Tampering Investigation
CLASSIFICATION: CONFIDENTIAL
DATE: December 22, 2024 08:30 EST

Analyst,

Automated security systems detected unauthorized modifications to the Naughty & Nice Registry during off-peak hours. Admin override bypassed all standard approval protocols. With Christmas approaching, we need immediate identification of the tampered record. Use all available forensic tools to investigate.

---

## Step-by-Step Walkthrough & Explanations

### Step 1: Review the Investigation Brief

Open the Document Viewer application from the desktop.

- Read "Investigation Brief" document
- Note the key objective: Identify child whose status changed from NAUGHTY to NICE in last 48 hours
- Expected answer format: TC-#### (Child ID)
- Note connection to broader Starshard mystery

**Why?**
Understanding the scope and objectives ensures focused investigation. The brief establishes that we're looking for a status change specifically from NAUGHTY to NICE, not other modifications.

---

### Step 2: Examine the Security Alert Report

In Document Viewer, read "Suspicious Activity" document.

Key timeline discovered:
- Dec 22, 08:14 - Administrative login
- Dec 22, 08:15 - Database write operation
- Dec 22, 08:16 - Security flag raised
- Dec 22, 08:17 - Emergency backup triggered

Critical detail: Override performed by **admin_elf_42**

**Why?**
The precise timeline establishes when the tampering occurred. This 3-minute window on December 22nd provides the exact timeframe to focus investigation efforts.

---

### Step 3: Review Field Monitoring Reports

Still in Document Viewer, read "Field Reports" document.

Focus on TC-5503 (Bramble Shadowpine):
- Dec 15: Caught stealing cookies, refused to share toys
- Dec 16: Broke window, blamed younger child (deception confirmed)
- Dec 17: Lied about homework
- Dec 18: Attempted to take lunch money
- Dec 19-21: **NO REPORTS FILED** (highly unusual)

**Why?**
The documented pattern of dishonesty and escalating behavior contradicts any legitimate status improvement. The 3-day reporting gap immediately before the override is suspicious and suggests potential corruption of the monitoring process itself.

---

### Step 4: Compare Registry Snapshots with Archive Viewer

Open Archive Viewer application from desktop.

Compare snapshots chronologically:

**Dec 15 Snapshot - TC-5503:**
- Status: NAUGHTY
- Good Deeds: 12
- Naughty Acts: 38
- Last Updated: Dec 15, 2024 11:20

**Dec 18 Snapshot - TC-5503:**
- Status: NAUGHTY (unchanged)
- Good Deeds: 12 (no change)
- Naughty Acts: 38 (no change)
- Last Updated: Dec 15, 2024 11:20 (no updates during gap)

**Dec 21 Snapshot - TC-5503:**
- Status: NAUGHTY (still unchanged)
- Good Deeds: 12 (no change)
- Naughty Acts: 38 (no change)

**Dec 22 Snapshot - TC-5503:**
- Status: **NICE** (CHANGED!)
- Good Deeds: 31 (increased by 19)
- Naughty Acts: 5 (decreased by 33)
- Last Updated: Dec 22, 2024 08:15

**Why?**
The dramatic overnight transformation is impossible through legitimate means. No child goes from 38 naughty acts to 5 and gains 19 good deeds in minutes. The update timestamp matches exactly with the security alert timeline.

---

### Step 5: Use Change Detector for Precise Analysis

Open Change Detector application.

Select snapshots to compare:
- Baseline: Dec 21, 2024
- Current: Dec 22, 2024

System identifies exactly ONE modified record:
- Child ID: TC-5503
- Field: status
- Old Value: NAUGHTY
- New Value: NICE
- Field: goodDeeds
- Old Value: 12
- New Value: 31
- Field: naughtyActs
- Old Value: 38
- New Value: 5

**Why?**
The Change Detector provides forensic-level precision, eliminating any ambiguity. Only TC-5503 shows status modification between these snapshots, confirming this is the tampered record.

---

### Step 6: Verify with Audit Log Analysis

Open Audit Log Reader application.

Search for TC-5503 entries:

1. Dec 15, 11:20:33 - "status set to NAUGHTY" by elf_council_review
2. **Dec 22, 08:15:47 - "status set to NICE" by admin_elf_42 [OVERRIDE FLAG]**
3. Dec 22, 08:16:23 - "AUDIT: Manual override detected - flagged for review" by security_monitor

**Why?**
The audit trail provides immutable proof of unauthorized access. The override flag indicates this action bypassed normal approval workflows, confirming tampering rather than legitimate review.

---

### Step 7: Cross-Reference Current Registry

Open Registry Viewer to confirm current state.

Locate TC-5503 in active registry:
- Name: Bramble Shadowpine
- Current Status: NICE
- Good Deeds: 31
- Naughty Acts: 5
- Last Updated: Dec 22, 2024 08:15
- Notes: "Recently updated"

**Why?**
Verifying the current state confirms the tampering persists in the live system. The generic "Recently updated" note replaces the previous "Under review" designation, further indicating cover-up attempt.

---

### Step 8: Submit Flag

Open Flag Format modal (button in top-right corner).

Submit the Child ID in the correct format: **TC-5503**

**Why?**
The investigation conclusively identifies TC-5503 as the compromised record through multiple independent verification methods: archive comparison, change detection, audit logs, and field reports all converge on this single child ID.

---

## Skills Developed

- Digital forensics and change detection
- Database snapshot comparison analysis
- Audit log investigation and timeline reconstruction
- Cross-referencing multiple data sources
- Pattern recognition in behavioral data
- Manual override and authorization bypass detection
- Evidence correlation across disparate systems

---

## Answer Key

**FLAG:** TC-5503

### Evidence Summary:

1. **Archive Analysis:** TC-5503 status changed from NAUGHTY (Dec 21) to NICE (Dec 22)
2. **Numerical Impossibility:** Good deeds jumped from 12 to 31, naughty acts dropped from 38 to 5 overnight
3. **Audit Trail:** Override performed by admin_elf_42 at Dec 22, 08:15:47
4. **Field Reports:** Documented pattern of dishonesty, theft, and deception through Dec 18
5. **Reporting Gap:** Suspicious 3-day monitoring silence (Dec 19-21) preceding override
6. **Timeline Correlation:** Database modification timestamp matches security alert exactly
7. **Bypass Indicators:** Manual override flag, no approval workflow, off-hours access

### Investigation Conclusion:

Bramble Shadowpine (TC-5503) was fraudulently elevated from NAUGHTY to NICE status through unauthorized administrative override. The tampering involved sophisticated data manipulation including retroactive good deed insertion and naughty act deletion. Field monitoring was potentially compromised during the 3-day gap preceding the override. This case demonstrates insider threat capabilities and suggests connection to the broader Starshard conspiracy investigation.

**Recommended Actions:** Immediate status reversion, investigation of admin_elf_42 credentials, audit of all records modified in 48-hour window, review of access control protocols.

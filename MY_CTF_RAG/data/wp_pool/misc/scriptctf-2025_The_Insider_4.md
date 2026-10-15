# scriptctf 2025 The Insider 4

# The Insider 4 | NoobMaster
- Description: Good luck! Note: max flag limit is 6 for a reason, you should be able to get it in less than that. If not, open a ticket. Flag is case insensitive

# Write-up

Looking at github repo from Insider 3, we can see a folder with "The Insider 4" challenge details including 2 attachments and a note. Note says to check comments. On fireworks.jpg, comment mention Wendell Family -> Gives location: Rockport TX.
Finding hotel is pretty easy, to find room number (AND CONFIRM IT for people needing more attempts) you can:
1. Go behind the hotel and see "107" and "109" as two rooms. Check this image: ![Room Numbers](numbers.png)
2. Get a range of > 109. Check this image: ![Calculations](calculations.png) 
3. so just guess from 110-115 (room number is 111)

# Flag - scriptCTF{901_Hwy_35_N_111}

/*
    Copyright (C) 2005 Michael K. McCarty & Fritz Bronner

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

// This file executes the results of the newscast (event cards, etc.)

#include "news_suq.h"

#include <algorithm>

#include "astros.h"
#include "Buzz_inc.h"
#include "game_main.h"
#include "mc.h"
#include "mission_util.h"
#include "news.h"
#include "news_sup.h"
#include "options.h"
#include "pace.h"
#include "place.h"
#include "state_utils.h"


// Roll a six die
int rollSixDie(int nrolls) {
    int result = 0;
    for (int i = 0; i < nrolls; i++) {
        result += brandom(6) + 1;
    }
    return result;
}

/**
 *
 * \return true (1) if need to scrub mission because of crews
 */
char CheckCrewOK(char plr, char pad)
{
    char RT_value = 0;
    int P_crew, B_crew, prog, mis;
    P_crew = Data->P[plr].Mission[pad].PCrew - 1;
    B_crew = Data->P[plr].Mission[pad].BCrew - 1;
    prog = Data->P[plr].Mission[pad].Prog;
    mis = Data->P[plr].Mission[pad].MissionCode;

    if (mis != 0 && Data->P[plr].CrewCount[prog][P_crew] == 0 
    && Data->P[plr].CrewCount[prog][B_crew] == 0
        && Data->P[plr].Mission[pad].Men > 0) {
        if (!AI[plr]) {
            switch (pad) {
            case 0:
                Help("i138");
                break;

            case 1:
                Help("i139");
                break;

            case 2:
                Help("i140");
                break;
            }

            RT_value = 1;
        }
    }

    return RT_value;
}


char REvent(char plr)
{
    int NoMods = 1, i = 0, j = 0;

    if (NoMods == 1) {

	struct BuzzData *pData = &Data->P[plr];
                
        if (pData->Budget < 40) {
            pData->Budget = 40;
        }

	j = 5; // for Budget over 160
	int range[6] = {0, 50, 90, 110, 140, 160}; // Budget ranges
        
        for(int k = 0; k < 5; k++){
             if (pData->Budget > range[k] && pData->Budget <= range[k+1]) {
        	j = k;
             }            
        }

        std::vector<int> nTable; // vector for NTABLE
        DESERIALIZE_JSON_FILE(&nTable, locate_file("ntable.json", FT_DATA));
        //int index =  (plr * 60) + (j * 10) + brandom(10);
        int index =  (j * 10) + brandom(10); // The first table is used for both players
        // TODO; Change the budget table (+60 / +120) according to difficulty
        if (index >= 0 && index < nTable.size()){
            Data->P[plr].Budget += nTable[index];
        } else {
            std:cerr << "Index out of ntable range: " << index << std::endl;
            return 1;
        }
        
    } // end if

    if (Data->Year <= 60 && Data->Events[Data->Count] > 44) {
        return 1;
    }

    if (Data->Year <= 62 && Data->Events[Data->Count] > 63) {
        return 1;
    }

    if (Data->Year <= 65 && Data->Events[Data->Count] > 80) {
        return 1;
    }
    
    // evflag defined in news.h
    switch (Data->Events[Data->Count]) {
    case  0:
    case  1:
    case 44:
    case 59:
    case 63:  // Nothing Events
    case 67:
    case 68:
    case 69:
    case 70:
    case 71:
    case 72:
    case 73:
    case 74:
    case 75:
    case 81:
    case 86:
    case 87:
    case 88:
    case 90:
    case 97:
    case 98:
        break;

    // Cash Modifier Events --------------------------

    case 30:
    case 32:
        evflag = brandom(10) + 1;
        Data->P[plr].Cash += evflag;
        break;

    case 31:
        evflag = rollSixDie(3);
        Data->P[plr].Cash += evflag;
        break;

    case 33:
        if (Data->P[plr].Cash < 10) {
            return 1;
        }

        evflag = Data->P[plr].Cash / 10;
        Data->P[plr].Cash -= evflag;
        break;

    // Budget Modifier Events ------------------------

    case  9:
    case 76:  // Budget increase of 15 MB
        Data->P[plr].Budget += 15;
        break;

    case 45:  // increase budget by total two 10-sided dice this year
        evflag = brandom(10) + brandom(10) + 2;
        Data->P[plr].Budget += evflag;
        break;

    case 46:  // increase budget by 5 MB's
        Data->P[plr].Budget += 5;
        break;

    case 91:  // increase budget by 20MB's in the budget
        Data->P[plr].Budget += 20;
        break;

    case 92:  // 20% or 20MB's whichever is less
        evflag = std::min(Data->P[plr].Budget / 5, 20);
        Data->P[plr].Budget -= evflag;
        break;

    case 95:  // 30% or 30MB's whichever is less
        Data->P[plr].Budget -= std::min(Data->P[plr].Budget * 3 / 10, 30);
        evflag = Data->P[plr].Budget;
        break;

    case 96:  // 25% or 25MB's whichever is less
        if (Data->Year > 72) {
            return 1;
        }

        Data->P[plr].Budget -= std::min(Data->P[plr].Budget / 4, 25);
        evflag = Data->P[plr].Budget;
        break;

    case 99:  // deduct 5 MB's from budget
        Data->P[plr].Budget -= 5;
        break;

    // RD Modifier Events ----------------------------

    case  2:
    case 12:
    case 13:
    case 14:  // RD improve by 1: season
    case 15:
    case 16:
    case 17:
    case 18:
        Data->P[plr].RD_Mods_For_Turn += 1;
        break;

    case  3:
    case 61:
    case 62:  // Minus one on all R&D for ONE year
        Data->P[plr].RD_Mods_For_Turn -= 1;
        Data->P[plr].RD_Mods_For_Year = -1;
        break;

    case 19:  // minus 2 on each die roll for R&D this turn
        Data->P[plr].RD_Mods_For_Turn -= 2;
        break;

    case 41:  // R&D will decrease for one year
        Data->P[plr].RD_Mods_For_Turn -= 2;
        Data->P[plr].RD_Mods_For_Year -= 2;
        break;

    case 42:
    case 43:  /* increment R&D cost by 1 on most advanced program */
        evflag = RDMods(plr, 0, 1, 1);

        if (evflag == 0) {
            return 1;
        }
        break;


    // Improve/Subtract Tech to Other players -------------------

    case 5:
    case 47: // Improve tech of plr prog from rival prog
        evflag = Steal(plr, 0, 1);

        if (!evflag) return 1;
        break;
 
    case 6:
    case 7: // Lower tech of plr prog from rival prog
        evflag = Steal(plr, 0, -1);

        if (!evflag) return 1;
        break;
    
    
    // Special Events -------------------------------------------

    case  8:  // Allow females into space program
        Data->P[plr].FemaleAstronautsAllowed = 1;
        break;

    // Launch Affected ------------------------------------------

    case 4:  // Next Launch must be a manned launch.  If the launch
        // is not then -10 on Budget
        //Data->P[plr].Plans=1;
        break;

    case 10: {  /* Launch window for Launch #x is blocked */
        int choice[3] = {0, 0, 0};

        evflag = 0;

        for (int i = 0; i < 3; i++) {
            if(Data->P[plr].Mission[i].MissionCode) {
                choice[evflag++] += i + 1;
            }
        }

        if (!evflag) return 1;

        evflag = choice[brandom(evflag)] - 1;
        xMODE |= xMODE_CLOUDS;
        ScrubMission(plr, evflag);
    }
    break;

    case 35:  // may launch only one mission this year ??? Season?!?
        evflag = 0;

        if (Data->P[plr].Mission[0].Joint == 1 ||
            Data->P[plr].Mission[1].Joint == 1) {
            return 1;
        }

        for (int i = 0; i < 3; i++) {
            if(Data->P[plr].Mission[i].MissionCode) {
                evflag += i * 2;
            }
        }

        if (!evflag) return 1;

        switch (evflag) {
        case 1:
        case 2:
        case 4:
            evflag = 0;
            break;

        case 3:
            ScrubMission(plr, 1);
            break;

        case 5:
        case 6:
            ScrubMission(plr, 2);
            break;

        case 7:
            ScrubMission(plr, 1);
            ScrubMission(plr, 2);
            break;
        }

        if (evflag == 0) {
            return 1;
        }
        break;

    case 36:  /* damage launch facility */
        if (Data->P[plr].Mission[0].MissionCode == Mission_None) {
            Data->P[plr].LaunchFacility[0] = 15;
        } else {
            return 1;
        }
        break;

    // Program Saving cards ------------------------------------

    case 11:  // Set Safety Card for most Advanced program
        evflag = SaveMods(plr, 0);

        if (!evflag) return 1;
        break;

    case 48:  // Set Safety Card for advanced Rocket program
        evflag = SaveMods(plr, 2);

        if (!evflag) return 1;
        break;

    case 77:  // set Safety Card for advanced Capsule Program
        evflag = SaveMods(plr, 3);

        if (!evflag) return 1;
        break;

    case 93:  // set Safety Card for advanced LEM Program
        evflag = SaveMods(plr, 4);

        if (!evflag) return 1;
        break;

    // Drop Safety  ------------------------------------

    case 20:  /* the most advanced rocket program is affected.
                 drop the safety factor in half. */
        evflag = 0;

        for (i = 3; i >= 0; i--) {
            if (Data->P[plr].Rocket[i].Num >= 0 && Data->P[plr].Rocket[i].Safety > 50) {
                evflag++;
            }
        }

        if (!evflag) return 1;

        for (i = 3; i >= 0; i--) {
            if (Data->P[plr].Rocket[i].Num >= 0 && Data->P[plr].Rocket[i].Safety > 50) {
                evflag = Data->P[plr].Rocket[i].Safety / 2;
                Data->P[plr].Rocket[i].Safety -= evflag;
                strcpy(&Name[0], &Data->P[plr].Rocket[i].Name[0]);
                break;
            }
        }
        break;

    // Increase Safety   ------------------------------------

    case 21:  // Probes' and Sats' Max SF and RD up 5% */
        for (i = 0; i < 3; i++) {
            Data->P[plr].Probe[i].MaxRD += 5;
            Data->P[plr].Probe[i].MaxSafety += 5;
        }

        for (i = 0; i < 3; i++) {
            if (Data->P[plr].Probe[i].MaxSafety >= 99) {
                Data->P[plr].Probe[i].MaxSafety = 99;
            }

            if (Data->P[plr].Probe[i].MaxRD > Data->P[plr].Probe[i].MaxSafety) {
                Data->P[plr].Probe[i].MaxRD = Data->P[plr].Probe[i].MaxSafety;
            }
        }
        break;

    case 22:
    case 84:  
    case 94:  /*  this applies for the most advanced capsule program. 
                  roll four 6-sided dice and add to current safety factor. */
        x = rollSixDie(4);
        evflag = NMod(plr, 3, 1, x);

         if (!evflag) return 1;

        evflag = x;
        break;    

    case 23:  /* this applies to the most advanced rocket program.
                 roll six 6-sided dice and add to current safety factor. */
        x = rollSixDie(6);
        evflag = NMod(plr, 2, 1, x);

        if (!evflag) return 1;

        evflag = x;
        break;

    case 24:  /*  this applies for the most advanced satellite program. 
                  roll four 6-sided dice and add to current safety factor. */
        x = rollSixDie(4);
        evflag = NMod(plr, 1, 1, x);

        if (!evflag) return 1;

        evflag = x;
        break;

    case 26:  /* select most advanced capsule program and reduce safety by 25%  */
        evflag = NMod(plr, 3, -1, 25);

        if (!evflag) return 1;

        evflag = 25;
        break;

    case 27:  /* select most advanced probe program and reduce safety by 15%  */
        evflag = NMod(plr, 1, -1, 15);

        if (!evflag) return 1;

        evflag = 15;
        break;

    case 34:  /* 20% loss most advanced capsule program */
        evflag = NMod(plr, 3, -1, 20);

        if (!evflag) return 1;

        evflag = 20;
        break;

    case 79:  /* select most advanced program and reduce safety by 20%  */
        evflag = NMod(plr, 0, -1, 20);

        if (!evflag) return 1;

        evflag = 20;
        break;

    case 28:  // hardware cost 50% less this season
    case 56:
        Data->P[plr].TurnOnly = 3;
        break;

    case 29:  // set the delay of the rockets to 2 == one year
        for (i = 0; i < 5; i++) {
            Data->P[plr].Rocket[i].Delay = 2;
        }
        break;

    // Cost to Repair or Safety Loss   ------------------------------------

    case 25:  /* cost 15MB repair or 20% safety loss on the most advanced rocket program*/
        evflag = DamMod(plr, 2, -20, 15);

        if (!evflag) return 1;
        break;

    case 37:  /* cost 10MB repair or 10% safety loss on the most advanced probe program*/
        evflag = DamMod(plr, 1, -10, 10);

        if (!evflag) return 1;
        break;

    case 38:
    case 39:  /* cost 10MB repair or 5% safety loss on the most advanced rocket program*/
        evflag = DamMod(plr, 2, -5, 10);

        if (!evflag) return 1;
        break;

    case 40:  /* cost 10MB repair or 10% safety loss on the most advanced program*/
        evflag = DamMod(plr, 0, -10, 10);

        if (!evflag) return 1;
        break;

    case 54:  /* cost 15 MB repair or 20% safety loss on the most advanced rocket program*/
        evflag = DamMod(plr, 2, -20, 15);

        if (!evflag) return 1;
        break;

    case 55:  /* cost 20 MB repair or 10% safety loss on the most advanced capsule program*/
        evflag = DamMod(plr, 3, -10, 20);

        if (!evflag) return 1;
        break;

    case 78:  /* cost 10MB repair or 10% safety loss on the most advanced capsule program*/
        evflag = DamMod(plr, 3, -10, 10);

        if (!evflag) return 1;
        break;

    // Astronaut Portion ------------------------------------

    case 49:  /* pick random astronaut retire, budget minus 5 MB's */
        evflag = 0;

        for (i = 0; i < Data->P[plr].AstroCount; i++) {
            if (!(Data->P[plr].Pool[i].Status == AST_ST_DEAD || 
            Data->P[plr].Pool[i].Status == AST_ST_RETIRED)) {
                evflag++;
            }
        }

        if (evflag) {
          i = brandom(Data->P[plr].AstroCount);
          Data->P[plr].Budget -= 5;
        } else {
          return 1;
        }

        while (Data->P[plr].Pool[i].Status == AST_ST_DEAD || 
        Data->P[plr].Pool[i].Status == AST_ST_RETIRED) {
            i = brandom(Data->P[plr].AstroCount);
        }

        Data->P[plr].Pool[i].RetirementDelay = 2;
        Data->P[plr].Pool[i].Mood = 10;
        strcpy(&Name[0], &Data->P[plr].Pool[i].Name[0]);

        if (plr == 1) {
            Data->P[plr].Pool[i].Status = AST_ST_RETIRED;
            Data->P[plr].Pool[i].RetirementDelay = 0;
        }

        if (Data->P[plr].Pool[i].Status == AST_ST_RETIRED) {
            CheckFlightCrews(plr);
        }
        break;

    case 50:
    case 85:  // backup crew will fly mission
        evflag = 0;

        // Event text specifies the "Primary Crew", so check for a
        // Primary crew rather than manned.
        for (i = 0; i < MAX_MISSIONS; i++) {
            if (Data->P[plr].Mission[i].MissionCode != Mission_None &&
                Data->P[plr].Mission[i].PCrew > 0) {
                evflag = i + 1;
                break;
            }
        }

        if (!evflag) return 1;

        ClearMissionCrew(plr, --evflag, CREW_PRIMARY);
        break;

    case 51:  /* astronaut killed delay all manned missons = 1 */
        evflag = 0;

        for (i = 0; i < Data->P[plr].AstroCount; i++) {
            if (!(Data->P[plr].Pool[i].Status == AST_ST_DEAD || 
            Data->P[plr].Pool[i].Status == AST_ST_RETIRED))  {
                evflag++;
            }
        }

        if (!evflag) return 1;

        Data->P[plr].Budget -= 5;
        i = brandom(Data->P[plr].AstroCount);

        while (Data->P[plr].Pool[i].Status == AST_ST_DEAD || 
        Data->P[plr].Pool[i].Status == AST_ST_RETIRED) {
            i = brandom(Data->P[plr].AstroCount);
        }

        strcpy(&Name[0], &Data->P[plr].Pool[i].Name[0]);
        Data->P[plr].Pool[i].Status = AST_ST_DEAD;
        Data->P[plr].MissionCatastrophicFailureOnTurn = 2;
        xMODE |= xMODE_SPOT_ANIM;  //trigger spot anim
        CheckFlightCrews(plr);

        //cancel manned missions
        for (size_t pad = 0; pad < MAX_LAUNCHPADS; pad++) {
            if (Data->P[plr].Mission[pad].Men > 0) {
                ScrubMission(plr, pad);
            }
        }
        break;

    case 52:
        evflag = 0;

        for (i = 0; i < Data->P[plr].AstroCount; i++) {
            if (!(Data->P[plr].Pool[i].Status == AST_ST_DEAD || 
            Data->P[plr].Pool[i].Status == AST_ST_RETIRED))  {
                evflag++;
            }
        }

        if (!evflag) return 1;

        i = brandom(Data->P[plr].AstroCount);

        while (Data->P[plr].Pool[i].Status == AST_ST_DEAD || 
        Data->P[plr].Pool[i].Status == AST_ST_RETIRED) {
            i = brandom(Data->P[plr].AstroCount);
        }

        strcpy(&Name[0], &Data->P[plr].Pool[i].Name[0]);
        Data->P[plr].Pool[i].Status = AST_ST_DEAD;
        Data->P[plr].MissionCatastrophicFailureOnTurn = 2;
        CheckFlightCrews(plr);
        break;

    case 60:
    case 53:
        evflag = 0;

        for (i = 0; i < Data->P[plr].AstroCount; i++) {
            if (!(Data->P[plr].Pool[i].Status == AST_ST_DEAD || 
            Data->P[plr].Pool[i].Status == AST_ST_RETIRED))  {
                evflag++;
            }
        }

        if (!evflag) return 1;

        i = brandom(Data->P[plr].AstroCount);

        while (Data->P[plr].Pool[i].Status == AST_ST_DEAD || 
        Data->P[plr].Pool[i].Status == AST_ST_RETIRED) {
            i = brandom(Data->P[plr].AstroCount);
        }

        strcpy(&Name[0], &Data->P[plr].Pool[i].Name[0]);
        Data->P[plr].Pool[i].Status = AST_ST_INJURED;
        Data->P[plr].Pool[i].InjuryDelay = 2;

        if (options.feat_use_endurance && (brandom(100) < (Data->P[plr].Pool[i].Endurance - 1) * 25)) {
            Data->P[plr].Pool[i].InjuryDelay = 1;  // High endurance can shorten time in hospital to half a year
        }

        if (options.feat_use_endurance && Data->P[plr].Pool[i].Endurance < 1 && brandom(100) < 25) {
            Data->P[plr].Pool[i].InjuryDelay = 3;  // An Endurance of 0 can mean 3 turns in the hospital instead
        }

        Data->P[plr].Pool[i].Special = 4;
        CheckFlightCrews(plr);
        break;

    case 57:  /* transfer new nauts to space program */
        NewNauts(plr);
        break;

    case 58:  // 50% chance explosion on pad
        Data->P[plr].TurnOnly = 5;
        break;

    case 64:  /* launch facility repair 10MB's */
        for (j = 0; j < 20; j++) {
            i = rand() % 3;

            if (Data->P[plr].LaunchFacility[i] == LAUNCHPAD_OPERATIONAL 
            && Data->P[plr].Mission[i].MissionCode == Mission_None) {
                break;
            }
        }

        if (j == 20) return 1;

        /* get which of the three facilities damaged */
        Data->P[plr].LaunchFacility[i] = 10;
        break;

    case 65:  // Gemini or Voskhod will cost additional 2MB's per spacecraft
        if (Data->P[plr].Manned[MANNED_HW_TWO_MAN_CAPSULE].Num < 0) {
            return 1;
        }

        Data->P[plr].Manned[MANNED_HW_TWO_MAN_CAPSULE].UnitCost += 2;  // Used to say "InitCost", which effectively disabled this newscast -Leon
        break;

    case 83:  // Apollo or Soyuz will cost additional 3MB's per spacecraft
        if (Data->P[plr].Manned[MANNED_HW_THREE_MAN_CAPSULE].Num < 0) return 1;

        Data->P[plr].Manned[MANNED_HW_THREE_MAN_CAPSULE].UnitCost += 3;
        break;

    case 66:  // mission delay
        evflag = 0;

        for (i = 0; i < 3; i++) {
            if (Data->P[plr].Mission[i].MissionCode) {

                if (!IsJoint(Data->P[plr].Mission[i].MissionCode) &&
                    MissionTimingOk(Data->P[plr].Mission[i].MissionCode,
                                    Data->Year, Data->Season)) {
                    evflag++;
                }
            }
        }

        if (!evflag) return 1;

        i = brandom(3);

        while (Data->P[plr].Mission[i].MissionCode == Mission_None ||
               Data->P[plr].Mission[i].Joint == 1 ||
               ! MissionTimingOk(Data->P[plr].Mission[i].MissionCode,
                                 Data->Year, Data->Season)) {
            i = brandom(3);
        }

        evflag = i;
        DelayMission(plr, i);
        break;

    case 80:  // Can't Deliver any Cap/Mods this year
        for (i = 0; i < 5; i++) {
            Data->P[plr].Manned[i].Delay = 2;
        }
        break;

    case 82:  // Duration E Mission Required
        Data->P[plr].FuturePlans = 5;
        break;

    case 89:  // random active astronaut leaves program
        evflag = 0;

        for (i = 0; i < Data->P[plr].AstroCount; i++) {
            if (Data->P[plr].Pool[i].Status == AST_ST_ACTIVE) {
                evflag++;
            }
        }

        if (!evflag) return 1;

        i = brandom(Data->P[plr].AstroCount);

        while (Data->P[plr].Pool[i].Status != AST_ST_ACTIVE) {
            i = brandom(Data->P[plr].AstroCount);
        }

        Data->P[plr].Pool[i].RetirementDelay = 2;
        strcpy(&Name[0], &Data->P[plr].Pool[i].Name[0]);

        if (plr == 1) {
            Data->P[plr].Pool[i].Status = AST_ST_RETIRED;
            Data->P[plr].Pool[i].RetirementDelay = 0;
        }

        if (Data->P[plr].Pool[i].Status == AST_ST_RETIRED) {
            CheckFlightCrews(plr);
        }
        break;

    default:
        break;
    }  // End of Switch

    return 0;
}

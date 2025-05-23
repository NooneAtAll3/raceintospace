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
//****************************************************************
//*Interplay's BUZZ ALDRIN's RACE into SPACE                     *
//*                                                              *
//*Formerly -=> LiftOff : Race to the Moon :: IBM version MCGA   *
//*Copyright 1991 by Strategic Visions, Inc.                     *
//*Designed by Fritz Bronner                                     *
//*Programmed by Michael K McCarty                               *
//*                                                              *
//****************************************************************

// NewsCaster Main Files
// This file controls the newscast.

#include "news.h"

#include <stdexcept>

#include "display/graphics.h"
#include "display/surface.h"
#include "display/palettized_surface.h"

#include "Buzz_inc.h"
#include "draw.h"
#include "filesystem.h"
#include "gamedata.h"
#include "game_main.h"
#include "gr.h"
#include "logging.h"
#include "mmfile.h"
#include "news_suq.h"
#include "pace.h"
#include "sdlhelper.h"
#include "utils.h"

/* LOG_DEFAULT_CATEGORY(LOG_ROOT_CAT); */

static const char *news_shots[] = { "angle", "opening", "closing" };

double load_news_anim_start;

#define PHYS_PAGE_OFFSET  0x4000
#define BUFFR_FRAMES 1
#define FIRST_FRAME 0
#define TOMS_BUGFIX 69

int evflag;
static int bufsize, LOAD_US = 0, LOAD_SV = 0;
static int Frame, MaxFrame, AnimIndex = 255;

enum news_type {
    NEWS_ANGLE,
    NEWS_OPENING,
    NEWS_CLOSING
};

/* country, color, type */
static int news_index[2][2][3] = {
    { /* USA */
        { 9, 1, 3 }, /* color */
        { 8, 0, 2 }, /* b&w */
    },
    { /* Soviet */
        { 10, 4, 6 }, /* color */
        { 11, 5, 7 }, /* b&w */
    },
};

static std::vector<std::string> event;
static std::vector<std::string> naut_news;
static std::vector<std::string> reasons;
static std::vector<std::string> news;

struct rNews {
    int16_t offset;
    char chrs;
};


void GoNews(char plr);
void OpenNews(char plr, char *buf, int bud);
void DispNews(char plr, char *src, char *dest);
void DrawNText(char plr, char got);
char ResolveEvent(char plr);
int PlayNewsAnim(mm_file *);
mm_file *LoadNewsAnim(int plr, int bw, int type, int Mode, mm_file *fp);
void ShowEvt(char plr, char crd);
void LoadEventData(char plr);
void LoadNewsData(char plr);


void
GoNews(char plr)
{
    int i, j;
    struct rNews *list;

    memset(Name, 0x00, sizeof Name);
    memset(buffer, 0x00, 20480);   // clear buffer
    display::graphics.setForegroundColor(1);

    j = ResolveEvent(plr);

    if (j > 0) {
        Data->Events[Data->Count] = j + 99;
    }

    OpenNews(plr, buffer + 6000, (int) Data->Events[Data->Count]);

    Data->P[plr].eCount++;
    assert(Data->P[plr].eCount <= MAX_NEWS_ITEMS / 2);

    DispNews(plr, buffer + 6000, buffer);

    j = 0;
    memset(buffer + 6000, 0x00, 8000);  // clear memory
    list = (struct rNews *)(buffer + 6000);

    for (i = 0; i < (int) strlen(buffer); i++) {
        if (buffer[i] == 'x') {
            list[i].chrs = j;
            list[i].offset = i;
        } else {
            j++;
        }
    }
}


// Open News Constructs a complete event array.
void OpenNews(char plr, char *buf, int bud)
{
    int size = 250;
    //size = (plr == 0) ? 232 : 177;
    
    LoadEventData(plr);
    
    // News Intro - "GOOD EVENING..."
    strncpy(&buf[0], event[0].c_str(), event[0].size());
    
    // Event News
    bufsize = strlen(buf);
    strncpy(&buf[bufsize], event[bud + 2].c_str(), event[bud + 2].size());
 
    bufsize = strlen(buf);
    buf[bufsize] = 'x';

    //Astronaut info
    LoadNewsData(plr);
    
    int i = 0; // counter

    for (int j = 0; j < Data->P[plr].AstroCount; j++) {
        if (Data->P[plr].Pool[j].Special > 0) {
            i++;
        }
    }

    // Nauts in the news....
    bufsize = strlen(buf);
    
    if (i > 0) {
        strncpy(&buf[bufsize], naut_news[0].c_str(), naut_news[0].size());
    }
    
    for (int j = 0; j < Data->P[plr].AstroCount; j++) {
        if (Data->P[plr].Pool[j].Special > 0) {
            // 12 ideas
            bufsize = strlen(buf);
            strcpy(&buf[bufsize], Data->P[plr].Pool[j].Name); // Copy Naut Name
            i = Data->P[plr].Pool[j].Special;
            bufsize = strlen(buf);
            strncpy(&buf[bufsize], naut_news[i].c_str(), naut_news[i].size());
        }

        /* Show reasons for retirement announcements, mission deaths (8), and
           retirements due to mission injuries (9) */
        // (Special == 1,) OR (Special greater than 0 AND RetirementReason 8 or 9).
        if ( (Data->P[plr].Pool[j].Special == 1) || 
             (Data->P[plr].Pool[j].Special > 0 && 
             (Data->P[plr].Pool[j].RetirementReason == 8 || Data->P[plr].Pool[j].RetirementReason == 9)) ) {
            //13 other things
            i = Data->P[plr].Pool[j].RetirementReason - 1;

            bufsize = strlen(buf);
            strncpy(&buf[bufsize], reasons[i].c_str(), reasons[i].size());
        }

        Data->P[plr].Pool[j].Special = 0;
    }

    if (Data->Year >= 58) {
        if (Data->Season == 1 && plr == 0) {
            strcpy(&buf[strlen(buf)], "xCHECK INTELLIGENCE: THE CIA REPORTS NEW INFORMATION.x\0");
        } else if (Data->Season == 1 && plr == 1) {
            strcpy(&buf[strlen(buf)], "xCHECK INTELLIGENCE: THE KGB REPORTS NEW INFORMATION.x\0");
        }
    }

    //------------------------------------------------------
    //Specs: check tracking station for director's message |
    //------------------------------------------------------
    if (Option != -1) {
        FILE *messages = sOpen((Option == 0) ? "SENDR.MSG" : "SENDH.MSG", "rb", FT_DATA);
        char old[120];

        if (messages != NULL) {
            fread(&old, sizeof(old), 1, messages);

            if (old[0] != 0x00) {
                if (Option == 0) {
                    strcpy(&buf[strlen(buf)], "xCHECK THE TRACKING STATION: THE SOVIETx");
                    strcpy(&buf[strlen(buf)], "DIRECTOR HAS SENT A NEW MESSAGE.x\0");
                } else if (Option == 1) {
                    strcpy(&buf[strlen(buf)], "xCHECK THE TRACKING STATION: THE UNITEDx");
                    strcpy(&buf[strlen(buf)], "STATES DIRECTOR HAS SENT A NEW MESSAGE.x\0");
                }
            }

            fclose(messages);
        }
    }

    bufsize = strlen(buf);

    if (Data->P[plr].Plans & 0xff) {
        display::graphics.setForegroundColor(16);
        strcpy(&buf[bufsize], "xPLANETARY MISSION UPDATES...x");
    }

    // Past Mission Info
    if (Data->P[plr].Plans & 0x0f) {
        // Failures
        display::graphics.setForegroundColor(6);

        if (Data->P[plr].Plans & 0x01) {
            strcpy(&buf[strlen(buf)], "MARS FLYBY FAILS!x");
        }

        if (Data->P[plr].Plans & 0x02) {
            strcpy(&buf[strlen(buf)], "JUPITER FLYBY FAILS!x");
        }

        if (Data->P[plr].Plans & 0x04) {
            strcpy(&buf[strlen(buf)], "SATURN FLYBY FAILS!x");
        }
    }

    if (Data->P[plr].Plans & 0xf0) {
        display::graphics.setForegroundColor(5);

        if (Data->P[plr].Plans & 0x10) {
            display::graphics.setForegroundColor(13);
            strcpy(&buf[strlen(buf)], "MARS FLYBY SUCCEEDS!x");
        }

        if (Data->P[plr].Plans & 0x20) {
            display::graphics.setForegroundColor(23);
            strcpy(&buf[strlen(buf)], "JUPITER FLYBY SUCCEEDS!x");
        }

        if (Data->P[plr].Plans & 0x40) {
            display::graphics.setForegroundColor(16);
            strcpy(&buf[strlen(buf)], "SATURN FLYBY SUCCEEDS!x");
        }
    }

    Data->P[plr].Plans = 0;

    // History News
    bufsize = strlen(buf);

    // Also in the news... 
    strncpy(&buf[bufsize], news[0].c_str(), news[0].size());

    bufsize = strlen(buf);

    if (plr == 0) {
        // Pool of 6 us news per year
        i = (Data->Year - 57) * 6 + Data->Season * 3 + brandom(3) + 1;
    } else {
        // Pool of 4 soviet news per year
        i = (Data->Year - 57) * 4 + Data->Season * 2 + brandom(2) + 1;
    }

    strncpy(&buf[bufsize], news[i].c_str(), news[i].size());
    strcat(buf, "x");
    bufsize = strlen(buf);
    
    // News Outro
    strncpy(&buf[bufsize], event[1].c_str(), event[1].size());
}


void
DispNews(char plr, char *src, char *dest)
{
    int j = 0, k = 0;

    for (int i = 0; i < (int) strlen(src); i++) {
        dest[j] = src[i];

        switch (dest[j]) {
        case 'a':
        case 'd':
        case 'f':
        case 'j':
        case 'i':
            snprintf(&Name[0], sizeof(Name), "%d", evflag);
            strncpy(&dest[j], &Name[0], strlen(Name));
            j += strlen(Name) - 1;
            break;

        case 'b':
            switch (evflag) {
            case 0:
                strcpy(&Name[0], "FIRST");
                break;

            case 1:
                strcpy(&Name[0], "SECOND");
                break;

            case 2:
                strcpy(&Name[0], "THIRD");
                break;
            }

            strncpy(&dest[j], &Name[0], strlen(Name));

            j += strlen(Name) - 1;

            break;

        case 'c':
            strncpy(&dest[j], &Name[0], strlen(Name));
            j += strlen(Name) - 1;
            break;

        case 'e':
            strncpy(&dest[j], &Name[0], strlen(Name));
            j += strlen(Name) - 1;
            break;

        case 'h':
            switch (evflag) {
            case 0:
                strcpy(&Name[0], "PRIMARY");
                break;

            case 1:
                strcpy(&Name[0], "SECONDARY");
                break;

            case 2:
                strcpy(&Name[0], "THIRD");
                break;

            default:
                break;
            }

            strncpy(&dest[j], &Name[0], strlen(Name));
            j += strlen(Name) - 1;
            break;

        case 'g':
            if (plr == 0) {
                if (Data->Year <= 59) {
                    strcpy(&Name[0], "EISENHOWER");
                }

                if (Data->Year >= 60 && Data->Year <= 63) {
                    strcpy(&Name[0], "KENNEDY");
                }

                if (Data->Year >= 64 && Data->Year <= 67) {
                    strcpy(&Name[0], "JOHNSON");
                }

                if (Data->Year >= 68 && Data->Year <= 73) {
                    strcpy(&Name[0], "NIXON");
                }

                if (Data->Year >= 74 && Data->Year <= 75) {
                    strcpy(&Name[0], "FORD");
                }

                if (Data->Year >= 76 && Data->Year <= 79) {
                    strcpy(&Name[0], "CARTER");
                }
            }

            if (plr == 1) {
                if (Data->Year < 64) {
                    strcpy(&Name[0], "KHRUSHCHEV");
                }

                if (Data->Year == 64 && Data->Season == 0) {
                    strcpy(&Name[0], "KHRUSHCHEV");
                }

                if (Data->Year == 64 && Data->Season == 1) {
                    strcpy(&Name[0], "BREZHNEV");
                }

                if (Data->Year > 65) {
                    strcpy(&Name[0], "BREZHNEV");
                }
            }

            strncpy(&dest[j], &Name[0], strlen(Name));

            j += strlen(Name) - 1;

            break;

        case 'm':
            strncpy(&dest[j], Data->P[plr].Name, strlen(Data->P[plr].Name));
            j += strlen(Data->P[plr].Name) - 1;
            break;

        case 'x':
            k = 0;
            break;

        case ' ':
            if (k > 34) {
                dest[j] = 'x';
                k = 0;

                if (src[i + 1] == ' ') {
                    i++;
                }
            }

        default:
            break;
        }

        j++;
        k++;
    }
}


void
DrawNText(char plr, char got)
{
    int xx = 12, yy = 128, i;
    char *buf;

    buf = buffer;
    display::graphics.setForegroundColor(1);

    for (i = 0; i < got; i++) {
        while (*buf != 'x') {
            buf++;
        }

        buf++;

        if (strncmp(&buf[0], "ASTRONAUTS IN THE NEWS", 22) == 0) {
            display::graphics.setForegroundColor(11);
        }

        if (strncmp(&buf[0], "ALSO IN THE NEWS", 16) == 0) {
            display::graphics.setForegroundColor(12);
        }

        if (strncmp(&buf[0], "IN COSMONAUT NEWS", 17) == 0) {
            display::graphics.setForegroundColor(11);
        }

        if (strncmp(&buf[0], "OTHER EVENTS IN THE NEWS", 24) == 0) {
            display::graphics.setForegroundColor(12);
        }

        if (strncmp(&buf[0], "PLANETARY", 9) == 0) {
            display::graphics.setForegroundColor(11);
        }

        if (strncmp(&buf[0], "CHECK INTEL", 11) == 0) {
            display::graphics.setForegroundColor(11);
        }

        if (strncmp(&buf[0], "CHECK THE TRACKING STATION", 26) == 0) {
            display::graphics.setForegroundColor((plr == 0) ? 9 : 7);
        }
    }

    for (i = 0; i < 8; i++) {
        fill_rectangle(5, yy - 7, 296, yy + 1, 7 + 3 * plr);
        grMoveTo(xx, yy);

        if (strncmp(&buf[0], "ASTRONAUTS IN THE NEWS", 22) == 0) {
            display::graphics.setForegroundColor(11);
        }

        if (strncmp(&buf[0], "ALSO IN THE NEWS", 16) == 0) {
            display::graphics.setForegroundColor(12);
        }

        if (strncmp(&buf[0], "IN COSMONAUT NEWS", 17) == 0) {
            display::graphics.setForegroundColor(11);
        }

        if (strncmp(&buf[0], "OTHER EVENTS IN THE NEWS", 24) == 0) {
            display::graphics.setForegroundColor(12);
        }

        if (strncmp(&buf[0], "PLANETARY", 9) == 0) {
            display::graphics.setForegroundColor(11);
        }

        if (strncmp(&buf[0], "AND THAT'S THE NEWS", 19) == 0) {
            display::graphics.setForegroundColor(11);
        }

        if (strncmp(&buf[0], "THIS CONCLUDES OUR NEWS", 23) == 0) {
            display::graphics.setForegroundColor(11);
        }

        if (strncmp(&buf[0], "CHECK INTEL", 11) == 0) {
            display::graphics.setForegroundColor(11);
        }

        if (strncmp(&buf[0], "CHECK THE TRACKING STATION", 26) == 0) {
            display::graphics.setForegroundColor((plr == 0) ? 9 : 7);
        }

        while (buf[0] != 'x' && buf[0] != '\0') {
            draw_character(buf[0]);
            buf++;
        }

        yy += 9;

        buf++;

        if (*buf == '\0') {
            i = 9;
        }
    }

}


void
News(char plr)
{
    int bline = 0, ctop = 0, i;
    char cYr[5];
    char loc = 0;
    uint8_t Status = 0, BW = 0;
    mm_file video_file, *fp = &video_file;

    BW = (Data->Year <= 63);

    memset(fp, 0, sizeof(*fp));

    // Autosave Fix
    // An autosave stores the state of the game at the time it was
    // created, after the news has been generated for the turn. However,
    // the news broadcast is still repeated. Rather than skip the
    // introduction, the current turn is calculated, compared with the
    // number of news events (1/turn) and a flag is set to replay the
    // news if the save has already accounted for the events.
    int turn = 2 * (Data->Year - 57) + Data->Season + 1;  // start at turn 1
    bool freshNews = (turn > Data->P[plr].eCount);

    if (freshNews) {
        GoNews(plr);

        // GoNews increments player eCount, so it should equal the turn
        // File Structure is 84 longs 42 per side
        size_t eventIndex = plr * 42 + Data->P[plr].eCount - 1;
        interimData.tempEvents.at(eventIndex) = buffer;

    } else {
        // Copy the recorded news event for the turn into the buffer
        // so it will be available for display.
        strncpy(buffer, interimData.tempEvents.at(plr * 42 + turn - 1).c_str(),
                BUFFER_SIZE - 1000);
        memset(buffer + BUFFER_SIZE - 1000, 0, 999);
    }

    if ((plr == 0 && LOAD_US == 0) || (plr == 1 && LOAD_SV == 0)) {

        snprintf(cYr, sizeof(cYr), "%d", 1900 + Data->Year);

        if (Data->Season == 1) {
            draw_heading(42 + (BW * 200), 40 - (plr * 4), "FALL", 0, -1);
        } else {
            draw_heading(37 + (BW * 200), 40 - (plr * 4), "SPRING", 0, -1);
        }

        draw_heading(48 + (BW * 200), 63 - (4 * plr), &cYr[0], 0, -1);

    }

    for (i = 0; i < (int) strlen(buffer); i++) {
        if (buffer[i] == 'x') {
            bline++;
        }
    }

    bline -= 8;

    // Leaving this here for now, rather than moving it to the
    // loop -- rnyoakum
    music_start_loop((plr % 2) ? M_NEW1950 : M_NEW1970, false);

    /* Tom's News kludge, also open and load first anim */
    fp = LoadNewsAnim(plr, BW, NEWS_ANGLE, TOMS_BUGFIX, fp);
    loc = 1;
    Status = 0;

    helpText = "i002";
    WaitForMouseUp();

    while (1) {
        key = 0;
        GetMouse_fast();

        if (!(loc == 0 && Status == 1)) {
            NUpdateVoice();
        }

        i = AnimSoundCheck();

        if (Status == 1 || (loc == 3 && i == 1))
            switch (loc) {
            case 0:        //: Angle In
                AnimIndex = 255;
                LoadNewsAnim(plr, BW, NEWS_ANGLE, FIRST_FRAME, fp);
                Status = 0;
                loc++;
                break;

            case 1:        //: Intro
                LoadNewsAnim(plr, BW, NEWS_OPENING, FIRST_FRAME, fp);
                Status = 0;

                if (AnimIndex == 5) {

                    fill_rectangle(227, 108, 228, 108, display::graphics.legacyScreen()->getPixel(227, 108));

                }

                NGetVoice(plr, 0);
                PlayNewsAnim(fp);
                PlayNewsAnim(fp);
                PlayNewsAnim(fp);
                PlayNewsAnim(fp);
                loc++;
                break;

            case 2:        //: Event (sound)
                loc++;

                if (IsChannelMute(AV_SOUND_CHANNEL)) {
                    /* if no sound then just skip event picture */
                    Status = 1;
                    break;
                }

                NGetVoice(plr, Data->Events[Data->Count] + 2);
                Status = 0;
                i = bline;
                ShowEvt(plr, Data->Events[Data->Count]);
                bline = i;
                break;

            case 3:        //: Close
                LoadNewsAnim(plr, BW, NEWS_CLOSING, FIRST_FRAME, fp);
                Status = 0;
                NGetVoice(plr, 1);

                if (plr == 0) {
                    // This is done to sync with audio
                    PlayNewsAnim(fp);
                    PlayNewsAnim(fp);
                    PlayNewsAnim(fp);
                    PlayNewsAnim(fp);
                    PlayNewsAnim(fp);
                }

                loc++;

                break;

            case 4:        //: Angle Out
                music_stop();  // Should have ended, but force stop.
                music_start_loop((plr % 2) ? M_NEW1950 : M_NEW1970, false);
                LoadNewsAnim(plr, BW, NEWS_ANGLE, FIRST_FRAME, fp);
                Status = 0;
                loc++;
                break;

            case 5:        //: Event (no sound)
                Status = 0;
                loc++;

                if (AnimIndex == 9) {

                    ShBox(240, 3, 316, 22);
                    fill_rectangle(315, 20, 317, 21, 3);
                    fill_rectangle(241, 2, 242, 4, 3);
                    IOBox(243, 3, 316, 19);
                    display::graphics.setForegroundColor(1);
                    draw_string(258, 13, "CONTINUE");

                }

                i = bline;

                ShowEvt(plr, Data->Events[Data->Count]);

                bline = i;

                break;

            default:
                break;
            };

        if (loc != 3 && loc != 6 && !Status) {
            Status = PlayNewsAnim(fp);
        } else {
            idle_loop_secs(.05);    /* was: .125 */
        }

        //: Repeat News Sequence
        if (key == 'R' && loc == 6) {
            stop_voice();
            loc = 0;
            Status = 1;
        }

        if (ctop > 0 && key == K_HOME) {
            // Home Key
            ctop = 0;
            DrawNText(plr, ctop);

        } else if (ctop > 0 && key == K_PGUP) {
            // Page Up Key
            ctop -= 7;

            if (ctop < 0) {
                ctop = 0;
            }

            DrawNText(plr, ctop);
        } else if (ctop < bline && key == K_PGDN) {
            // Page Down Key
            ctop += 7;

            if (ctop > bline) {
                ctop = bline;
            }

            DrawNText(plr, ctop);
        } else if (ctop < bline && key == K_END) {
            // End Key
            ctop = bline;
            DrawNText(plr, ctop);

        } else if (ctop > 0 && ((x >= 303 && y > 120 && x <= 313 && y <= 156
                                 && mousebuttons > 0) || (key >> 8) == 72)) {
            // Up Arrow
            InBox(303, 120, 313, 156);
            ctop--;
            DrawNText(plr, ctop);
            OutBox(303, 120, 313, 156);
        } else if ((x >= 245 && y >= 5 && x <= 314 && y <= 17
                    && mousebuttons > 0) || (key == K_ENTER || key == K_ESCAPE)) {
            // Continue
            InBox(245, 5, 314, 17);
            WaitForMouseUp();
            key = 0;
            music_stop();
            stop_voice();
            break;
        } else if (ctop < bline && ((x >= 303 && y > 158 && x <= 313
                                     && y <= 194 && mousebuttons > 0) || (key >> 8) == 80)) {
            // Down Arrow
            InBox(303, 158, 313, 194);
            ctop++;
            DrawNText(plr, ctop);
            OutBox(303, 158, 313, 194);
        }

        if (ctop <= 0) {
            draw_up_arrow(305, 126);
        } else {
            draw_up_arrow_highlight(305, 126);
        }

        if (ctop >= bline) {
            draw_down_arrow(305, 163);
        } else {
            draw_down_arrow_highlight(305, 163);
        }

//   gr_sync ();
    }

    mm_close(fp);

    display::graphics.newsRect().w = 0;

    display::graphics.newsRect().h = 0;
}


void
AIEvent(char plr)
{
    int turn = 2 * (Data->Year - 57) + Data->Season + 1;  // start at turn 1
    bool freshNews = (turn > Data->P[plr].eCount);

    ResolveEvent(plr);

    // Do not update event counter when loading saved game
    if (freshNews) {
        Data->P[plr].eCount++;
        assert(Data->P[plr].eCount <= MAX_NEWS_ITEMS / 2);
    }
}

// ResolveEvent seems to set a flag in the BadCard array
// and return the index into that array
char
ResolveEvent(char plr)
{
    int bad, ctr = 0;

    bad = REvent(plr);

    if (bad) {
        // News event was bad, find an open slot to record the bad event
        do {
            ctr++;
            bad = brandom(14);

            if (ctr > 30) {
                // After 30 good faith random attempts to find an open slot
                // reset the BadCard array
                memset(&Data->P[plr].BadCardEventFlag[0], 0x00, sizeof(Data->P[plr].BadCardEventFlag));
                ctr = 0;
            }
        } while (Data->P[plr].BadCardEventFlag[bad] != 0);

        Data->P[plr].BadCardEventFlag[bad] = 1;
        bad++;
    }

    return bad;                    // zero if card is good
}


/* modified to return true if end of anim */
int
PlayNewsAnim(mm_file *fp)
{
    double delta;
    double fps = 15;            /* TODO hardcoded fps here! */
    int skip_frame = 0;

    if (Frame == MaxFrame) {
        return 1;
    }

    delta = get_time() - load_news_anim_start;

    if (Frame < (int)(delta * fps)) {
        skip_frame = 1;
    }

    if (mm_decode_video(fp, display::graphics.newsOverlay()) <= 0) {
        MaxFrame = Frame;
        return 1;
    }

    /* TODO skipping frames should not use decode_video */
    if (!skip_frame) {
        static double diff = 0;
        double t1 = get_time(), to_sleep;

        to_sleep = (Frame - delta * fps) / fps - diff;
        idle_loop_secs(to_sleep);
        diff = get_time() - t1 - to_sleep;
        CTRACE4(video, "sleep % 4.3f, drift % 4.3f", to_sleep, diff);
    }

    Frame += 1;

    return 0;
}


static void
DrawTopNewsBox(int player)
{
    OutBox(0, 0, 319, 113);
    display::graphics.legacyScreen()->outlineRect(1, 1, 318, 112, 3);
    display::graphics.legacyScreen()->outlineRect(2, 2, 317, 111, 3);
    InBox(3, 3, 316, 110);
    ShBox(240, 3, 316, 22);
    fill_rectangle(315, 20, 317, 21, 3);
    fill_rectangle(241, 2, 242, 4, 3);
    IOBox(243, 3, 316, 19);
    display::graphics.setForegroundColor(1);
    draw_string(258, 13, "CONTINUE");
}


static void
DrawBottomNewsBox(int player)
{
    ShBox(0, 115, 319, 199);
    InBox(4, 118, 297, 196);
    fill_rectangle(5, 119, 296, 195, 7 + 3 * player);
    InBox(301, 118, 315, 196);
    fill_rectangle(302, 119, 314, 195, 0);
    ShBox(303, 120, 313, 156);
    ShBox(303, 158, 313, 194);
    draw_up_arrow(305, 126);
    draw_down_arrow(305, 163);
    DrawNText(player, 0);
}


mm_file *
LoadNewsAnim(int plr, int bw, int type, int Mode, mm_file *fp)
{
    int Index = news_index[plr][bw][type];

    if (AnimIndex != Index) {
        char fname[100];
        unsigned h = 0, w = 0;

        mm_close(fp);
        display::graphics.newsRect().w = 0;
        display::graphics.newsRect().h = 0;

        snprintf(fname, sizeof(fname), "%s_%s_%s.ogg",
                 plr ? "sov" : "usa",
                 bw  ? "bw"  : "col",
                 news_shots[type]);

        /* XXX error checking */
        mm_open_fp(fp, sOpen(fname, "rb", FT_VIDEO));

        /* XXX we know fps anyway */
        mm_video_info(fp, &w, &h, NULL);
        display::graphics.newsRect().h = h;
        display::graphics.newsRect().w = w;
        display::graphics.newsRect().x = 4;
        display::graphics.newsRect().y = 4;
    }

    Frame = 1;
    AnimIndex = Index;
    MaxFrame = 0;

    // Specs: Display Single Frame
    if (Mode == FIRST_FRAME && fp->video) {
        /* XXX: error checking */
        mm_decode_video(fp, display::graphics.newsOverlay());
    }

    // *************** TCS001 my kludge (tom) 3/15/94
    if (Mode == TOMS_BUGFIX) {
        FadeOut(2, 1, 0, 0);

        display::graphics.screen()->clear();
        DrawTopNewsBox(plr);
        DrawBottomNewsBox(plr);

        /* XXX: error checking */
        if (fp->video) {
            mm_decode_video(fp, display::graphics.newsOverlay());
        }

        /* This fade was too long given current fades impl. */
        FadeIn(2, 10, 0, 0); /* was: 50 */
    }

    load_news_anim_start = get_time();

    return fp;
}


/**
 * Draws the News image associated with a given news event.
 *
 * News event images use a 128-color palette from [128, 255].
 * Overwrites the display color palette from [32, 128) with black
 * and loads the image's 128-color palette into [128, 255].
 *
 * \param plr  0 for USA event images, 1 for USSR event images.
 * \param crd  the index of the news event (0-114).
 */
void
ShowEvt(char plr, char crd)
{
    // Clear out the palette from color index 32 onward.
    {
        display::AutoPal p(display::graphics.legacyScreen());
        memset(&p.pal[96], 0, 672);
    }

    // Apparently not all of the news events are associated with the
    // right images? This would suggest some images get used twice.
    if (plr == 0) {
        switch (crd) {
        case 41:
            crd = 45;
            break;

        case 50:
            crd = 67;
            break;

        case 8:
            crd = 98;
            break;

        case 105:
            crd = 112;
            break;

        case 33:
            crd = 111;
            break;

        case 109:
            crd = 108;
            break;

        default:
            break;
        }
    }

    char filename[128];
    snprintf(filename, sizeof(filename),
             "images/news/news.cdr.%s.%d.png",
             plr == 0 ? "usa" : "ussr", (int) crd);

    boost::shared_ptr<display::PalettizedSurface> image;

    try {
        image = Filesystem::readImage(filename);
    } catch (const std::runtime_error &err) {
        CERROR4(filesys, "error loading %s: %s", filename, err.what());
        return;
    }

    image->exportPalette(128, 255);

    display::graphics.screen()->draw(image, 0, 0);
    DrawTopNewsBox(plr);

    display::graphics.newsRect().w = 0;
    display::graphics.newsRect().h = 0;
}

void LoadEventData(char plr) {
    // Deserialize Events
    std::ifstream file(locate_file("event.json", FT_DATA));
    if (!file) {
        throw std::runtime_error("event.json could not be opened.");
    }
    cereal::JSONInputArchive ar(file);
    
    if (plr == 0) {
        ar(cereal::make_nvp("us_event", event));
    } else {
        ar(cereal::make_nvp("sov_event", event));
    }
}

void LoadNewsData(char plr) {
    // Deserialize Nauts and Historic News
    std::ifstream file(locate_file("news.json", FT_DATA));
    if (!file) {
        throw std::runtime_error("news.json could not be opened.");
    }
    cereal::JSONInputArchive ar(file);
    
    if (plr == 0) {
        ar(cereal::make_nvp("astro_news", naut_news));
        ar(cereal::make_nvp("us_reasons", reasons));
        ar(cereal::make_nvp("us_news", news));
    } else {
        ar(cereal::make_nvp("cosmo_news", naut_news));
        ar(cereal::make_nvp("sov_reasons", reasons));
        ar(cereal::make_nvp("sov_news", news));
    }
}


// EOF

/***************************************************************************
* Copyright (C) 2010 by Ammar Qammaz *
* ammarkov@gmail.com *
* *
* This program is free software; you can redistribute it and/or modify *
* it under the terms of the GNU General Public License as published by *
* the Free Software Foundation; either version 2 of the License, or *
* (at your option) any later version. *
* *
* This program is distributed in the hope that it will be useful, *
* but WITHOUT ANY WARRANTY; without even the implied warranty of *
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the *
* GNU General Public License for more details. *
* *
* You should have received a copy of the GNU General Public License *
* along with this program; if not, write to the *
* Free Software Foundation, Inc., *
* 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA. *
***************************************************************************/

#ifndef VIDEOINPUT_H_INCLUDED
#define VIDEOINPUT_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

struct VideoFeedSettings
{
   unsigned int EncodingType;
   unsigned int PixelFormat;
   unsigned int FieldType;
};

char * VideoInput_Version();

int InitVideoInputs(int numofinputs);
int CloseVideoInputs();
int InitVideoFeed(int inpt,char * viddev,int width,int height,char snapshots_on,struct VideoFeedSettings videosettings);
int FeedReceiveLoopAlive(int feed_num);
int PauseFeed(int feednum);
int UnpauseFeed(int feednum);
//int InitVideoInput(char * vid1 , char * vid2,char snapshots_on);
//int CloseVideoInput();
unsigned char * GetFrame(int webcam_id);

// Playback / Recording
void Play(char * filename);
void Record(char * filename);
void RecordOne(char * filename);
void Stop();
unsigned int VideoSimulationState();
#ifdef __cplusplus
}
#endif


#endif // VIDEOINPUT_H_INCLUDED

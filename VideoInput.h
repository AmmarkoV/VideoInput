#ifndef VIDEOINPUT_H_INCLUDED
#define VIDEOINPUT_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

char * VideoInput_Version();

int InitVideoInputs(int numofinputs);
int CloseVideoInputs();
int InitVideoFeed(int inpt,char * viddev,int width,int height,char snapshots_on);
int SyncFeeds(int feed1,int feed2);
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

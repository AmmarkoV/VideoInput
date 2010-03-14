#ifndef VIDEOINPUT_H_INCLUDED
#define VIDEOINPUT_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

char * VideoInput_Version();
int InitVideoInput(char * vid1 , char * vid2,char snapshots_on);
int CloseVideoInput();
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

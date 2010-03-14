#ifndef IMAGE_STORAGE_H_INCLUDED
#define IMAGE_STORAGE_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

struct Image
{
  char * pixels;
  unsigned int size_x;
  unsigned int size_y;
  unsigned int depth;
};

int ReadDoubleRAW(char * filename,struct Image * left_pic,struct Image * right_pic);
int WriteDoubleRAW(char * filename,struct Image * left_pic,struct Image * right_pic);
int ClearImage(struct Image * pic );

#ifdef __cplusplus
}
#endif


#endif // IMAGE_STORAGE_H_INCLUDED
